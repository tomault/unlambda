#ifndef __UNLAMBDA__TESTING_UTILS_HPP__
#define __UNLAMBDA__TESTING_UTILS_HPP__

extern "C" {
#include <array.h>
#include <stack.h>
#include <vm_instructions.h>
#include <vmmem.h>
}

#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <vector>

#include <stdint.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace unlambda {
  namespace testing {
    namespace internal {
      /** Convert a uint8_t to a uint32_t but leave other types as-is
       *
       *  This function and its specialization are used by toString to
       *  convert a uint8_t to a uint32_t while leaving other data types
       *  unaltered, so values of type uint8_t don't get printed as
       *  single characters.
       */
      template <typename T>
      const T& convertUInt8ToUInt32(const T& v) { return v; }

      inline uint32_t convertUInt8ToUInt32(const uint8_t& v) { return v; }
    }
    
    /** Create a string representation for a fixed-size array
     *
     *  Arguments:
     *    data      Start of the array
     *    size      Number of items in the array
     *
     *  Returns:
     *    The contents of the array, separated by commas and eclosed within
     *    square brackets.
     *
     *  Throws:
     *    Does not throw
     */
    template <typename T>
    std::string toString(const T* data, uint64_t size) {
      std::ostringstream msg;
      const T* const end = data + size;

      msg << "[";
      for (auto p = data; p != end; ++p) {
	if (p != data) {
	  msg << ",";
	}
	msg << " " << internal::convertUInt8ToUInt32(*p);
      }
      msg << " ]";
      return msg.str();
    }

    /** Describes a block on the heap for verification */
    struct BlockSpec {
      uint8_t blockType;
      uint64_t blockSize;
      uint64_t address;

      BlockSpec(uint8_t type, uint64_t size, uint64_t addr = 0):
	blockType(type), blockSize(size), address(addr) {
      }

      bool operator==(const BlockSpec& other) const {
	return (blockType == other.blockType) && (blockSize == other.blockSize)
	         && (address == other.address);
      }

      bool operator!=(const BlockSpec& other) const {
	return (blockType != other.blockType) || (blockSize != other.blockSize)
                 || (address != other.address);
      }
    };

    /** Write a BlockSpec to a stream */
    inline std::ostream& operator<<(std::ostream& out, const BlockSpec& s) {
      return out << "BlockSpec{" << (uint32_t)s.blockType
		 << ", " << s.blockSize << ", " << s.address << "}";
    }

    /** Write blocks on the heap as described by "blocks"
     *
     *  Block sizes in "block" should be the size of the block's content
     *  and should not include the header.  The actual size of the block
     *  on the heap will be block.blockSize + sizeof(HeapBlock).
     *
     *  The blocks must cover the heap - that is the total size of the
     *  blocks, including headers, in "blocks" must be equal to the size
     *  of the heap in "memory" - or an assert will fail.
     *
     *  Does not write the content of the blocks, just their structure
     *  (headers and next free block addresses).  Sets the first free
     *  block address and number of bytes free in "memory."
     *
     *  Arguments:
     *    memory    Write blocks to the heap 
     *    blocks    Type and size of blocks to write, in the order
     *                they will be written.  Upon return, layoutBlocks()
     *                will set the "address" field of each block to where
     *                the block is located on the heap.  This will be the
     *                address of the block's header.  The data region is
     *                located at this address plus sizeof(HeapBlock).
     *
     *  Throws:
     *    Nothing, but an assert will fail if the blocks do not cover the
     *    heap (see above).
     */
    void layoutBlocks(VmMemory memory, std::vector<BlockSpec>& blocks);

    /** Fill a block with a constant value
     *
     *  Arguments:
     *    memory    Memory containing the block to fill
     *    address   Address of the block's header.  Note that although
     *                this function writes to the block's data region,
     *                it expects the address of the block's header and
     *                will adjust that address to point to the data region
     *                prior to filling it with "value."
     *    size      Number of bytes to fill, typically equal to the size
     *                of the block's data region
     *    value     Value to write
     *
     *  Throws:
     *    Nothing
     */
    void fillBlock(VmMemory memory, uint64_t address, uint64_t size,
		   uint8_t value);

    /** Write the bytecode for a code block
     *
     *  Arguments:
     *    p          Pointer to the code block's header
     *    code       Bytecode to write
     *    codeSize   How many bytes to write
     *
     *  Throws:
     *    Nothing
     */
    void writeCodeBlock(uint8_t* p, uint8_t* code, size_t codeSize);

    /** Write the data portion of a state block
     *
     *  Writes the guard and both stacks.
     *
     *  Arguments:
     *    p                  Pointer to the state block's header
     *    callStackSize      Number of frames on the call stack.
     *    callStackData      Call stack frames to write.    Each frame is
     *                         16 bytes and consists of the address of the
     *                         block that was called into followed by the
     *                         return address.  Frames are listed from
     *                         bottom to top of the stack.
     *    addressStackSize   Number of addresses on the address stack
     *    addressStackData   Contents of the address stack, from bottom to
     *                         top of the stack.
     */
    void writeStateBlock(uint8_t* p, uint32_t callStackSize,
			 const uint64_t* callStackData,
			 uint32_t addressStackSize,
			 const uint64_t* addressStackData);

    /** Push "size" 8-byte values from "data" onto the stack "s", checking
     *  for errors as each value is pushed
     *
     *  Arguments:
     *    s     Stack to push values onto
     *    data  Values to push, in order
     *    size  Number of values to push
     *
     *  Returns:
     *    ::testing::AssertionSuccess() if all values were pushed onto the stack
     *    ::testing::AssertionFailure() if an error occurred
     *  Throws:
     *    Nothing
     */
    ::testing::AssertionResult pushOntoStack(Stack s, const uint64_t* data,
					     uint64_t size);

    /** Dump the blocks on the heap to standard output
     *
     *  Writes the blocks in ascending order by address, one line per block.
     *  Writes the address, size and type for each block in that order.
     *  For free blocks, writes the next block address, and for state blocks,
     *  writes the sizes of the call and data stacks.  Does not write anything
     *  extra for code blocks.
     *
     *  Arguments:
     *    memory  Dump the block's on the heap of this VmMemory
     *
     *  Throws:
     *    Nothing
     */
    void dumpHeapBlocks(VmMemory memory);

    /** Verify that the block structure of "memory"'s heap matches the
     *    expected structure in "blocks"
     *
     *  Verifies that the address, type and size of each block on the heap
     *  matches the address, type and size of the corresponding block in
     *  "blocks."  Will return an AssertionFailure if "blocks" does not cover
     *  the heap.
     *
     *  Note that block sizes in "blocks" are the size of the block's
     *  data region and do not include the header.
     *
     *  Arguments:
     *    memory   Contains the heap to check
     *    blocks   Expected structure of the heap.  Blocks should be listed
     *               in ascending order by address and must cover the
     *               entire heap, or verifyBlockStructure() will return
     *               an AssertionFailure.
     *  
     *  Returns:
     *    ::testing::AssertionSuccess if the address, type and size of
     *      each block matches the corresponding block in "blocks" and
     *      "blocks" covers the entire heap.
     *   ::testing::AssertionFailure otherwise.
     *
     *  Throws:
     *    Nothing
     */
    ::testing::AssertionResult verifyBlockStructure(
      VmMemory memory, const std::vector<BlockSpec>& blocks
    );

    /** Verify that the free block list of "memory" is not circular and
     *    contains free blocks at the specified addresses
     *
     *  The "truth" argument should contain addresses of the block's headers.
     *  Verifies that the free block list contains the same number of
     *  blocks as "truth," has free blocks located at the addresses listed
     *  in "truth," and the free block list is not circular.
     *
     *  Arguments:
     *    memory  Check this VmMemory's free block list
     *    truth   Number and addresses of free blocks
     *
     *  Returns:
     *    ::testing::AssertionSuccess() if "memory"'s free list matches
     *       truth and ::testing::AssertionFailure() if it does not match.
     *
     *  Throws:
     *    Nothing
     */
    ::testing::AssertionResult verifyFreeBlockList(
      VmMemory memory, const std::vector<uint64_t>& truth
    );

    /** Implementation of GcErrorHandler that transforms the GC error
     *    information into a diagnostic error message and appends that
     *    message to the std::vector<string> "errorListPtr" points to
     *
     *  Used to collect diagnostic error messages from VmMemory's garbage
     *  collector during a call to collectUnreachableVmmBlocks().  Because
     *  collectUnreachableVmmBlocks() is a C function, the std::vector<string>
     *  that collects the error messages is passed through an untyped void*.
     *
     *  Arguments:
     *    memory         VmMemory undergoing garbage collection
     *    address        Address at which the error occurred
     *    block          Block containng the error
     *    details        A brief message describing the error
     *    errorListPtr   Pointer to a std::vector<std::string> to which
     *                     diagnostic error messages will be appended.
     *
     *  Throws:
     *    Nothing
     */
    void handleCollectorError(VmMemory memory, uint64_t address,
			      HeapBlock* block, const char* details,
			      void* errorListPtr);

    /** Pushes a 8-byte address onto a Stack and verifies that address
     *    was pushed successfully.
     *
     *
     *  Arguments:
     *    stack    Where to push the address
     *    address  Address to push
     *
     *  Returns:
     *    ::testing::AssertionSuccess if "address" was successfully pushed
     *    onto "stack" and ::testing::AssertionFailure if "address" could
     *    not be pushed onto "stack."
     *
     *  Throws:
     *    Nothing
     */
    ::testing::AssertionResult assertPushAddress(Stack stack,
						 uint64_t address);
    
    /** Verify the bytecode of a program is correct
     *
     *  Arguments:
     *    context    Brief, human-readable name for the error message that
     *                 identifies the check being performed
     *    program    The bytecote to check
     *    truth      What that bytecode should be
     *    length     Length of the bytecode
     *
     *  Returns:
     *    A testing::AssertionResult that indicates success or failure
     *    and contains a meaningful error message.
     *
     *  Throws:
     *    Nothing.
     */
    ::testing::AssertionResult verifyProgram(
      const std::string& context, const uint8_t* program,
      const uint8_t* truth, uint64_t length
    );

    /** Verify the size and content of a stack
     *
     *  Arguments:
     *    name      Name for the stack (appears in error messages)
     *    s         The stack to verify
     *    trueData  Expected stack content, from bottom to top
     *    trueSize  Expected stack size, in number of 8-byte values
     *
     *  Returns:
     *   ::testing::AssertionSuccess if "s" has size "trueSize" and contents
     *     "trueData"
     *   ::testing::AssertionFailure if the size or content of "s" does
     *     not match "trueSize" or "trueData"
     *
     *  Throws:
     *    Nothing
     */
    ::testing::AssertionResult verifyStack(
      const std::string& name, Stack s, const uint64_t* trueData,
      uint64_t trueSize
    );

    /** Verify the contents of the state block at "p"
     *
     *  Verifies the guard, the sizes of the saved address and call stacks
     *  and the contents of those stacks.
     *
     *  The state block pointer "p" should point to the data portion of
     *  the saved state block, not the header.  The verifyStateBlock()
     *  function will adjust it to point to the block's header.
     *
     *  Arguments:
     *    p                      Pointer to data area of state block to verify
     *    trueCallStackData      Expected content of the call stack.  Should
     *                             be 16 * trueCallStackSize bytes long.
     *    trueCallStackSize      Expected size of the call stack, in frames
     *    trueAddressStackData   Expected content of the address stack.
     *                             Should be 8 * trueAddressStackSize bytes
     *                             long.
     *    trueAddressStackSize   Expected size of the address stack, in
     *                             number of 8-byte addresses
     *
     *  Returns:
     *    ::testing::AssertionSuccess if the saved state's guard contains
     *      the correct bytes, the address and stack sizes match their expected
     *      sizes, and the contents of the address and stack sizes match their
     *      expected content.
     *    ::testing::AssertionFailure if any of those conditions are not met
     *
     *  Throws:
     *    Nothing
     */
    ::testing::AssertionResult verifyStateBlock(
      const uint8_t* p, const uint64_t* trueCallStackData,
      const uint64_t trueCallStackSize, const uint64_t* trueAddressStackData,
      const uint64_t trueAddressStackSize
    );

    /** Create and initialize an array
     *
     *  The initial size of the array will be "size * sizeof(T)" bytes and
     *  the maximum size will be "maxSize * sizeof(T)" in bytes.
     *
     *  Arguments:
     *    content    The initial content of the array
     *    size       Number of items in "content."
     *    maxSize    Maximum size, in number of items of size sizeof(T).
     *
     *  Returns:
     *    The new array, if successful, or NULL if array creation failed.
     */
    template <typename T>
    Array createAndInitArray(const T* content, size_t size, size_t maxSize) {
      Array a = createArray(size * sizeof(T), maxSize * sizeof(T));
      if (a) {
	::memcpy(startOfArray(a), content, size * sizeof(T));
      }
      return a;
    }
    
    /** Verify the contents of an array
     *
     *  Verifies the array has the correct size and content
     *
     *  Arguments:
     *    a         The array to verify
     *    trueData  It's expected content
     *    trueSize  It's expected size
     *
     *  Returns:
     *    ::testing::AssertionSuccess if the array has the exppected content
     *    and size, or ::testing::AssertionFailure if it does not.
     *
     *  Throws:
     *    Nothing
     */
    ::testing::AssertionResult verifyArray(Array a,
					   const uint8_t* trueData,
					   const uint64_t trueSize);

    /** Verify the contents of a block of memory
     *
     *  Verifies the contents of memory starting at "data."
     *
     *  Arguments:
     *    data      Verify memory starting from here
     *    trueData  What the region [trueData, trueData+trueSize) should
     *                contain
     *    trueSize  How many bytes to verify
     */
    ::testing::AssertionResult verifyBytes(const uint8_t* data,
					   const uint8_t* trueData,
					   const uint64_t trueSize);

    /** Dump the contents of array "a" to stdout */
    void dumpArray(Array a);
  }
}

#endif

