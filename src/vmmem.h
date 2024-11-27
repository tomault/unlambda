#ifndef __VMMEM_H__
#define __VMMEM_H__

/** Simulated memory for the Unlambda virtual machine */
#include <stddef.h>
#include <stdint.h>
#include <stack.h>

/** Header common to all blocks allocated dynamically on the heap */
typedef struct HeapBlock_ {
  /** Block type and size
   *
   *  Bits  0-55: Block size.  This is the number of bytes allocated for
   *                  this block after this header.
   *  Bits 56-57: Block type:
   *      00: Free block
   *      01: Block containing VM code
   *      10: Block containing saved VM state
   *  Bits 58-62: Unused (should be 0)
   *  Bit 63:     Mark for garbage collection
   */
  uint64_t typeAndSize;
} HeapBlock;

/** A block that has been freed */
typedef struct FreeBlock_ {
  /** Block type and size */
  HeapBlock header;
  
  /** Address of next free block */
  uint64_t next;
} FreeBlock;

/** A block that contains VM code, used to store functions created at */
/*  runtime  */
typedef struct CodeBlock_ {
  /** Block type and size */
  HeapBlock header;

  /** VM instructions stored in the block */
  uint8_t code[];
} CodeBlock;

/** A block that stores the VM state for a continuation */
typedef struct VmStateBlock_ {
  HeapBlock header;

  /** Eight PANIC instructions in case the VM tries to execute this
   *  as a code block.  Eight bytes keeps the two saved stacks aligned
   *  to eight bytes
   */
  uint8_t guard[8];

  /** Number of entries in the call stack */
  uint32_t callStackSize;

  /** Number of entries in the address stack */
  uint32_t addressStackSize;

  /** The saved call and address stacks.  The call stack is first
   *  and has size 16 * callStackSize.  The address stack is next and
   *  has size 8 * addressStackSize
   */
  uint8_t stacks[];
} VmStateBlock;

/** Functions for working with blocks */
uint8_t getVmmBlockType(const HeapBlock* block);
uint64_t getVmmBlockSize(const HeapBlock* block);
int vmmBlockIsMarked(const HeapBlock* block);
void clearVmmBlockMark(HeapBlock* block);
void setVmmBlockMark(HeapBlock* block);

/** Block type constants */
#ifdef __cplusplus
const int VmmFreeBlockType = 0;
const int VmmCodeBlockType = 1;
const int VmmStateBlockType = 2;
#else
const int VmmFreeBlockType;
const int VmmCodeBlockType;
const int VmmStateBlockType;
#endif

/** Memory for the virtual machine */
typedef struct VmMemoryImpl_* VmMemory;

/** Create an new VmMemory instance
 *
 *  Arguments:
 *    initialSize    Initial size of the memory, in bytes
 *    maxSize        Maximum size of the memory, in bytes
 *
 *  Returns:
 *    A new VmMemory instance, or NULL if an error occurred
 */
VmMemory createVmMemory(uint64_t initialSize, uint64_t maxSize);

/** Destroy a VmMemory instance and deallocate the memory it occupies */
void destroyVmMemory(VmMemory memory);

/** Return the error code for the last operation or 0 if that op succeeded */
int getVmmStatus(VmMemory memory);

/** Return an error message for the last operation or 0 if that op succeeded */
const char* getVmmStatusMsg(VmMemory memory);

/** Clear the last error and reset status to 0/OK */
void clearVmmStatus(VmMemory memory);

/** Return the current size of the memory, in bytes */
uint64_t currentVmmSize(VmMemory memory);

/** Return the maximum size of the memory, in bytes */
uint64_t maxVmmSize(VmMemory memory);

/** Return the number of bytes free on the heap */
uint64_t vmmBytesFree(VmMemory memory);

/** Return the current heap size, in bytes */
uint64_t vmmHeapSize(VmMemory memory);

/** Reserve "size" bytes in the memory for the program.
 *
 *  Because the Unlambda VM creates new functions while the program runs,
 *  the program and the heap need to occupy the same address space.  This
 *  function reserves "size" bytes of memory starting at address zero for
 *  the program.  The heap starts at the next eight-byte boundary after
 *  the end of the memory reserved for the program.
 *
 *  This function may be called multiple times to change the size of the
 *  program, so long as the heap is empty (has no blocks, even free ones).
 *  Once a block has been allocated, this call will fail with
 *  VmMemoryInvalidStateError.
 *
 *  Arguments:
 *    size   Number of bytes to reserve for the program
 *
 *  Returns:
 *    0 if successful, nonzero if an error occurred.  Use getVmmStatus() or
 *    getVmmStatusMsg() to obtain a specific error code or message describing
 *    the failure.
 */
int reserveVmMemoryForProgram(VmMemory memory, uint64_t size);

/** Return the size of the area reserved for the program, in bytes */
uint64_t getVmmProgramMemorySize(VmMemory memory);

/** Return a pointer to the first byte of memory reserved for the program */
uint8_t* getProgramStartInVmm(VmMemory memory);

/** Return a pointer to the start of the heap */
uint8_t* getVmmHeapStart(VmMemory memory);

/** Return a pointer to the start of the virtual machine memory */
uint8_t* ptrToVmMemory(VmMemory memory);

/** Return a pointer to the end of the virtual machine memory */
uint8_t* ptrToVmMemoryEnd(VmMemory memory);

/** Return nonzero if the address is valid, 0 if it is not valid */
int isValidVmmAddress(VmMemory memory, uint64_t address);

/** Obtain a pointer to the given address
 *
 *  Returns NULL if address is not valid, but does not change the error status
 */
uint8_t* ptrToVmmAddress(VmMemory memory, uint64_t address);

/** Compute the address for the given pointer
 *
 *  Returns a value equal to maxVmmSize(memory) if "p" is NULL or out-of-range.
 */
uint64_t vmmAddressForPtr(VmMemory memory, const uint8_t* p);

/** Return the first block on the heap or NULL if no blocks have been created */
HeapBlock* firstHeapBlockInVmm(VmMemory memory);

/** Return the next block after "block" or NULL if "block" is the last one */
HeapBlock* nextHeapBlockInVmm(VmMemory memory, HeapBlock* block);

/** Execute an operation on each block on the heap
 *
 *  Arguments:
 *    memory   Memory whose blocks should be iterated over
 *    f        Function to call on each block.  A non-NULL result will
 *                 terminate execution of the loop and return that value.
 *
 *  Returns:
 *    The value returned by the last call to f.
 */
HeapBlock* forEachVmmBlock(
    VmMemory memory,
    HeapBlock* (*f)(VmMemory, HeapBlock*)
);

/** Return the first free block on the heap, or NULL if no free blocks */
FreeBlock* firstFreeBlockInVmm(VmMemory memory);

/** Return the next free block after "block" or NULL if "block" is the last
 *  free block
 */
FreeBlock* nextFreeBlockInVmm(VmMemory memory, FreeBlock* block);

/** Execute an operation on each free block in the heap
 *
 *  Arguments:
 *    memory   The memory whose free blocks should be iterated over
 *    f        Function to call on each new block.  A non-NULL return value
 *                 will terminate the loop and return that value.
 *
 *  Returns:
 *    The value returned by the last call to f.
 */
FreeBlock* forEachFreeBlockInVmm(VmMemory memory,
				 FreeBlock* (*f)(VmMemory, FreeBlock*));

/** Allocate a block for code
 *
 *  Arguments:
 *    memory   The memory to allocate from
 *    size     The desired size of the block, in bytes
 *
 *  Returns:
 *    A pointer to the allocated CodeBlock, or NULL if a block could not
 *    be allocated.  The block will have at least "size" bytes of memory
 *    allocated for it, not including the header.  
 */
CodeBlock* allocateVmmCodeBlock(VmMemory memory, uint64_t size);

/** Allocate a block to store the VM state
 *
 *  Arguments:
 *    memory             The memory to allocate from
 *    callStackSize      The number of entries on the call stack
 *    addressStackSize   The number of entries on the address stack
 *
 *  Returns:
 *    A pointer to the allocated StateBlock, or NULL if a block could not
 *    be allocated.  The block will be big enough to store two stacks
 *    of the indicated sizes.
 */
VmStateBlock* allocateVmmStateBlock(VmMemory memory, uint32_t callStackSize,
				    uint32_t addressStackSize);

/** Error handler the garbage collector calls when it encounters an error
 *  such as a pointer to a free block.
 *
 *  The arguments are (in order):
 *  * The VmMemory instance on which garbage collection is running
 *  * The address in VmMemory of the block where the error occurred
 *  * A pointer to the HeapBlock at that address
 *  * A message describing the error
 */
typedef void (*GcErrorHandler)(VmMemory, uint64_t, HeapBlock*,
			       const char*);

/** Collect all unreachable blocks and return them to the heap
 *
 *  The current garbage collector is a simple mark/sweep algorithm that
 *  finds all blocks reachable from either the call stack or the address
 *  stack and then transforms all other blocks on the heap into free
 *  blocks, coalescing neighboring free blocks into single free blocks
 *  as it goes.  The collector is not compacting and it's implementation is
 *  not particularly suited to Unlambda's memory usage patterns, which
 *  allocate a lot of small blocks on the heap.  However, the current
 *  implementation is "good enough" for a proof of concept.
 *
 *  Arguments:
 *    memory:
 *      The VmMemory whose unreachable blocks should be collected
 *
 *    callStack:
 *      The virtual machine's call stack.
 *
 *    addressStack:
 *      The virtual machine's address stack.
 *
 *    errorHandler:
 *      A function the garbage collector will call when it encounters an
 *      error.
 *
 *  Returns:
 *    0 if collection was successful or a nonzero value if collection failed
 *    for some reason.  The current implementation always returns 0, but
 *    callers should not depend on this behavior in the future, since the
 *    GC algorithm may change.
 *
 *  TODO:
 *    Allow for multiple GC algorithms
 */
int collectUnreachableVmmBlocks(VmMemory memory, Stack callStack,
				Stack addressStack,
				GcErrorHandler errorHandler);

/** Increase the size of the memory, up to its maximum size
 *
 *  This function is typically called after an allocation on the heap fails
 *  and garbage collection fails to allocate enough memory.  Note that
 *  this function may move the VM memory around in the emulator's own heap,
 *  so all pointers into the VM memory will be invalid after this function
 *  completes if it succeeds.
 *
 *  Arguments:
 *    memory   The memory whose size should be increased
 *
 *  Returns:
 *    0 if successful, nonzero if an error occurred.  Use getVmmStatus() or
 *    getVmmStatusMsg() to obtain a specific error code or message describing
 *    the failure.
 */
int increaseVmmSize(VmMemory memory);

/** Error codes */

#ifdef __cplusplus

/** One of the arguments to a function was invalid */
const int VmmInvalidArgumentError = -1;

/** An invalid block was passed to a function.
 *
 *  For this error, the pointer passed to the function lies within the
 *  VmMemory instance, but does not correspond to the start of a block.
 */
const int VmmBadBlockError = -2;

/** The VmMemory has reached its maximum size */
const int VmmMaxSizeExceededError = -3;

/** Could not allocate enough memory to increase the VmMemory's size */
const int VmmSizeIncreaseFailedError = -4;

/** Not enough memory to allocate a block or the requested program size */
const int VmmNotEnoughMemoryError = -5;

/** Cannot change the area allocated for the program because the heap is
 *  in use.
 */
const int VmmHeapInUseError = -6;

#else

/** One of the arguments to a function was invalid */
const int VmmInvalidArgumentError;

/** An invalid block was passed to a function.
 *
 *  For this error, the pointer passed to the function lies within the
 *  VmMemory instance, but does not correspond to the start of a block.
 */
const int VmmBadBlockError;

/** The VmMemory has reached its maximum size */
const int VmmMaxSizeExceededError;

/** Could not allocate enough memory to increase the VmMemory's size */
const int VmmSizeIncreaseFailedError;

/** Not enough memory to allocate a block or the requested program size */
const int VmmNotEnoughMemoryError;

/** Cannot change the area allocated for the program because the heap is
 *  in use.
 */
const int VmmHeapInUseError;

#endif

#endif
