extern "C" {
#include <vm_instructions.h>
#include <vmmem.h>

/** Not part of the VmMemory public API.  Used only for testing */
void setVmmFreeList(VmMemory memory, uint64_t addrOfFirstFreeBlock,
		    uint64_t bytesFree);
}

#include <gtest/gtest.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

namespace {
  struct BlockSpec {
    uint8_t blockType;
    uint64_t blockSize;
    uint64_t address;

    BlockSpec(uint8_t type, uint64_t size, uint64_t addr = 0):
      blockType(type), blockSize(size), address(addr) {
    }
  };

  template <typename ListType>
  ::testing::AssertionResult& writeList(
    ::testing::AssertionResult& out, const ListType& list,
    bool addElipsis = false
  ) {
    out << "[ ";
    for (auto i = list.begin(); i != list.end(); ++i) {
      if (i != list.begin()) {
	out << ", ";
      }
      out << *i;
    }
    if (addElipsis) {
      out << "...";
    }
    out << " ]";
    return out;
  }
  
  void layoutBlocks(VmMemory memory, std::vector<BlockSpec>& blocks) {
    uint8_t* const heapStart = getVmmHeapStart(memory);
    uint8_t* const endOfMemory = ptrToVmMemoryEnd(memory);
    uint8_t* p = heapStart;
    FreeBlock* firstFreeBlock = NULL;
    FreeBlock* lastFreeBlock = NULL;
    uint64_t bytesFree = 0;
      
    for (auto& block : blocks) {
      HeapBlock* hp = reinterpret_cast<HeapBlock*>(p);
      hp->typeAndSize = ((uint64_t)block.blockType << 56) | block.blockSize;
      if (block.blockType == VmmFreeBlockType) {
	if (!firstFreeBlock) {
	  firstFreeBlock = reinterpret_cast<FreeBlock*>(p);
	}
	if (lastFreeBlock) {
	  lastFreeBlock->next = vmmAddressForPtr(memory, p);
	}
	lastFreeBlock = reinterpret_cast<FreeBlock*>(p);
	lastFreeBlock->next = 0;
	bytesFree += block.blockSize;
      }
      block.address = vmmAddressForPtr(memory, p);
      p += block.blockSize + sizeof(HeapBlock);
    }

    assert(p == endOfMemory);
    if (firstFreeBlock) {
      const uint64_t firstFreeAddr = vmmAddressForPtr(
	memory, reinterpret_cast<uint8_t*>(firstFreeBlock)
      );
      setVmmFreeList(memory, firstFreeAddr, bytesFree);
    } else {
      assert(bytesFree == 0);
      setVmmFreeList(memory, 0, 0);
    }
  }

  void writeCodeBlock(uint8_t* p, uint8_t* code, size_t codeSize) {
    CodeBlock* cb = reinterpret_cast<CodeBlock*>(p);
    ::memcpy(cb->code, code, codeSize);
  }

  void writeStateBlock(uint8_t* p, uint32_t callStackSize,
		       uint64_t* callStackData, uint32_t addressStackSize,
		       uint64_t* addressStackData) {
    VmStateBlock* sbp = reinterpret_cast<VmStateBlock*>(p);

    for (int i = 0; i < 8; ++i) {
      sbp->guard[i] = PANIC_INSTRUCTION;
    }
    sbp->callStackSize = callStackSize;
    sbp->addressStackSize = addressStackSize;

    ::memcpy(sbp->stacks, callStackData, 16 * callStackSize);
    ::memcpy(sbp->stacks + 16 * callStackSize, addressStackData,
	     8 * addressStackSize);
  }

  void dumpHeapBlocks(VmMemory memory) {
    uint8_t* const endOfMemory = ptrToVmMemoryEnd(memory);
    uint8_t* const heapStart = getVmmHeapStart(memory);
    uint8_t* p = heapStart;

    while ((p >= heapStart) && (p < endOfMemory)) {
      const HeapBlock* bp = reinterpret_cast<const HeapBlock*>(p);
      const uint8_t blockType = getVmmBlockType(bp);
      const uint64_t blockSize = getVmmBlockSize(bp);

      std::cout << vmmAddressForPtr(memory, p) << " " << blockSize << " ";

      if (blockType == VmmFreeBlockType) {
	std::cout << "FREE next=" << reinterpret_cast<const FreeBlock*>(p)->next
		  << std::endl;
      } else if (blockType == VmmCodeBlockType) {
	std::cout << "CODE" << std::endl;
      } else if (blockType == VmmStateBlockType) {
	const VmStateBlock* sbp = reinterpret_cast<const VmStateBlock*>(p);
	std::cout << "STATE cs=" << sbp->callStackSize << ", as="
		  << sbp->addressStackSize << std::endl;
      } else {
	std::cout << "UNKNOWN (" << blockType << ")" << std::endl;
      }

      p += blockSize + sizeof(HeapBlock);
    }
  }
  
  ::testing::AssertionResult verifyBlockStructure(
    VmMemory memory, const std::vector<BlockSpec>& blocks
  ) {
    uint8_t* const endOfMemory = ptrToVmMemoryEnd(memory);
    uint8_t* p = getVmmHeapStart(memory);
    uint64_t cnt = 0;

    for (auto& block : blocks) {
      HeapBlock* bp = reinterpret_cast<HeapBlock*>(p);

      if (vmmAddressForPtr(memory, p) != block.address) {
	return ::testing::AssertionFailure()
	  << "Block with index " << cnt << " is at address "
	  << vmmAddressForPtr(memory, p) << ", but it should be at address "
	  << block.address;
      }

      if (getVmmBlockType(bp) != block.blockType) {
	return ::testing::AssertionFailure()
	  << "Block at address " << vmmAddressForPtr(memory, p)
	  << " has incorrect type " << (uint32_t)getVmmBlockType(bp)
	  << ".  It should have type " << (uint32_t)block.blockType;
      }

      if (getVmmBlockSize(bp) != block.blockSize) {
	return ::testing::AssertionFailure()
	  << "Block at address " << vmmAddressForPtr(memory, p)
	  << " has incrrect size " << getVmmBlockSize(bp)
	  << ".  It should have size " << block.blockSize;
      }

      p += getVmmBlockSize(bp) + sizeof(HeapBlock);
      ++cnt;
    }

    if (p != endOfMemory) {
      return ::testing::AssertionFailure()
	<< "Heap blocks do not cover heap.  Blocks end at address "
	<< vmmAddressForPtr(memory, p) << ", but the heap ends at "
	<< vmmAddressForPtr(memory, endOfMemory) << ", a difference of "
	<< (vmmAddressForPtr(memory, endOfMemory) - vmmAddressForPtr(memory, p))
	<< " bytes.";
    }

    return ::testing::AssertionSuccess();
  }

  bool haveMoreFreeBlocks(VmMemory memory, FreeBlock* const p) {
    uint8_t* const bp = reinterpret_cast<uint8_t*>(p);
    uint64_t const addr = vmmAddressForPtr(memory, bp);
    return p && isValidVmmAddress(memory, addr);
  }

  bool freeBlockListIsCircular(VmMemory memory) {
    FreeBlock *p1 = firstFreeBlockInVmm(memory);
    FreeBlock *p2 = p1;
    while (p1 && p2) {
      // std::cout << "freeBlockListIsCircular: p1 = "
      // 		<< vmmAddressForPtr(memory, reinterpret_cast<uint8_t*>(p1))
      // 		<< ", p2 = "
      // 		<< vmmAddressForPtr(memory, reinterpret_cast<uint8_t*>(p2))
      // 		<< std::endl;
      p1 = nextFreeBlockInVmm(memory, p1);
      p2 = nextFreeBlockInVmm(memory, p2);
      if (!p2) {
	break;
      }
      p2 = nextFreeBlockInVmm(memory, p2);
      assert(p1 || p2);
      if (p1 == p2) {
	// std::cout << "freeBlockListIsCircular: returning true" << std::endl;
	return true;
      }
    }
    // std::cout << "freeBlockListIsCircular: returning false" << std::endl;
    return false;
  }
  
  ::testing::AssertionResult verifyFreeBlockList(
    VmMemory memory, const std::vector<uint64_t>& truth
  ) {
    std::vector<uint64_t> addresses;
    FreeBlock* pb = firstFreeBlockInVmm(memory);

    // If the free list is circular, stop here...
    if (freeBlockListIsCircular(memory)) {
      return ::testing::AssertionFailure() << "Free block list is circular";
    }

    // Only add as much to "addresses" as there is in "truth" to prevent
    // a malformed free block list from building a really long list.
    while (haveMoreFreeBlocks(memory, pb)
	     && (addresses.size() < truth.size())) {
      uint64_t addr = vmmAddressForPtr(memory, reinterpret_cast<uint8_t*>(pb));
      addresses.push_back(addr);
      if (pb->next) {
	pb = reinterpret_cast<FreeBlock*>(ptrToVmMemory(memory) + pb->next);
      } else {
	pb = NULL;
      }
    }

    if (addresses != truth) {
      auto result = ::testing::AssertionFailure();
      result << "Free list is incorrect (";
      writeList(result, addresses, pb != NULL);
      result << " vs. ";
      writeList(result, truth);
      result << ")";
      return result;
    }

    if (pb != NULL) {
      uint64_t addr = vmmAddressForPtr(memory, reinterpret_cast<uint8_t*>(pb));
      return ::testing::AssertionFailure()
	<< "Free list terminated incorrectly -- \"next\" points to "
	<< addr << " instead of 0";
    }
    
    return ::testing::AssertionSuccess();
  }
}

TEST(vmmem_tests, createVmMemory) {
  VmMemory memory = createVmMemory(1024, 2048);

  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 2048);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - sizeof(HeapBlock));
  EXPECT_EQ(vmmHeapSize(memory), 1024);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 0);
  EXPECT_EQ(getProgramStartInVmm(memory), (void*)0);

  /** Should have one free block at the start of memory */
  HeapBlock* p = (HeapBlock*)ptrToVmMemory(memory);
  EXPECT_EQ(getVmmBlockType(p), VmmFreeBlockType);
  EXPECT_EQ(getVmmBlockSize(p), 1024 - sizeof(HeapBlock));
  EXPECT_FALSE(vmmBlockIsMarked(p));
  
  destroyVmMemory(memory);
}

TEST(vmmem_tests, reserveMemoryForProgram) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(reserveVmMemoryForProgram(memory, 512), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmBytesFree(memory), 512 - sizeof(HeapBlock));
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  /** Should have one free block at the start of the heap */
  HeapBlock* p = (HeapBlock*)getVmmHeapStart(memory);
  EXPECT_EQ(getVmmBlockType(p), VmmFreeBlockType);
  EXPECT_EQ(getVmmBlockSize(p), 512 - sizeof(HeapBlock));
  EXPECT_FALSE(vmmBlockIsMarked(p));
  
  destroyVmMemory(memory);
}

TEST(vmmem_tests, reserveVmMemoryForProgramIncreasingMemorySize) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(reserveVmMemoryForProgram(memory, 3072), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  
  EXPECT_EQ(currentVmmSize(memory), 4096);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - sizeof(HeapBlock));
  EXPECT_EQ(vmmHeapSize(memory), 1024);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 3072);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 3072);

  /** Should have one free block at the start of the heap */
  HeapBlock* p = (HeapBlock*)getVmmHeapStart(memory);
  EXPECT_EQ(getVmmBlockType(p), VmmFreeBlockType);
  EXPECT_EQ(getVmmBlockSize(p), 1024 - sizeof(HeapBlock));
  EXPECT_FALSE(vmmBlockIsMarked(p));
  
  destroyVmMemory(memory);
}

TEST(vmmem_tests, reserveAllVmMemoryForProgram) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(reserveVmMemoryForProgram(memory, 4096), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  
  EXPECT_EQ(currentVmmSize(memory), 4096);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmBytesFree(memory), 0);
  EXPECT_EQ(vmmHeapSize(memory), 0);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 4096);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 4096);

  // All memory used for the program.  The heap has size zero and therefore
  // no free blocks.
  
  destroyVmMemory(memory);
}

TEST(vmmem_tests, reserveTooMuchMemoryForProgram) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_NE(reserveVmMemoryForProgram(memory, 8192), 0);
  EXPECT_EQ(getVmmStatus(memory), VmmNotEnoughMemoryError);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)),
	    "Cannot allocate 8192 bytes for the program in a memory of "
	    "size 4096");

  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmBytesFree(memory), 4096 - sizeof(HeapBlock));
  EXPECT_EQ(vmmHeapSize(memory), 4096);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 0);
  EXPECT_EQ(getProgramStartInVmm(memory), (void*)0);
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 0);

  // Should be one free block at the start of the heap
  HeapBlock* p = (HeapBlock*)getVmmHeapStart(memory);
  EXPECT_EQ(getVmmBlockType(p), VmmFreeBlockType);
  EXPECT_EQ(getVmmBlockSize(p), 4096 - sizeof(HeapBlock));
  EXPECT_FALSE(vmmBlockIsMarked(p));
  
  destroyVmMemory(memory);
}

/** Use the allocateCodeBlock* tests to verify the block allocator behaves
 *  correctly under different conditions.
 */
TEST(vmmem_tests, allocateCodeBlockFromNewHeap) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  CodeBlock* cb = allocateVmmCodeBlock(memory, 128 - sizeof(HeapBlock));
  EXPECT_NE(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 512 - 128 - sizeof(HeapBlock));
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb), getVmmHeapStart(memory));

  /** Memory should be laid out like this:
   *  * Program area (512 bytes)
   *  * Code block (128 bytes, including header)
   *  * Free block (384 bytes, including header)
   */
  std::vector<BlockSpec> trueStructure{
    BlockSpec(VmmCodeBlockType, 128 - sizeof(HeapBlock), 512),
    BlockSpec(VmmFreeBlockType, 384 - sizeof(HeapBlock), 640),
  };  
  EXPECT_TRUE(verifyBlockStructure(memory, trueStructure));

  destroyVmMemory(memory);
}

TEST(vmmem_tests, allocateCodeBlockSizeNotMultipleOfEight) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  /** Should allocate a block of 56 bytes, including header */
  CodeBlock* cb = allocateVmmCodeBlock(memory, 54 - sizeof(HeapBlock));
  EXPECT_NE(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 512 - 56 - sizeof(HeapBlock));
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb), getVmmHeapStart(memory));

  /** Memory should be laid out like this:
   *  * Program area (512 bytes)
   *  * Code block (56 bytes, including header)
   *  * Free block (456 bytes, including header)
   */
  std::vector<BlockSpec> trueStructure{
    BlockSpec(VmmCodeBlockType, 56 - sizeof(HeapBlock), 512),
    BlockSpec(VmmFreeBlockType, 512 - 56 - sizeof(HeapBlock), 512 + 56),
  };  
  EXPECT_TRUE(verifyBlockStructure(memory, trueStructure));

  destroyVmMemory(memory);
}

/** Allocate all the memory in a new heap as a single code block */
TEST(vmmem_tests, allocateEntireHeap) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  CodeBlock* cb = allocateVmmCodeBlock(memory, 512 - sizeof(HeapBlock));
  EXPECT_NE(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 0);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb), getVmmHeapStart(memory));

  /** Memory should be laid out like this:
   *  * Program area (512 bytes)
   *  * Code block (512 bytes, including header)
   */
  std::vector<BlockSpec> trueStructure{
    BlockSpec(VmmCodeBlockType, 512 - sizeof(HeapBlock), 512),
  };  
  EXPECT_TRUE(verifyBlockStructure(memory, trueStructure));

  destroyVmMemory(memory);
}

/** Allocate more memory than the heap currently has, causing the VmMemory
 *  to issue a VmmNotEnoughMemoryError.
 */
TEST(vmmem_tests, allocateTooMuch) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  CodeBlock* cb = allocateVmmCodeBlock(memory, 1024 - sizeof(HeapBlock));
  EXPECT_EQ(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), VmmNotEnoughMemoryError);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)),
	    "Could not allocate block of size 1016 (Not enough memory)");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 512 - sizeof(HeapBlock));
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  /** Memory should be laid out like this:
   *  * Program area (512 bytes)
   *  * Free block (512 bytes, including header)
   */
  std::vector<BlockSpec> trueStructure{
    BlockSpec(VmmFreeBlockType, 512 - sizeof(HeapBlock), 512),
  };  
  EXPECT_TRUE(verifyBlockStructure(memory, trueStructure));

  destroyVmMemory(memory);
}

// Allocate two consecutive blocks
TEST(vmmem_tests, allocateTwoConsecutiveBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  // Allocate two blocks, one of 64 bytes and one of 400 bytes
  CodeBlock* cb1 = allocateVmmCodeBlock(memory, 64);
  EXPECT_NE(cb1, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  CodeBlock* cb2 = allocateVmmCodeBlock(memory, 400);
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb2),
	    reinterpret_cast<uint8_t*>(cb1) + 64 + sizeof(HeapBlock));
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 48 - 3 * sizeof(HeapBlock));
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb1), getVmmHeapStart(memory));

  /** Memory should be laid out like this:
   *  * Program area (512 bytes)
   *  * Code block 1 (72 bytes, including header)
   *  * Code block 2 (408 bytes, including header)
   *  * Free block (32 bytes, including header)
   */
  const std::vector<BlockSpec> trueStructure{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmCodeBlockType, 400, 584),
    BlockSpec(VmmFreeBlockType, 24, 992),
  };  
  EXPECT_TRUE(verifyBlockStructure(memory, trueStructure));

  const std::vector<uint64_t> freeBlockAddresses{ 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyVmMemory(memory);
}

// Allocate memory by splitting the middle block in a heap with
// three free blocks on it
TEST(vmmem_tests, splitSecondOfThreeBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);
  
  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 64),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 72),
    BlockSpec(VmmCodeBlockType, 32),
    BlockSpec(VmmFreeBlockType, 120),
    BlockSpec(VmmCodeBlockType, 112),
    BlockSpec(VmmFreeBlockType, 24)
  };

  layoutBlocks(memory, blockStructure);

  CodeBlock* cb = allocateVmmCodeBlock(memory, 40);
  ASSERT_NE(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 32 + 72 + 24);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb),
	    ptrToVmMemory(memory) + blockStructure[4].address);

  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Free block (40 = 32 + 8)
  // * Code block (80 = 72 + 8)
  // * Code block (40 = 32 + 8)
  // * Code block (48 = 40 + 8)
  // * Free block (80 = 72 + 8)
  // * Code block (120 = 112 + 8)
  // * Free block (32 = 24 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmFreeBlockType, 32, 584),
    BlockSpec(VmmCodeBlockType, 72, 624),
    BlockSpec(VmmCodeBlockType, 32, 704),
    BlockSpec(VmmCodeBlockType, 40, 744),
    BlockSpec(VmmFreeBlockType, 72, 792),
    BlockSpec(VmmCodeBlockType, 112, 872),
    BlockSpec(VmmFreeBlockType, 24, 992)
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 584, 792, 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}


// Allocate memory by consuming the entire middle block in a heap with
// three free blocks on it
TEST(vmmem_tests, consumeSecondOfThreeBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);
  
  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 64),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 72),
    BlockSpec(VmmCodeBlockType, 32),
    BlockSpec(VmmFreeBlockType, 120),
    BlockSpec(VmmCodeBlockType, 112),
    BlockSpec(VmmFreeBlockType, 24)
  };

  layoutBlocks(memory, blockStructure);

  CodeBlock* cb = allocateVmmCodeBlock(memory, 110);
  ASSERT_NE(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 32 + 24);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  dumpHeapBlocks(memory);
  
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb),
	    ptrToVmMemory(memory) + blockStructure[4].address);

  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Free block (40 = 32 + 8)
  // * Code block (80 = 72 + 8)
  // * Code block (40 = 32 + 8)
  // * Code block (128 = 120 + 8)
  // * Code block (120 = 112 + 8)
  // * Free block (32 = 24 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmFreeBlockType, 32, 584),
    BlockSpec(VmmCodeBlockType, 72, 624),
    BlockSpec(VmmCodeBlockType, 32, 704),
    BlockSpec(VmmCodeBlockType, 120, 744),
    BlockSpec(VmmCodeBlockType, 112, 872),
    BlockSpec(VmmFreeBlockType, 24, 992)
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 584, 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Allocate memory by splitting the first block in a three block chain
TEST(vmmem_tests, splitFirstOfThreeBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);
  
  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 64),
    BlockSpec(VmmFreeBlockType, 48),
    BlockSpec(VmmCodeBlockType, 56),
    BlockSpec(VmmCodeBlockType, 32),
    BlockSpec(VmmFreeBlockType, 120),
    BlockSpec(VmmCodeBlockType, 112),
    BlockSpec(VmmFreeBlockType, 24)
  };

  layoutBlocks(memory, blockStructure);

  CodeBlock* cb = allocateVmmCodeBlock(memory, 1);
  ASSERT_NE(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 32 + 120 + 24);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  dumpHeapBlocks(memory);
  
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb),
	    ptrToVmMemory(memory) + blockStructure[1].address);

  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Code block (16 = 8 + 8)
  // * Free block (40 = 32 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (40 = 32 + 8)
  // * Free block (128 = 120 + 8)
  // * Code block (120 = 112 + 8)
  // * Free block (32 = 24 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmCodeBlockType,  8, 584),
    BlockSpec(VmmFreeBlockType, 32, 600),
    BlockSpec(VmmCodeBlockType, 56, 640),
    BlockSpec(VmmCodeBlockType, 32, 704),
    BlockSpec(VmmFreeBlockType, 120, 744),
    BlockSpec(VmmCodeBlockType, 112, 872),
    BlockSpec(VmmFreeBlockType, 24, 992)
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 600, 744, 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Allocate memory by consuming the entire first block in a three block chain
TEST(vmmem_tests, consumeFirstOfThreeBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);
  
  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 64),
    BlockSpec(VmmFreeBlockType, 48),
    BlockSpec(VmmCodeBlockType, 56),
    BlockSpec(VmmCodeBlockType, 32),
    BlockSpec(VmmFreeBlockType, 120),
    BlockSpec(VmmCodeBlockType, 112),
    BlockSpec(VmmFreeBlockType, 24)
  };

  layoutBlocks(memory, blockStructure);

  CodeBlock* cb = allocateVmmCodeBlock(memory, 48);
  ASSERT_NE(cb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 120 + 24);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  dumpHeapBlocks(memory);
  
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb),
	    ptrToVmMemory(memory) + blockStructure[1].address);

  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Code block (56 = 48 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (40 = 32 + 8)
  // * Free block (128 = 120 + 8)
  // * Code block (120 = 112 + 8)
  // * Free block (32 = 24 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmCodeBlockType, 48, 584),
    BlockSpec(VmmCodeBlockType, 56, 640),
    BlockSpec(VmmCodeBlockType, 32, 704),
    BlockSpec(VmmFreeBlockType, 120, 744),
    BlockSpec(VmmCodeBlockType, 112, 872),
    BlockSpec(VmmFreeBlockType, 24, 992)
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 744, 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Allocate two blocks, both from the first free block.  The first allocation
// splits the block, while the second consumes the rest of the first block
TEST(vmm_tests, allocateTwiceFromFirstBlock) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);
  
  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 64),
    BlockSpec(VmmFreeBlockType, 48),
    BlockSpec(VmmCodeBlockType, 56),
    BlockSpec(VmmCodeBlockType, 32),
    BlockSpec(VmmFreeBlockType, 120),
    BlockSpec(VmmCodeBlockType, 112),
    BlockSpec(VmmFreeBlockType, 24)
  };

  layoutBlocks(memory, blockStructure);

  // Split the second block into 8 and 32 bytes
  CodeBlock* cb1 = allocateVmmCodeBlock(memory, 8);
  ASSERT_NE(cb1, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb1),
	    ptrToVmMemory(memory) + blockStructure[1].address);

  // Consume what's left of the first block
  CodeBlock* cb2 = allocateVmmCodeBlock(memory, 32);
  ASSERT_NE(cb2, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb2),
	    ptrToVmMemory(memory) + blockStructure[1].address + 16);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 120 + 24);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  dumpHeapBlocks(memory);
  
  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Code block (16 =  8 + 8)
  // * Code block (40 = 32 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (40 = 32 + 8)
  // * Free block (128 = 120 + 8)
  // * Code block (120 = 112 + 8)
  // * Free block (32 = 24 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmCodeBlockType,  8, 584),
    BlockSpec(VmmCodeBlockType, 32, 600),
    BlockSpec(VmmCodeBlockType, 56, 640),
    BlockSpec(VmmCodeBlockType, 32, 704),
    BlockSpec(VmmFreeBlockType, 120, 744),
    BlockSpec(VmmCodeBlockType, 112, 872),
    BlockSpec(VmmFreeBlockType, 24, 992)
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 744, 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Allocate two blocks, one from the first free block and one from the
// second free block
TEST(vmm_tests, allocateFromFirstThenSecondBlock) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);
  
  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 64),
    BlockSpec(VmmFreeBlockType, 48),
    BlockSpec(VmmCodeBlockType, 56),
    BlockSpec(VmmCodeBlockType, 32),
    BlockSpec(VmmFreeBlockType, 120),
    BlockSpec(VmmCodeBlockType, 112),
    BlockSpec(VmmFreeBlockType, 24)
  };

  layoutBlocks(memory, blockStructure);

  // Allocate 32 bytes from the second block, leaving 8
  CodeBlock* cb1 = allocateVmmCodeBlock(memory, 32);
  ASSERT_NE(cb1, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb1),
	    ptrToVmMemory(memory) + blockStructure[1].address);

  // Allocate 64 bytes from the second block, leaving 48
  CodeBlock* cb2 = allocateVmmCodeBlock(memory, 64);
  ASSERT_NE(cb2, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb2),
	    ptrToVmMemory(memory) + blockStructure[4].address);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 8 + 48 + 24);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  dumpHeapBlocks(memory);
  
  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Code block (40 = 32 + 8)
  // * Free block (16 =  8 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (40 = 32 + 8)
  // * Code block (72 = 64 + 8)
  // * Free block (56 = 48 + 8)
  // * Code block (120 = 112 + 8)
  // * Free block (32 = 24 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmCodeBlockType, 32, 584),
    BlockSpec(VmmFreeBlockType,  8, 624),
    BlockSpec(VmmCodeBlockType, 56, 640),
    BlockSpec(VmmCodeBlockType, 32, 704),
    BlockSpec(VmmCodeBlockType, 64, 744),
    BlockSpec(VmmFreeBlockType, 48, 816),
    BlockSpec(VmmCodeBlockType, 112, 872),
    BlockSpec(VmmFreeBlockType, 24, 992)
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 624, 816, 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Allocate two blocks, both from the second block.  The first allocation
// splits the second free block and the second allocation consumes it

// Allocate three blocks, first from the second free block and second from
// the first free block and the third from the third free block.  Tests
// the linked list of free blocks is maintained correctly.
