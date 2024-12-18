#include "testing_utils.hpp"
#include <assert.h>

extern "C" {
/** Not part of the VmMemory public API.  Used only for testing */
void setVmmFreeList(VmMemory memory, uint64_t addrOfFirstFreeBlock,
		    uint64_t bytesFree);
}

namespace unl_test = unlambda::testing;

::testing::AssertionResult unl_test::verifyProgram(const std::string& context,
						   const uint8_t* program,
						   const uint8_t* truth,
						   uint64_t length) {
  for (uint64_t i = 0; i < length; ++i) {
    if (program[i] != truth[i]) {
      return ::testing::AssertionFailure()
	<< "Byte " << i << " of " << context << " is " << (uint32_t)program[i]
	<< ", but it should be " << (uint32_t)truth[i];
    }
  }

  return ::testing::AssertionSuccess();
}


template <typename ListType>
static ::testing::AssertionResult& writeList(
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
  
void unl_test::layoutBlocks(VmMemory memory,
			    std::vector<unl_test::BlockSpec>& blocks) {
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

void unl_test::fillBlock(VmMemory memory, uint64_t address, uint64_t size,
			 uint8_t value) {
  uint8_t* blockPtr = ptrToVmmAddress(memory, address + sizeof(HeapBlock));
  ::memset(blockPtr, value, size);
}

void unl_test::writeCodeBlock(uint8_t* p, uint8_t* code, size_t codeSize) {
  CodeBlock* cb = reinterpret_cast<CodeBlock*>(p);
  ::memcpy(cb->code, code, codeSize);
}

void unl_test::writeStateBlock(uint8_t* p, uint32_t callStackSize,
			       const uint64_t* callStackData,
			       uint32_t addressStackSize,
			       const uint64_t* addressStackData) {
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

void unl_test::dumpHeapBlocks(VmMemory memory) {
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
  
::testing::AssertionResult unl_test::verifyBlockStructure(
  VmMemory memory, const std::vector<unl_test::BlockSpec>& blocks
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

static bool haveMoreFreeBlocks(VmMemory memory, FreeBlock* const p) {
  uint8_t* const bp = reinterpret_cast<uint8_t*>(p);
  uint64_t const addr = vmmAddressForPtr(memory, bp);
  return p && isValidVmmAddress(memory, addr);
}

static bool freeBlockListIsCircular(VmMemory memory) {
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
  
::testing::AssertionResult unl_test::verifyFreeBlockList(
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

void unl_test::handleCollectorError(VmMemory memory, uint64_t address,
				    HeapBlock* block, const char* details,
				    void* errorListPtr) {
  std::ostringstream msg;
  msg << "GC error at " << address << ", block = " << block
      << " (" << details << ")";
  ((std::vector<std::string>*)errorListPtr)->push_back(msg.str());
}

::testing::AssertionResult unl_test::assertPushAddress(Stack stack,
						       uint64_t address) {
  if (pushStack(stack, &address, 8)) {
    return ::testing::AssertionFailure()
      << "Failed to push " << address << " onto stack [code="
      << getStackStatus(stack) << "] (" << getStackStatusMsg(stack)
      << ")";
  }
  return ::testing::AssertionSuccess();
}





