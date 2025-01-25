extern "C" {
#include <vm_instructions.h>
}

#include "testing_utils.hpp"
#include <assert.h>
#include <iomanip>
#include <sstream>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
/** Not part of the VmMemory public API.  Used only for testing */
void setVmmFreeList(VmMemory memory, uint64_t addrOfFirstFreeBlock,
		    uint64_t bytesFree);
}

namespace unl_test = unlambda::testing;

namespace internal {
  std::string toString(Stack s) {
    std::ostringstream msg;
    msg << "[";
    for (const uint64_t* p = reinterpret_cast<uint64_t*>(bottomOfStack(s));
	 p != reinterpret_cast<const uint64_t*>(topOfStack(s));
	 ++p) {
      if (p != reinterpret_cast<const uint64_t*>(bottomOfStack(s))) {
	msg << ",";
      }
      msg << " " << *p;
    }
    msg << " ]";
    return msg.str();
  }
}

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
    } else {
      uint8_t* data = reinterpret_cast<uint8_t*>(hp) + sizeof(HeapBlock);
      ::memset(data, 0, block.blockSize);
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

::testing::AssertionResult unl_test::pushOntoStack(Stack s,
						   const uint64_t* data,
						   uint64_t size) {
  for (uint64_t i = 0; i < size; ++i) {
    if (pushStack(s, (const void*)&data[i], sizeof(uint64_t))) {
      return ::testing::AssertionFailure()
	<< "Failed to push data[" << i << "] = " << data[i]
	<< "into the stack (" << std::string(getStackStatusMsg(s)) << ")";
    }
  }

  return ::testing::AssertionSuccess();
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

::testing::AssertionResult unl_test::verifyStack(
  const std::string& name, Stack s, const uint64_t* trueData,
  uint64_t trueSize
) {
  if (stackSize(s) != (8 * trueSize)) {
    return ::testing::AssertionFailure()
      << "The size of the " << name << " is incorrect.  It is "
      << stackSize(s) << ", but it should be " << (8 * trueSize);
  }

  const uint64_t* bottom = reinterpret_cast<const uint64_t*>(bottomOfStack(s));
  for (int i = 0; i < trueSize; ++i) {
    if (bottom[i] != trueData[i]) {
      return ::testing::AssertionFailure()
	<< "Content of " << name << " is incorrect.  It is "
	<< ::internal::toString(s)  << ", but it should be "
	<< unl_test::toString(trueData, trueSize);
    }
  }

  return ::testing::AssertionSuccess();
}

::testing::AssertionResult unl_test::verifyStateBlock(
  const uint8_t* p, const uint64_t* trueCallStackData,
  const uint64_t trueCallStackSize, const uint64_t* trueAddressStackData,
  const uint64_t trueAddressStackSize
) {
  const VmStateBlock* sb =
    reinterpret_cast<const VmStateBlock*>(p - sizeof(HeapBlock));

  // Verify the guard contains PANIC instructions
  for (int i = 0; i < sizeof(sb->guard); ++i) {
    if (sb->guard[i] != PANIC_INSTRUCTION) {
      return ::testing::AssertionFailure()
	<< "The guard is " << unl_test::toString(sb->guard, sizeof(sb->guard))
	<< ", but it should consist entirely of "
	<< (uint32_t)PANIC_INSTRUCTION << " bytes";
    }
  }

  // Verify the call stack size and content
  if (sb->callStackSize != trueCallStackSize) {
    return ::testing::AssertionFailure()
      << "The call stack has size " << sb->callStackSize
      << ", but it should have size " << trueCallStackSize;
  }

  const uint64_t* callStackData = reinterpret_cast<const uint64_t*>(sb->stacks);
  if (::memcmp(callStackData, trueCallStackData, 16 * trueCallStackSize)) {
    return ::testing::AssertionFailure()
      << "The saved call stack is "
      << unl_test::toString(callStackData, sb->callStackSize)
      << ", but it should be "
      << unl_test::toString(trueCallStackData, trueCallStackSize);
  }
  
  // Verify the address stack size and content
  if (sb->addressStackSize != trueAddressStackSize) {
    return ::testing::AssertionFailure()
      << "The address stack has size " << sb->addressStackSize
      << ", but it should have size " << trueAddressStackSize;
  }

  const uint64_t* addrStackData =
    reinterpret_cast<const uint64_t*>(sb->stacks + 16 * sb->callStackSize);
  if (::memcmp(addrStackData, trueAddressStackData, 8 * trueAddressStackSize)) {
    return ::testing::AssertionFailure()
      << "The saved address stack is "
      << unl_test::toString(addrStackData, sb->addressStackSize)
      << ", but it should be "
      << unl_test::toString(trueAddressStackData, trueAddressStackSize);
  }

  return ::testing::AssertionSuccess();
}

::testing::AssertionResult unl_test::verifyArray(Array a,
						 const uint8_t* trueData,
						 const uint64_t trueSize) {
  if (arraySize(a) != trueSize) {
    return ::testing::AssertionFailure()
      << "Array has size " << arraySize(a) << ", but it should have size "
      << trueSize;
  }

  if (memcmp(startOfArray(a), trueData, trueSize)) {
    return ::testing::AssertionFailure()
      << "Array is " << unl_test::toString(startOfArray(a), arraySize(a))
      << ", but it should be " << unl_test::toString(trueData, trueSize);
  }

  return ::testing::AssertionSuccess();
}

static std::string dumpBytesInMemory(const uint8_t* const data,
				     const uint8_t* const endOfData,
				     const uint8_t* target) {
  const uint8_t* start = std::max(data, target - 17);
  const uint8_t* end = std::min(endOfData, target + 17);
  std::ostringstream dataStr;

  dataStr << "[";
  if (start > data) {
    dataStr << " ... ";
  }
  for (const uint8_t* pp = start; pp != end; ++pp) {
    if (pp == target) {
      dataStr << "**";
    }
    dataStr << " " << std::hex << std::setw(2) << std::setfill('0')
	    << (uint32_t)*pp;
  }
  if (end < endOfData) {
    dataStr << " ...";
  }
  dataStr << " ]";
  return dataStr.str();
}

::testing::AssertionResult unl_test::verifyBytes(const uint8_t* data,
						 const uint8_t* trueData,
						 const uint64_t trueSize) {
  const uint8_t* const endOfData = data + trueSize;
  for (const uint8_t* p = data, *q = trueData; p != endOfData; ++p, ++q) {
    if (*p != *q) {
      uint64_t offset = p - data;

      return ::testing::AssertionFailure()
	<< "Data at offset " << offset << " is "
	<< dumpBytesInMemory(data, endOfData, p)
	<< ", but it should be "
	<< dumpBytesInMemory(trueData, trueData + trueSize, q);
    }
  }

  return ::testing::AssertionSuccess();
}

void unl_test::dumpArray(Array a) {
  std::cout << unl_test::toString(startOfArray(a), arraySize(a));
}

unl_test::TemporaryFile::TemporaryFile()
  : name_(unl_test::TemporaryFile::createFilename_()), fd_(-1), lastError_() {
}

unl_test::TemporaryFile::TemporaryFile(unl_test::TemporaryFile&& other)
  : name_(std::move(other.name_)), fd_(other.fd_),
    lastError_(std::move(other.lastError_)) {
  other.fd_ = -1;
}

ssize_t unl_test::TemporaryFile::read(void* buffer, size_t n) {
  lastError_ = std::string();
  
  if (!n) {
    return 0;
  }

  if (open_(true)) {
    return -1;
  } else {
    ssize_t nRead = ::read(fd_, buffer, n);
    if (nRead < 0) {
      std::ostringstream msg;
      msg << "Error reading from " << name() << ": " << strerror(errno);
      lastError_ = msg.str();
    }
    return nRead;
  }
}

ssize_t unl_test::TemporaryFile::write(const void* buffer, size_t n) {
  lastError_ = std::string();
  if (!n) {
    return 0;
  }

  if (open_(false)) {
    return -1;
  } else {
    ssize_t nWritten = ::write(fd_, buffer, n);
    if (nWritten < 0) {
      std::ostringstream msg;
      msg << "Error writing to " << name() << ": " << strerror(errno);
      lastError_ = msg.str();
    }
    return nWritten;
  }
}

::testing::AssertionResult unl_test::TemporaryFile::verify(
    const void* expectedData, size_t expectedSize
) {
  uint8_t* data = (uint8_t*)malloc(expectedSize + 1);
  const uint8_t* expectedBytes = (const uint8_t*)expectedData;
  if (!data) {
    close();
    return ::testing::AssertionFailure()
      << "Failed to open file " << name() << " for verification: Could not "
      << "allocate buffer to hold file contents";
  }

  ssize_t nRead = read((void*)data, expectedSize + 1);
  if (nRead < 0) {
    close();
    ::free((void*)data);
    return ::testing::AssertionFailure() << lastError();
  } else if (nRead != expectedSize) {
    close();
    ::free((void*)data);
    return ::testing::AssertionFailure()
      << "Read " << nRead << " bytes from " << name()
      << ", but expected to read " << expectedSize << " bytes";
  } else if (::memcmp(data, expectedData, expectedSize)) {
    std::ostringstream msg;
    msg << "Content of " << name() << " is incorrect.\n";
    for (size_t i = 0; i < expectedSize; i += 16) {
      msg << std::setw(8) << std::dec << i << " ";
      for (size_t j = i; (j < (i + 16)) && (j < expectedSize); ++j) {
	msg << " " << std::setw(2) << std::setfill('0') << std::hex
	    << (uint32_t)data[j];
      }
      msg << "\n         ";
      for (size_t j = i; (j < (i + 16)) && (j < expectedSize); ++j) {
	if (data[j] == expectedBytes[j]) {
	  msg << "   ";
	} else {
	  msg << " " << std::setw(2) << std::setfill('0') << std::hex
	      << (uint32_t)expectedBytes[j];
	}	  
      }
      msg << "\n";
    }
    return ::testing::AssertionFailure() << msg.str();
  }

  close();
  ::free((void*)data);
  return ::testing::AssertionSuccess();
}

void unl_test::TemporaryFile::close() {
  lastError_ = std::string();
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

unl_test::TemporaryFile& unl_test::TemporaryFile::operator=(
  unl_test::TemporaryFile&& other
) {
  if (this != &other) {
    close();
    name_ = std::move(other.name_);
    fd_ = other.fd_;
    lastError_ = std::move(other.lastError_);
    other.fd_ = -1;
  }
  return *this;
}

std::string unl_test::TemporaryFile::createFilename_() {
  char* filename = tempnam("/tmp", NULL);
  std::string result(filename);
  free((void*)filename);
  return std::move(result);
}

int unl_test::TemporaryFile::open_(bool forRead) {
  if (fd_ < 0) {
    int flags = forRead ? O_RDONLY : O_WRONLY | O_CREAT | O_EXCL;
    fd_ = ::open(name().c_str(), flags, 0666);
    if (fd_ < 0) {
      std::ostringstream msg;
      msg << "Failed to open " << name() << ": " << strerror(errno);
      lastError_ = msg.str();
      return -1;
    }
  }
  return 0;
}

void unl_test::TemporaryFile::unlink_() {
  close();
  if (::unlink(name().c_str())) {
    std::cerr << "WARNING: Failed to unlink temporary file " << name()
	      << ": " << strerror(errno) << std::endl;
  }
}
