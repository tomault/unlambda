#include "vm_instructions.h"
#include "vmmem.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct VmMemoryImpl_ {
  /** The VM memory itself */
  uint8_t* bytes;

  /** End of the VM memory */
  uint8_t* end;

  /** Maximum size of the memory */
  uint64_t maxSize;
  
  /** Start address of the heap.  Also doubles as the end of the memory
   *  reserved for the program.
   */
  uint64_t heapStart;

  /** Number of bytes free on the heap */
  uint64_t bytesFree;

  /** Address of first free block */
  uint64_t firstFree;

  /** Current status.  0 == no error */
  int statusCode;

  /** Current error message.  "OK" if no error */
  const char* statusMsg;
  
} VmMemoryImpl;

/** "OK" message that indicates no error */
static const char OK_MSG[] = "OK";

/** Default error message when we can't allocate enough memory for a more
 *  detailed error message.
 */
static const char DEFAULT_ERR_MSG[] = "ERROR";

/** Values for the allocated block types */
const int VmmFreeBlockType = 0;
const int VmmCodeBlockType = 1;
const int VmmStateBlockType = 2;

/** Values for the error codes */
const int VmmInvalidArgumentError = -1;
const int VmmBadBlockError = -2;
const int VmmMaxSizeExceededError = -3;
const int VmmSizeIncreaseFailedError = -4;
const int VmmNotEnoughMemoryError = -5;
const int VmmHeapInUseError = -6;

static void writeFreeBlock(uint8_t* where, uint64_t size, uint64_t next);
static int ptrOutOfBounds(VmMemory memory, uint8_t* p);
static HeapBlock* allocateBlock(VmMemory memory, uint64_t size);
static FreeBlock* findFreeBlockWithSize(VmMemory memory, uint64_t size,
					FreeBlock** prevFree);
static HeapBlock* splitFreeBlock(VmMemory memory, FreeBlock* block,
				 FreeBlock* prev, uint64_t size);
static void visitBlock(VmMemory memory, uint64_t address,
		       GcErrorHandler errorHandler, void* errorContext);
static void visitCodeBlock(VmMemory memory, CodeBlock* block,
			   GcErrorHandler errorHandler, void* errorContext);
static void visitVmStateBlock(VmMemory memory, VmStateBlock* block,
			      GcErrorHandler errorHandler, void* errorContext);
static int collectUnmarkedBlocks(VmMemory memory, GcErrorHandler errorHandler,
				 void* errorContext);

static uint64_t alignTo8(uint64_t v) {
  return (v + 7) & ~(uint64_t)7;
}

uint8_t getVmmBlockType(const HeapBlock* block) {
  return (uint8_t)((block->typeAndSize >> 56) & 0x03);
}

static void setVmmBlockType(HeapBlock* block, uint8_t type) {
  block->typeAndSize = (block->typeAndSize & 0xFCFFFFFFFFFFFFFF)
                           | ((uint64_t)type << 56);
}

uint64_t getVmmBlockSize(const HeapBlock* block) {
  return block->typeAndSize & 0x00FFFFFFFFFFFFFF;
}

static void setVmmBlockSize(HeapBlock* block, uint64_t size) {
  block->typeAndSize = (block->typeAndSize & 0xFF00000000000000) | size;
}

static int shouldDeallocateStatusMsg(VmMemory memory) {
  return (memory->statusMsg != OK_MSG)
    && (memory->statusMsg != DEFAULT_ERR_MSG);
}

int vmmBlockIsMarked(const HeapBlock* block) {
  return (int)(block->typeAndSize >> 63);
}

void clearVmmBlockMark(HeapBlock* block) {
  block->typeAndSize &= 0x7FFFFFFFFFFFFFFF;
}

void setVmmBlockMark(HeapBlock* block) {
  block->typeAndSize |= 0x8000000000000000;
}

VmMemory createVmMemory(uint64_t initialSize, uint64_t maxSize) {
  /** Check initialSize >= 16 and <= maxSize
   *  16 is the smallest block we can allocate
   */
  if ((initialSize < 16) || (initialSize > maxSize)) {
    return NULL;  /** Invalid arguments */
  }
  
  VmMemoryImpl* memory = (VmMemoryImpl*)malloc(sizeof(VmMemoryImpl));
  if (!memory) {
    return NULL;
  }

  memory->bytes = (uint8_t*)malloc(initialSize);
  if (!memory->bytes) {
    free((void*)memory);
    return NULL;
  }

  memory->end = memory->bytes + initialSize;
  memory->maxSize = maxSize;
  memory->heapStart = 0;
  memory->bytesFree = initialSize - sizeof(HeapBlock);
  memory->firstFree = 0;
  memory->statusCode = 0;
  memory->statusMsg = OK_MSG;

  writeFreeBlock(memory->bytes, initialSize - 8, 0);

  return memory;
}

void destroyVmMemory(VmMemory memory) {
  free((void*)memory->bytes);
  free((void*)memory);
}

static void writeFreeBlock(uint8_t* where, uint64_t size, uint64_t next) {
  uint64_t* const p = (uint64_t*)where;
  p[0] = ((uint64_t)VmmFreeBlockType << 56) | size;
  p[1] = next;
}

int getVmmStatus(VmMemory memory) {
  return memory->statusCode;
}

const char* getVmmStatusMsg(VmMemory memory) {
  return memory->statusMsg;
}

void clearVmmStatus(VmMemory memory) {
  if (shouldDeallocateStatusMsg(memory)) {
    free((void*)memory->statusMsg);
  }
  memory->statusCode = 0;
  memory->statusMsg = OK_MSG;
}

static void setVmmStatus(VmMemory memory, int statusCode,
			 const char* statusMsg) {
  if (shouldDeallocateStatusMsg(memory)) {
    free((void*)memory->statusMsg);
  }

  memory->statusMsg = (const char*)malloc(strlen(statusMsg) + 1);
  if (!memory->statusMsg) {
    memory->statusMsg = DEFAULT_ERR_MSG;
  } else {
    strcpy((char*)memory->statusMsg, statusMsg);
  }
  memory->statusCode = statusCode;
}

uint64_t currentVmmSize(VmMemory memory) {
  return memory->end - memory->bytes;
}

uint64_t maxVmmSize(VmMemory memory) {
  return memory->maxSize;
}

uint64_t vmmBytesFree(VmMemory memory) {
  return memory->bytesFree;
}

uint64_t vmmHeapSize(VmMemory memory) {
  return (memory->end - memory->bytes) - memory->heapStart;
}

int reserveVmMemoryForProgram(VmMemory memory, uint64_t size) {
  const uint64_t alignedSize = alignTo8(size);
  clearVmmStatus(memory);

  /** In order to change the memory reserved for the program, the entire
   *  heap must contain one free block
   */
  uint8_t* heapStart = getVmmHeapStart(memory);
  if ((getVmmBlockType((HeapBlock*)heapStart) != VmmFreeBlockType)
        || (getVmmBlockSize((HeapBlock*)heapStart) !=
	      (vmmHeapSize(memory) - sizeof(HeapBlock)))) {
    setVmmStatus(memory, VmmHeapInUseError,
		 "Cannot change area allocated for the program while the "
		 "heap is in use");
    return -1;
  }

  while (alignedSize > currentVmmSize(memory)) {
    if (increaseVmmSize(memory)) {
      if (getVmmStatus(memory) == VmmMaxSizeExceededError) {
	char msg[100];
	snprintf(msg, sizeof(msg),
		 "Cannot allocate %" PRIu64 " bytes for the program in a "
		 "memory of size %" PRIu64, alignedSize, currentVmmSize(memory));
	setVmmStatus(memory, VmmNotEnoughMemoryError, msg);
      }
      return -1;
    }
  }

  memory->heapStart = alignedSize;

  /** 16 is the minimum block size */
  if (vmmHeapSize(memory) >= 16) {
    /** Update the # of bytes free and write a free block covering the whole
     *  heap.
     */
    memory->bytesFree = currentVmmSize(memory) - alignedSize
                          - sizeof(HeapBlock);
    memory->firstFree = alignedSize;
    writeFreeBlock(memory->bytes + alignedSize, memory->bytesFree, 0);
  } else {
    /** Not enough for even one block, so allocate all of the memory to
     *  the program.
     */
    memory->heapStart = currentVmmSize(memory);
    memory->bytesFree = 0;
    memory->firstFree = 0;
  }
  return 0;
}

uint64_t getVmmProgramMemorySize(VmMemory memory) {
  return memory->heapStart;
}

uint8_t* getProgramStartInVmm(VmMemory memory) {
  return memory->heapStart ? memory->bytes : NULL;
}

uint8_t* getVmmHeapStart(VmMemory memory) {
  return memory->bytes + memory->heapStart;
}

uint8_t* ptrToVmMemory(VmMemory memory) {
  return memory->bytes;
}

uint8_t* ptrToVmMemoryEnd(VmMemory memory) {
  return memory->end;
}

int isValidVmmAddress(VmMemory memory, uint64_t address) {
  return (memory->bytes + address) < memory->end;
}

uint8_t* ptrToVmmAddress(VmMemory memory, uint64_t address) {
  uint8_t* p = memory->bytes + address;
  return ((p >= memory->bytes) && (p < memory->end)) ? p : NULL;
}

uint64_t vmmAddressForPtr(VmMemory memory, const uint8_t* p) {
  return ((p >= memory->bytes) && (p < memory->end)) ? (p - memory->bytes)
                                                     : memory->maxSize;
}

HeapBlock* firstHeapBlockInVmm(VmMemory memory) {
  return (HeapBlock*)(memory->bytes + memory->heapStart);
}

HeapBlock* nextHeapBlockInVmm(VmMemory memory, HeapBlock* block) {
  if (!block) {
    return NULL;
  }
  HeapBlock* next =
    (HeapBlock*)(((uint8_t*)block) + getVmmBlockSize(block)
		      + sizeof(HeapBlock));
  return (((uint8_t*)next >= memory->bytes)
	  && ((uint8_t*)next < memory->end)) ? next : NULL;
}

HeapBlock* forEachVmmBlock(
    VmMemory memory, HeapBlock* (*f)(VmMemory, HeapBlock*, void*),
    void* context
) {
  HeapBlock *block = firstHeapBlockInVmm(memory);
  HeapBlock *result = NULL;
  while (block && !result) {
    result = f(memory, block, context);
    block = nextHeapBlockInVmm(memory, block);
  }
  return result;
}

FreeBlock* firstFreeBlockInVmm(VmMemory memory) {
    /** If firstFree is zero, then either the first free block is at
     *  address zero or there are no free blocks.  Check to see if there
     *  is a program area allocated.  If there is, then firstFree == 0 means
     *  there are no free blocks.  Otherwise, firstFree == 0 means the
     *  first free block is at address zero.
     */
  if (!memory->firstFree && memory->heapStart) {
    /** No free blocks */
    return NULL;  
  }

  return (FreeBlock*)(((uint8_t*)memory->bytes) + memory->firstFree);
}

FreeBlock* nextFreeBlockInVmm(VmMemory memory, FreeBlock* block) {
  if (!block || ptrOutOfBounds(memory, (uint8_t*)block)) {
    /** Invalid block */

    return NULL;
  }

  return block->next ? (FreeBlock*)(memory->bytes + block->next) : NULL;
}

/** This function is not part of VmMemory's public API and is only used
 *  for testing
 */
void setVmmFreeList(VmMemory memory, uint64_t addrOfFirstFreeBlock,
		    uint64_t bytesFree) {
  memory->firstFree = addrOfFirstFreeBlock;
  memory->bytesFree = bytesFree;
}

FreeBlock* forEachFreeBlockInVmm(VmMemory memory,
				 FreeBlock* (*f)(VmMemory, FreeBlock*, void*),
				 void* context) {
  FreeBlock* block = firstFreeBlockInVmm(memory);
  FreeBlock* result = NULL;
  while (block && !result) {
    result = f(memory, block, context);
    block = nextFreeBlockInVmm(memory, block);
  }
  return result;
}

static const uint64_t MAX_BLOCK_SIZE = 0x00FFFFFFFFFFFFF8;

CodeBlock* allocateVmmCodeBlock(VmMemory memory, uint64_t size) {
  if (!size || (size > MAX_BLOCK_SIZE)) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Cannot allocate a block of size %" PRIu64, size);
    setVmmStatus(memory, VmmInvalidArgumentError, msg);
    return NULL;
  }

  HeapBlock* block = allocateBlock(memory, alignTo8(size));
  if (block) {
    setVmmBlockType(block, VmmCodeBlockType);
  }
  return (CodeBlock*)block;
}

VmStateBlock* allocateVmmStateBlock(VmMemory memory, uint32_t callStackSize,
				    uint32_t addressStackSize) {
  const uint64_t neededSize = (16 * (uint64_t)callStackSize)
                                  + (8 * (uint64_t)addressStackSize) + 16;
  VmStateBlock* block = (VmStateBlock*)allocateBlock(memory, neededSize);
  if (!block) {
    return NULL;  /** Status already set */
  }

  setVmmBlockType((HeapBlock*)block, VmmStateBlockType);
  for (int i = 0; i < 8; ++i) {
    block->guard[i] = PANIC_INSTRUCTION;
  }
  block->callStackSize = callStackSize;
  block->addressStackSize = addressStackSize;

  /** Caller needs to copy the call and address stack data into block->stacks
   */
  return block;
}

static HeapBlock* clearBlockMark(VmMemory memory, HeapBlock* block,
				 void* unused) {
  /* printf("Clear block mark at %lu\n",
   *        (uint8_t*)block - ptrToVmMemory(memory));
   */
  clearVmmBlockMark(block);
  return NULL;
}

int collectUnreachableVmmBlocks(VmMemory memory, Stack callStack,
				Stack addressStack,
				GcErrorHandler errorHandler,
				void* errorContext) {
  /** Clear the marks on all the blocks */
  forEachVmmBlock(memory, clearBlockMark, NULL);

  /** Mark all blocks reachable from the call stack */
  for (uint64_t* p = (uint64_t*)bottomOfStack(callStack);
       p < (uint64_t*)topOfStack(callStack);
       p += 2) {
    /* printf("Visit call stack frame\n"); */
    visitBlock(memory, *p, errorHandler, errorContext);
  }

  /** Mark all blocks reachable from the address stack */
  for (uint64_t* p = (uint64_t*)bottomOfStack(addressStack);
       p < (uint64_t*)topOfStack(addressStack);
       ++p) {
    /* printf("Visit address stack frame\n"); */
    visitBlock(memory, *p, errorHandler, errorContext);
  }

  return collectUnmarkedBlocks(memory, errorHandler, errorContext);
}

static void visitBlock(VmMemory memory, uint64_t address,
		       GcErrorHandler errorHandler,
		       void* errorContext) {
  if (address >= memory->heapStart) {
    HeapBlock* block = (HeapBlock*)ptrToVmmAddress(memory,
						   address - sizeof(HeapBlock));

    if (!vmmBlockIsMarked(block)) {
      const int blockType = getVmmBlockType(block);

      setVmmBlockMark(block);

      if (blockType == VmmFreeBlockType) {
	/** Should not have an address pointing to a free block...*/
	clearVmmBlockMark(block);
	errorHandler(memory, vmmAddressForPtr(memory, (uint8_t*)block), block,
		     "Free block is reachable", errorContext);
      } else if (blockType == VmmCodeBlockType) {
	visitCodeBlock(memory, (CodeBlock*)block, errorHandler, errorContext);
      } else if (blockType == VmmStateBlockType) {
	visitVmStateBlock(memory, (VmStateBlock*)block, errorHandler,
			  errorContext);
      } else {
	char msg[100];

	clearVmmBlockMark(block);
	snprintf(msg, sizeof(msg), "Unknown block type %u",
		 (unsigned int)getVmmBlockType(block));
	errorHandler(memory, vmmAddressForPtr(memory, (uint8_t*)block), block,
		     msg, errorContext);
      }
    }
  }
}

static void visitCodeBlock(VmMemory memory, CodeBlock* block,
			   GcErrorHandler errorHandler, void* errorContext) {
  uint8_t* p = block->code;
  uint8_t* end = p + getVmmBlockSize((HeapBlock*)block);

  while (p < end) {
    if (*p == PUSH_INSTRUCTION) {
      /** TODO: Replace this with a read that will work on processors
       *        that don't support unaligned reads
       */
      const uint64_t address = *(uint64_t*)(p + 1);
      // If the operand lies inside the heap, it is the address of the
      // block's data, not the header, which is sizeof(HeapBlock) bytes behind.
      //
      // Could just call visitBlock with the operand and that would ignore
      // operands outside the heap, but this gives us extra checking that
      // if the operand lies inside the heap, it references a code block.
      if (address >= memory->heapStart) {
	HeapBlock* const q = (HeapBlock*)ptrToVmmAddress(
	  memory, address - sizeof(HeapBlock)
	);
	if (!q) {
	  errorHandler(memory, address, q,
		       "Operand of PUSH instruction is invalid",
		       errorContext);

	} else if (getVmmBlockType(q) != VmmCodeBlockType) {
	  errorHandler(memory, address, q,
		       "Operand of PUSH instruction does not point to "
		       "a code block", errorContext);
	} else {
	  visitBlock(memory, address, errorHandler, errorContext);
	}
      }
    }

    p += instructionSize(*p);
  }

  assert(p == end);
}

static void visitVmStateBlock(VmMemory memory, VmStateBlock* block,
			      GcErrorHandler errorHandler,
			      void* errorContext) {
  uint64_t* const callStackEnd =
    (uint64_t*)block->stacks + 2 * (uint64_t)block->callStackSize;
  uint64_t* const addressStackEnd = callStackEnd + block->addressStackSize;

  /** Visit all the addresses in the saved call stack */
  for (uint64_t* p = (uint64_t*)block->stacks; p < callStackEnd; p += 2) {
    visitBlock(memory, *p, errorHandler, errorContext);
  }

  /** Visit all the addresses in the saved address stack */
  for (uint64_t* p = callStackEnd; p < addressStackEnd; ++p) {
    visitBlock(memory, *p, errorHandler, errorContext);
  }
}

static int collectUnmarkedBlocks(VmMemory memory,
				 GcErrorHandler errorHandler,
				 void* errorContext) {
  HeapBlock* p = firstHeapBlockInVmm(memory);
  HeapBlock* prev = NULL;
  FreeBlock* prevFree = NULL;

  memory->bytesFree = 0;
  memory->firstFree = 0;
  
  while (p) {
    /** Get the next block now, since we may overwrite the current block */
    HeapBlock* const next = nextHeapBlockInVmm(memory, p);
    
    /**
    printf("prev = %p, prevFree = %p, p = %p, next = %p\n", prev, prevFree, p,
           next);
    if (p) {
      printf("addr(p) = %lu\n", (uint64_t)((uint8_t*)p - ptrToVmMemory(memory)));
    } else {
      printf("addr(p) = (nil)\n");
    }
    **/
    
    assert(!prev || (nextHeapBlockInVmm(memory, prev) == p));

    if (vmmBlockIsMarked(p)) {
      /* printf("Clear mark\n"); */
      clearVmmBlockMark(p);
      prev = p;
    } else {
      assert(!prev || !vmmBlockIsMarked(prev));
      
      if (prev && (getVmmBlockType(prev) == VmmFreeBlockType)) {
	/* printf("Merge into prior free block\n"); */
	/** prev should be at the end of the free list */
	assert((HeapBlock*)prevFree == prev);
	assert(((FreeBlock*)prev)->next == 0);

	/** Coalesce block into previous free block */
	const uint64_t blockSize = sizeof(HeapBlock) + getVmmBlockSize(p);
	setVmmBlockSize(prev, getVmmBlockSize(prev) + blockSize);
	memory->bytesFree += blockSize;
      } else {
	/* printf("Change to free block\n"); */
	/** Change to free block */
	assert(getVmmBlockSize(p) >= 8);
	
	setVmmBlockType(p, VmmFreeBlockType);
	((FreeBlock*)p)->next = 0;

	if (prevFree) {
	  assert(!prevFree->next);
	  prevFree->next = vmmAddressForPtr(memory, (uint8_t*)p);
	} else {
	  assert(!memory->firstFree);
	  memory->firstFree = vmmAddressForPtr(memory, (uint8_t*)p);
	}	
	memory->bytesFree += getVmmBlockSize(p);

	prev = p;
	prevFree = (FreeBlock*)p;
      }
    }

    p = next;
  }

  /* printf("Done collecting free blocks\n"); */
  assert(!(prev && nextHeapBlockInVmm(memory, prev)));

  if (1) {
    /** Verify that sum of memory in free blocks equals memory->bytesFree */
    /* printf("Verify free byte count\n"); */
    FreeBlock *q = firstFreeBlockInVmm(memory);
    uint64_t bytesFree = 0;
    
    while (q) {
      bytesFree += getVmmBlockSize((HeapBlock*)q);
      q = nextFreeBlockInVmm(memory, q);
    }

    if (bytesFree != memory->bytesFree) {
      /**
      printf("memory->bytesFree = %lu, sum of free blocks = %lu.  "
	     "Signaling error.\n", memory->bytesFree, bytesFree);
       **/
      char msg[200];
      snprintf(msg, sizeof(msg),
	       "After garbage collection, memory->bytesFree is %lu, but "
	       "total number of bytes in free blocks is %lu",
	       memory->bytesFree, bytesFree);
      errorHandler(memory, 0, NULL, msg, errorContext);
    }
  }

  return 0;
}

static int ptrOutOfBounds(VmMemory memory, uint8_t* p) {
  return ((p < memory->bytes) || (p >= memory->end));
}

static HeapBlock* allocateBlock(VmMemory memory, uint64_t size) {
  FreeBlock* prevFree;
  FreeBlock* block = findFreeBlockWithSize(memory, size, &prevFree);
  if (!block) {
    char msg[100];
    snprintf(msg, sizeof(msg),
	     "Could not allocate block of size %" PRIu64 " (Not enough memory)",
	     size);
    setVmmStatus(memory, VmmNotEnoughMemoryError, msg);
    return NULL;
  }

  return splitFreeBlock(memory, block, prevFree, size);
}

static FreeBlock* findFreeBlockWithSize(VmMemory memory, uint64_t size,
					FreeBlock** prevFree) {
  *prevFree = NULL;
  if (size > memory->bytesFree) {
    return NULL;
  }
  
  FreeBlock* block = firstFreeBlockInVmm(memory);

  while (block) {
    if (getVmmBlockSize((HeapBlock*)block) >= size) {
      return block;
    }
    *prevFree = block;
    block = nextFreeBlockInVmm(memory, block);
  }

  return NULL;
}

static const uint64_t MIN_FREE_BLOCK_SIZE = 16;

static HeapBlock* splitFreeBlock(VmMemory memory, FreeBlock* block,
				 FreeBlock* prev, uint64_t size) {
  const uint64_t remaining = getVmmBlockSize((HeapBlock*)block) - size;
  if (remaining < MIN_FREE_BLOCK_SIZE) {
    /** Allocate the whole block */
    if (prev) {
      prev->next = block->next;
    } else {
      /** This is the first free block */
      assert(((FreeBlock*)(memory->bytes + memory->firstFree)) == block);
      memory->firstFree = block->next;
    }
    memory->bytesFree -= getVmmBlockSize((HeapBlock*)block);
    return (HeapBlock*)block;
  } else {
    /** Split the free block in two */
    uint8_t* newFreeBlock = ((uint8_t*)block) + size + sizeof(HeapBlock);
    writeFreeBlock(newFreeBlock, remaining - sizeof(HeapBlock), block->next);
    if (prev) {
      prev->next = newFreeBlock - memory->bytes;
    } else {
      /** This is the first free block */
      assert(((FreeBlock*)(memory->bytes + memory->firstFree)) == block);
      memory->firstFree = newFreeBlock - memory->bytes;
    }
    setVmmBlockSize((HeapBlock*)block, size);
    memory->bytesFree -= size + sizeof(HeapBlock);
    return (HeapBlock*)block;
  }
}

int increaseVmmSize(VmMemory memory) {
  clearVmmStatus(memory);

  const uint64_t currentSize = currentVmmSize(memory);
  
  if (currentSize >= memory->maxSize) {
    setVmmStatus(memory, VmmMaxSizeExceededError,
		 "Maximum memory size exceeded");
    return -1;
  }

  uint64_t newSize = currentSize * 2;
  if ((newSize < currentSize) || (newSize > memory->maxSize)) {
    newSize = memory->maxSize;
  }

  uint8_t* newMemory = (uint8_t*)realloc(memory->bytes, newSize);
  if (!newMemory) {
    setVmmStatus(memory, VmmSizeIncreaseFailedError,
		 "Could not allocate enough memory to increase VMM size");
    return -1;
  }

  memory->bytes = newMemory;
  memory->end = memory->bytes + newSize;
  memory->bytesFree += newSize - currentSize;
  
  /** Find the last free block.  If this block is the last block in
   *  the old heap, extend it to cover the increase in memory size.  If not,
   *  write a new free block at the end of memory and add it to the
   *  free list.
   */
  FreeBlock* p = firstFreeBlockInVmm(memory);
  if (!p) {
    /** There were no free blocks on the heap, so write one at the end
     *  of the old heap and point the free block pointer to it
     */
      writeFreeBlock(memory->bytes + currentSize,
		     newSize - currentSize - sizeof(HeapBlock), 0);
      memory->firstFree = currentSize;

      /** Account for the header of the new block */
      memory->bytesFree -= sizeof(HeapBlock);    
  } else {
    FreeBlock* next = nextFreeBlockInVmm(memory, p);

    while (next) {
      p = next;
      next = nextFreeBlockInVmm(memory, p);
    }

    uint8_t* const nextBlock = (uint8_t*)nextHeapBlockInVmm(memory,
							    (HeapBlock*)p);
    if (nextBlock == (memory->bytes + currentSize)) {
      /** The last block on the old heap is a free block, so just increase its
       *  size to cover the newly-allocated memory
       */
      setVmmBlockSize((HeapBlock*)p,
		      getVmmBlockSize((HeapBlock*)p) + (newSize - currentSize));
    } else {
      /** The last block on the old heap is not a free block, so create a new
       *  free block to cover the newly-added memory and add it to the
       *  free list.
       */
      writeFreeBlock(memory->bytes + currentSize,
		     newSize - currentSize - sizeof(HeapBlock), 0);
      p->next = currentSize;

      /** Account for the header of the new block */
      memory->bytesFree -= sizeof(HeapBlock);
    }
  }

  return 0;
}

