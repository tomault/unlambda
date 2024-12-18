extern "C" {
#include <stack.h>
#include <vm_instructions.h>
#include <vmmem.h>

}

#include <testing_utils.hpp>
#include <gtest/gtest.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

// TODO: Prefix all calls to testing utilities
//       For now, just import the namespace
using namespace unlambda::testing;

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
TEST(vmmem_tests, allocateTwiceFromFirstBlock) {
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
TEST(vmmem_tests, allocateFromFirstThenSecondBlock) {
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

  // Allocate 32 bytes from the first block, leaving 8
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
TEST(vmmem_tests, allocateTwoFromSecondBlock) {
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

  // Allocate 56 bytes from the second block, leaving 64
  CodeBlock* cb1 = allocateVmmCodeBlock(memory, 56);
  ASSERT_NE(cb1, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb1),
	    ptrToVmMemory(memory) + blockStructure[4].address);

  // Allocate 56 bytes from the second block, leaving 0
  CodeBlock* cb2 = allocateVmmCodeBlock(memory, 56);
  ASSERT_NE(cb2, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb2),
	    ptrToVmMemory(memory) + blockStructure[4].address + 64);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 48 + 24);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Free block (56 = 48 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (40 = 32 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (120 = 112 + 8)
  // * Free block (32 = 24 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmFreeBlockType, 48, 584),
    BlockSpec(VmmCodeBlockType, 56, 640),
    BlockSpec(VmmCodeBlockType, 32, 704),
    BlockSpec(VmmCodeBlockType, 56, 744),
    BlockSpec(VmmCodeBlockType, 56, 808),
    BlockSpec(VmmCodeBlockType, 112, 872),
    BlockSpec(VmmFreeBlockType, 24, 992)
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 584, 992 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Allocate three blocks, first from the second free block and second from
// the first free block and the third from the third free block.  Tests
// the linked list of free blocks is maintained correctly.
TEST(vmmem_tests, allocateOneFromEachFreeBlock) {
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
    BlockSpec(VmmCodeBlockType, 104),
    BlockSpec(VmmFreeBlockType, 32)
  };

  layoutBlocks(memory, blockStructure);

  // Allocate 104 bytes from the second free block, leaving 8
  CodeBlock* cb1 = allocateVmmCodeBlock(memory, 104);
  ASSERT_NE(cb1, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb1),
	    ptrToVmMemory(memory) + blockStructure[4].address);

  // Allocate 32 bytes from the first free block, leaving 8
  CodeBlock* cb2 = allocateVmmCodeBlock(memory, 32);
  ASSERT_NE(cb2, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb2),
	    ptrToVmMemory(memory) + blockStructure[1].address);

  // Allocate 16 bytes from the third free block, leaving 8
  CodeBlock* cb3 = allocateVmmCodeBlock(memory, 16);
  ASSERT_NE(cb3, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(cb3),
	    ptrToVmMemory(memory) + blockStructure[6].address);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 8 + 8 + 8);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  // Layout should be:
  // * Code block (72 = 64 + 8 bytes for header)
  // * Code block (40 = 32 + 8)
  // * Free block (16 =  8 + 8)
  // * Code block (64 = 56 + 8)
  // * Code block (40 = 32 + 8)
  // * Code block (112 = 104 + 8)
  // * Free block (16 =  8 + 8)
  // * Code block (112 = 104 + 8)
  // * Code block (24 = 16 + 8)
  // * Free block (16 =  8 + 8)
  const std::vector<BlockSpec> structAfterSplit{
    BlockSpec(VmmCodeBlockType,  64,  512),
    BlockSpec(VmmCodeBlockType,  32,  584),
    BlockSpec(VmmFreeBlockType,   8,  624),
    BlockSpec(VmmCodeBlockType,  56,  640),
    BlockSpec(VmmCodeBlockType,  32,  704),
    BlockSpec(VmmCodeBlockType, 104,  744),
    BlockSpec(VmmFreeBlockType,   8,  856),
    BlockSpec(VmmCodeBlockType, 104,  872),
    BlockSpec(VmmCodeBlockType,  16,  984),
    BlockSpec(VmmFreeBlockType,   8, 1008),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterSplit));

  const std::vector<uint64_t> freeBlockAddresses{ 624, 856, 1008 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Allocate a virtual machine state block.  The shared allocator has
// already been thoroughly tested with code block allocations, so only
// two tests are needed - one for succesful allocation and one that
// cannot be satisfied because a large enough block does not exist on the
// heap
TEST(vmmem_tests, allocateVmmStateBlock) {
  VmMemory memory = createVmMemory(1024, 4096);
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  VmStateBlock* sb = allocateVmmStateBlock(memory, 10, 24);
  ASSERT_NE(sb, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  EXPECT_EQ(reinterpret_cast<uint8_t*>(sb), ptrToVmMemory(memory) + 512);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 128); // = 512 - 368 - 2*8
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  EXPECT_EQ(getVmmBlockType(&(sb->header)), VmmStateBlockType);
  EXPECT_EQ(getVmmBlockSize(&(sb->header)), 16 + 10 * 16 + 24 * 8);
  EXPECT_FALSE(vmmBlockIsMarked(&(sb->header)));

  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(sb->guard[i], PANIC_INSTRUCTION)
      << "Guard[" << i << "] is incorrect.  It is " << sb->guard[i]
      << ", but it should be " << (uint32_t)PANIC_INSTRUCTION;
  }

  EXPECT_EQ(sb->callStackSize, 10);
  EXPECT_EQ(sb->addressStackSize, 24);

  // Should have
  //   VM state block (376 = 368 + 8 bytes for header)
  //   Free block     (136 = 128 + 8)
  const std::vector<BlockSpec> structAfterAlloc{
    BlockSpec(VmmStateBlockType,  368,  512),
    BlockSpec(VmmFreeBlockType,   128,  888),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterAlloc));

  const std::vector<uint64_t> freeBlockAddresses{ 888 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));
  
  destroyVmMemory(memory);
}

// Garbage collector tests

// Collect heap with no allocated blocks
TEST(vmmem_tests, collectFromEmptyHeap) {
  VmMemory memory = createVmMemory(1024, 4096);
  Stack callStack = createStack(8 * 16, 8 * 16);
  Stack addressStack = createStack(8 * 8, 8 * 8);
  std::vector<std::string> gcErrors;
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  EXPECT_EQ(collectUnreachableVmmBlocks(memory, callStack, addressStack,
					handleCollectorError, &gcErrors), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  if (gcErrors.size()) {
    std::cout << "GC Errors:" << std::endl;
    for (auto msg : gcErrors) {
      std::cout << "  " << msg << std::endl;
    }
    FAIL() << "Have GC errors";
  }
  
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 504);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType, 504, 512),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));

  const std::vector<uint64_t> freeBlockAddresses{ 512 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyStack(callStack);
  destroyStack(addressStack);
  destroyVmMemory(memory);
}

// Collect heap with one allocated code block
TEST(vmmem_tests, collectFromOneCodeBlockHeap) {
  VmMemory memory = createVmMemory(1024, 4096);
  Stack callStack = createStack(8 * 16, 8 * 16);
  Stack addressStack = createStack(8 * 8, 8 * 8);
  std::vector<std::string> gcErrors;
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  ASSERT_NE(allocateVmmCodeBlock(memory, 128), (void*)0);

  EXPECT_EQ(collectUnreachableVmmBlocks(memory, callStack, addressStack,
					handleCollectorError, &gcErrors), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  if (gcErrors.size()) {
    std::cout << "GC Errors:" << std::endl;
    for (auto msg : gcErrors) {
      std::cout << "  " << msg << std::endl;
    }
    FAIL() << "Have GC errors";
  }

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 504);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType, 504, 512),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));

  const std::vector<uint64_t> freeBlockAddresses{ 512 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyStack(callStack);
  destroyStack(addressStack);
  destroyVmMemory(memory);
}

// Collect heap with one allocated state block

// Collect heap with multiple blocks, none of which are referenced
TEST(vmmem_tests, collectMultipleBlocksNoneReferenced) {
  VmMemory memory = createVmMemory(1024, 4096);
  Stack callStack = createStack(8 * 16, 8 * 16);
  Stack addressStack = createStack(8 * 8, 8 * 8);
  std::vector<std::string> gcErrors;
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 8),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmStateBlockType, 240),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmFreeBlockType, 152),
  };
  
  layoutBlocks(memory, blockStructure);

  fillBlock(memory, blockStructure[0].address, blockStructure[0].blockSize,
	    HALT_INSTRUCTION);
  fillBlock(memory, blockStructure[1].address, blockStructure[1].blockSize,
	    HALT_INSTRUCTION);
  fillBlock(memory, blockStructure[4].address, blockStructure[4].blockSize,
	    HALT_INSTRUCTION);

  static const uint64_t stateBlockCallStack[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
  };
  static const uint64_t stateBlockAddressStack[] = { 0, 0, 0, 0 };
  writeStateBlock(ptrToVmmAddress(memory, blockStructure[2].address),
		  12, stateBlockCallStack, 4, stateBlockAddressStack);
  
  EXPECT_EQ(collectUnreachableVmmBlocks(memory, callStack, addressStack,
					handleCollectorError, &gcErrors), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  if (gcErrors.size()) {
    std::cout << "GC Errors:" << std::endl;
    for (auto msg : gcErrors) {
      std::cout << "  " << msg << std::endl;
    }
    FAIL() << "Have GC errors";
  }

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 504);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType, 504, 512),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));

  const std::vector<uint64_t> freeBlockAddresses{ 512 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyStack(callStack);
  destroyStack(addressStack);
  destroyVmMemory(memory);
}

// Collect heap with multiple blocks, some of which are referenced directly
TEST(vmmem_tests, collectMultipleBlocksWithDirectReferencesOnly) {
  VmMemory memory = createVmMemory(1024, 4096);
  Stack callStack = createStack(8 * 16, 8 * 16);
  Stack addressStack = createStack(8 * 8, 8 * 8);
  std::vector<std::string> gcErrors;
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 8),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmStateBlockType, 240),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmFreeBlockType, 128),
  };
  
  layoutBlocks(memory, blockStructure);

  fillBlock(memory, blockStructure[0].address, blockStructure[0].blockSize,
	    HALT_INSTRUCTION);
  fillBlock(memory, blockStructure[1].address, blockStructure[1].blockSize,
	    HALT_INSTRUCTION);
  fillBlock(memory, blockStructure[4].address, blockStructure[4].blockSize,
	    HALT_INSTRUCTION);
  fillBlock(memory, blockStructure[5].address, blockStructure[5].blockSize,
	    HALT_INSTRUCTION);

  static const uint64_t stateBlockCallStack[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
  };
  static const uint64_t stateBlockAddressStack[] = { 0, 0, 0, 0 };
  writeStateBlock(ptrToVmmAddress(memory, blockStructure[2].address),
		  12, stateBlockCallStack, 4, stateBlockAddressStack);
  
  // Reference block 2 and block 4 from the address stack
  // Addresses need to reference the data portion of the block, not
  //   the header.
  ASSERT_TRUE(assertPushAddress(addressStack, blockStructure[4].address
				                + sizeof(HeapBlock)));
  ASSERT_TRUE(assertPushAddress(addressStack, blockStructure[2].address
				                + sizeof(HeapBlock)));

  EXPECT_EQ(collectUnreachableVmmBlocks(memory, callStack, addressStack,
					handleCollectorError, &gcErrors), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  if (gcErrors.size()) {
    std::cout << "GC Errors:" << std::endl;
    for (auto msg : gcErrors) {
      std::cout << "  " << msg << std::endl;
    }
    FAIL() << "Have GC errors";
  }

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 216);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType,   32, 512),
    BlockSpec(VmmStateBlockType, 240, 552),
    BlockSpec(VmmFreeBlockType,   32, 800),
    BlockSpec(VmmCodeBlockType,   16, 840),
    BlockSpec(VmmFreeBlockType,  152, 864),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));

  const std::vector<uint64_t> freeBlockAddresses{ 512, 800, 864 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyStack(callStack);
  destroyStack(addressStack);
  destroyVmMemory(memory);
}

// Collect heap with multiple blocks, some of which are referenced
// directly and some of which are referenced indirectly from code blocks
TEST(vmmem_tests, collectMultipleBlocksWithIndirectCodeBlockReferences) {
  VmMemory memory = createVmMemory(1024, 4096);
  Stack callStack = createStack(8 * 16, 8 * 16);
  Stack addressStack = createStack(8 * 8, 8 * 8);
  std::vector<std::string> gcErrors;
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 16),      
    BlockSpec(VmmCodeBlockType, 24),     
    BlockSpec(VmmStateBlockType, 240),   
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType,  8),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmCodeBlockType,  8),
    BlockSpec(VmmFreeBlockType, 48),
  };
  
  layoutBlocks(memory, blockStructure);

  // Block[1] references block[5] and block[7].  Block[0] references block[4],
  // but is not itself referenced by anything.

  // Fill all code blocks with HALT instructions
  for (auto& block : blockStructure) {
    if (block.blockType == VmmCodeBlockType) {
      fillBlock(memory, block.address, block.blockSize, HALT_INSTRUCTION);
    }
  }

  // Write PUSH isntruction to block 0 to reference block 4, plus
  // a SAVE and RESTORE instruction to test visitCodeBlock()'s ability
  // to handle instructions of multiple sizes
  uint8_t* blockPtr = ptrToVmmAddress(memory, blockStructure[0].address
				                + sizeof(HeapBlock));
  blockPtr[0] = SAVE_INSTRUCTION;
  blockPtr[1] = 1;
  blockPtr[2] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 3) = blockStructure[4].address + sizeof(HeapBlock);
  blockPtr[11] =  PRINT_INSTRUCTION;
  blockPtr[12] =  65;
  blockPtr[13] =  RET_INSTRUCTION;
  
  // Write PUSH instructions to block 1 to reference blocks 5 and 7
  blockPtr = ptrToVmmAddress(memory, blockStructure[1].address
			               + sizeof(HeapBlock));
  blockPtr[0] = RESTORE_INSTRUCTION;
  blockPtr[1] = 1;
  blockPtr[2] = POP_INSTRUCTION;
  blockPtr[3] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 4) = blockStructure[7].address + sizeof(HeapBlock);
  blockPtr[12] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 13) = blockStructure[5].address + sizeof(HeapBlock);
  blockPtr[21] = PCALL_INSTRUCTION;
  blockPtr[22] = PCALL_INSTRUCTION;
  blockPtr[23] = RET_INSTRUCTION;

  // State block does not reference any heap blocks
  static const uint64_t stateBlockCallStack[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
  };
  static const uint64_t stateBlockAddressStack[] = { 0, 0, 0, 0 };
  writeStateBlock(ptrToVmmAddress(memory, blockStructure[2].address),
		  12, stateBlockCallStack, 4, stateBlockAddressStack);

  // Reference block 1 and block 6 from the call stack.  Note that
  // the values on the call stack are (address of code portion of heap
  // block called into, return address) pairs, each of which is 16 bytes
  // (two addresses) in size.
  ASSERT_TRUE(assertPushAddress(callStack, blockStructure[6].address
				                + sizeof(HeapBlock)));
  ASSERT_TRUE(assertPushAddress(callStack, blockStructure[0].address
				                + sizeof(HeapBlock)));
  ASSERT_TRUE(assertPushAddress(callStack, blockStructure[1].address
				                + sizeof(HeapBlock)));
  ASSERT_TRUE(assertPushAddress(callStack, 100));

  EXPECT_EQ(collectUnreachableVmmBlocks(memory, callStack, addressStack,
					handleCollectorError, &gcErrors), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  if (gcErrors.size()) {
    std::cout << "GC Errors:" << std::endl;
    for (auto msg : gcErrors) {
      std::cout << "  " << msg << std::endl;
    }
    FAIL() << "Have GC errors";
  }

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 384);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType,   16, 512),   // Block 0
    BlockSpec(VmmCodeBlockType,   24, 536),   // Block 1
    BlockSpec(VmmFreeBlockType,  304, 568),   // Blocks 2-4
    BlockSpec(VmmCodeBlockType,   16, 880),   // Block 5
    BlockSpec(VmmCodeBlockType,    8, 904),   // Block 6
    BlockSpec(VmmCodeBlockType,   24, 920),   // Block 7
    BlockSpec(VmmFreeBlockType,   64, 952),   // Blocks 8-9
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));

  const std::vector<uint64_t> freeBlockAddresses{ 512, 568, 952 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyStack(callStack);
  destroyStack(addressStack);
  destroyVmMemory(memory);
}

// Collect heap with multiple blocks, some of which are referenced
// directly and some of which are referenced indirectly from state blocks
TEST(vmmem_tests, collectMultipleBlocksWithIndirectStateBlockReferences) {
  VmMemory memory = createVmMemory(1024, 4096);
  Stack callStack = createStack(8 * 16, 8 * 16);
  Stack addressStack = createStack(8 * 8, 8 * 8);
  std::vector<std::string> gcErrors;
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 16),      
    BlockSpec(VmmCodeBlockType, 24),     
    BlockSpec(VmmStateBlockType, 240),   
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType,  8),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmCodeBlockType,  8),
    BlockSpec(VmmFreeBlockType, 48),
  };
  
  layoutBlocks(memory, blockStructure);

  // Block[1] references block[5] and block[7].  Block[0] references block[4],
  // but is not itself referenced by anything.

  // Fill all code blocks with HALT instructions
  for (auto& block : blockStructure) {
    if (block.blockType == VmmCodeBlockType) {
      fillBlock(memory, block.address, block.blockSize, HALT_INSTRUCTION);
    }
  }

  // Write PUSH instructions to block 1 to reference blocks 5 and 7
  uint8_t* blockPtr = ptrToVmmAddress(memory, blockStructure[1].address
				                + sizeof(HeapBlock));
  blockPtr[0] = RESTORE_INSTRUCTION;
  blockPtr[1] = 1;
  blockPtr[2] = POP_INSTRUCTION;
  blockPtr[3] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 4) = blockStructure[7].address + sizeof(HeapBlock);
  blockPtr[12] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 13) = blockStructure[5].address + sizeof(HeapBlock);
  blockPtr[21] = PCALL_INSTRUCTION;
  blockPtr[22] = PCALL_INSTRUCTION;
  blockPtr[23] = RET_INSTRUCTION;

  // State block references block[0], block[4] and block[6].  Note that
  // values on the saved call stack are (start of code in code block called
  // into, return address) pairs.  The collector uses only the first address
  // and not the second
  uint64_t stateBlockCallStack[] = {
      0, 0, 0, 0, blockStructure[4].address + sizeof(HeapBlock), 120, 0, 0,
      blockStructure[0].address + sizeof(HeapBlock),
      blockStructure[1].address + sizeof(HeapBlock) + 8, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
  };
  uint64_t stateBlockAddressStack[] = {
    0, blockStructure[6].address + sizeof(HeapBlock), 0, 0
  };

  writeStateBlock(ptrToVmmAddress(memory, blockStructure[2].address),
		  12, stateBlockCallStack, 4, stateBlockAddressStack);

  // Reference block 2 from the address stack
  ASSERT_TRUE(assertPushAddress(addressStack, blockStructure[2].address
				                + sizeof(HeapBlock)));

  EXPECT_EQ(collectUnreachableVmmBlocks(memory, callStack, addressStack,
					handleCollectorError, &gcErrors), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  if (gcErrors.size()) {
    std::cout << "GC Errors:" << std::endl;
    for (auto msg : gcErrors) {
      std::cout << "  " << msg << std::endl;
    }
    FAIL() << "Have GC errors";
  }

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 168);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmCodeBlockType,   16, 512),   // Block 0
    BlockSpec(VmmFreeBlockType,    24, 536),   // Block 1
    BlockSpec(VmmStateBlockType, 240, 568),   // Block 2
    BlockSpec(VmmFreeBlockType,   32, 816),   // Block 3
    BlockSpec(VmmCodeBlockType,   16, 856),   // Block 4
    BlockSpec(VmmFreeBlockType,   16, 880),   // Block 5
    BlockSpec(VmmCodeBlockType,    8, 904),   // Block 6
    BlockSpec(VmmFreeBlockType,   96, 920),   // Blocks 7-9
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));

  const std::vector<uint64_t> freeBlockAddresses{ 536, 816, 880, 920 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyStack(callStack);
  destroyStack(addressStack);
  destroyVmMemory(memory);
}

// Collect heap with circular references between blocks.  While the current
// Unlambda compiler should not generate code that creates heap blocks
// that reference one another, the collector should be able to handle this
// case in case it ever happens in the future.
TEST(vmmem_tests, collectMultipleBlocksWithCircularReferences) {
  VmMemory memory = createVmMemory(1024, 4096);
  Stack callStack = createStack(8 * 16, 8 * 16);
  Stack addressStack = createStack(8 * 8, 8 * 8);
  std::vector<std::string> gcErrors;
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmStateBlockType, 240),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmCodeBlockType,  8),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType,  8),
    BlockSpec(VmmFreeBlockType, 48),
  };
  
  layoutBlocks(memory, blockStructure);

  // Block[1] references block[7], while block[7] references block[1] and
  // block[5].  Block[0] references block[4] and vice-versa, but only
  // block[1] has a reference from the address stack.

  // Fill all code blocks with HALT instructions
  for (auto& block : blockStructure) {
    if (block.blockType == VmmCodeBlockType) {
      fillBlock(memory, block.address, block.blockSize, HALT_INSTRUCTION);
    }
  }

  // Write PUSH isntruction to block 0 to reference block 4, plus
  // a SAVE and RESTORE instruction to test visitCodeBlock()'s ability
  // to handle instructions of multiple sizes
  uint8_t* blockPtr = ptrToVmmAddress(memory, blockStructure[0].address
				                + sizeof(HeapBlock));
  blockPtr[0] = SAVE_INSTRUCTION;
  blockPtr[1] = 1;
  blockPtr[2] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 3) = blockStructure[4].address + sizeof(HeapBlock);
  blockPtr[11] =  PRINT_INSTRUCTION;
  blockPtr[12] =  65;
  blockPtr[13] =  RET_INSTRUCTION;
  
  // Write PUSH instructions to block 1 to reference blocks 5
  blockPtr = ptrToVmmAddress(memory, blockStructure[1].address
			               + sizeof(HeapBlock));
  blockPtr[0] = RESTORE_INSTRUCTION;
  blockPtr[1] = 1;
  blockPtr[2] = POP_INSTRUCTION;
  blockPtr[3] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 4) = blockStructure[5].address + sizeof(HeapBlock);
  blockPtr[12] = PCALL_INSTRUCTION;
  blockPtr[13] = PCALL_INSTRUCTION;
  blockPtr[14] = RET_INSTRUCTION;

  // Write PUSH instruction to block 4 to reference block 0
  blockPtr = ptrToVmmAddress(memory, blockStructure[4].address
			               + sizeof(HeapBlock));
  blockPtr[0] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 1) = blockStructure[0].address + sizeof(HeapBlock);

  // Write PUSH instructions to block 5 to reference blocks 1 and 7
  blockPtr = ptrToVmmAddress(memory, blockStructure[5].address
			               + sizeof(HeapBlock));
  blockPtr[6] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 7) = blockStructure[1].address + sizeof(HeapBlock);
  blockPtr[15] = PUSH_INSTRUCTION;
  *(uint64_t*)(blockPtr + 16) = blockStructure[7].address + sizeof(HeapBlock);

  // State block does not reference any heap blocks
  static const uint64_t stateBlockCallStack[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
  };
  static const uint64_t stateBlockAddressStack[] = { 0, 0, 0, 0 };
  writeStateBlock(ptrToVmmAddress(memory, blockStructure[2].address),
		  12, stateBlockCallStack, 4, stateBlockAddressStack);

  // Reference block 1 from the address stack
  ASSERT_TRUE(assertPushAddress(addressStack, blockStructure[1].address
				                + sizeof(HeapBlock)));

  EXPECT_EQ(collectUnreachableVmmBlocks(memory, callStack, addressStack,
					handleCollectorError, &gcErrors), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  if (gcErrors.size()) {
    std::cout << "GC Errors:" << std::endl;
    for (auto msg : gcErrors) {
      std::cout << "  " << msg << std::endl;
    }
    FAIL() << "Have GC errors";
  }

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512);
  EXPECT_EQ(vmmBytesFree(memory), 392);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 1024);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType,   16, 512),   // Block 0
    BlockSpec(VmmCodeBlockType,   24, 536),   // Block 1
    BlockSpec(VmmFreeBlockType,  304, 568),   // Blocks 2-4
    BlockSpec(VmmCodeBlockType,   24, 880),   // Block 5
    BlockSpec(VmmFreeBlockType,    8, 912),   // Block 6
    BlockSpec(VmmCodeBlockType,   16, 928),   // Block 7
    BlockSpec(VmmFreeBlockType,   64, 952),   // Blocks 8-9
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));

  const std::vector<uint64_t> freeBlockAddresses{ 512, 568, 912, 952 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyStack(callStack);
  destroyStack(addressStack);
  destroyVmMemory(memory);
}

// Increase the VM memory size with only one free block on the heap
TEST(vmmem_tests, increaseVmmSizeWithOneFreeBlock) {
  VmMemory memory = createVmMemory(1024, 4096);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  EXPECT_EQ(increaseVmmSize(memory), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 2048);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512 + 1024);
  EXPECT_EQ(vmmBytesFree(memory), 512 + 1024 - 8);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 2048);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType, 1528, 512),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));
  
  const std::vector<uint64_t> freeBlockAddresses{ 512 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyVmMemory(memory);
}

// Increase the VM heap size with several blocks on the heap and a free block
// at the end
TEST(vmmem_tests, increaseVmmSizeWithFreeBlockAtEnd) {
  VmMemory memory = createVmMemory(1024, 4096);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmStateBlockType, 240),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmFreeBlockType, 136),
  };
  
  layoutBlocks(memory, blockStructure);

  EXPECT_EQ(increaseVmmSize(memory), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 2048);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512 + 1024);
  EXPECT_EQ(vmmBytesFree(memory), 168 + 1024);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 2048);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmCodeBlockType, 16, 512),
    BlockSpec(VmmCodeBlockType, 24, 536),
    BlockSpec(VmmStateBlockType, 240, 568),
    BlockSpec(VmmFreeBlockType, 32, 816),
    BlockSpec(VmmCodeBlockType, 16, 856),
    BlockSpec(VmmFreeBlockType, 136 + 1024, 880),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));
  
  const std::vector<uint64_t> freeBlockAddresses{ 816, 880 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyVmMemory(memory);
}

// Increase the VM heap size with several blocks on the heap and a code block
// at the end
TEST(vmmem_tests, increaseVmmSizeWithCodeBlockAtEnd) {
  VmMemory memory = createVmMemory(1024, 4096);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmStateBlockType, 240),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmFreeBlockType, 64),
    BlockSpec(VmmCodeBlockType, 64),
  };
  
  layoutBlocks(memory, blockStructure);

  EXPECT_EQ(increaseVmmSize(memory), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 2048);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512 + 1024);
  EXPECT_EQ(vmmBytesFree(memory), 96 + 1016);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 2048);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmCodeBlockType, 16, 512),
    BlockSpec(VmmCodeBlockType, 24, 536),
    BlockSpec(VmmStateBlockType, 240, 568),
    BlockSpec(VmmFreeBlockType, 32, 816),
    BlockSpec(VmmCodeBlockType, 16, 856),
    BlockSpec(VmmFreeBlockType, 64, 880),
    BlockSpec(VmmCodeBlockType, 64, 952),
    BlockSpec(VmmFreeBlockType, 1016, 1024),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));
  
  const std::vector<uint64_t> freeBlockAddresses{ 816, 880, 1024 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyVmMemory(memory);
}

// Increase the VM heap size with no free blocks
TEST(vmmem_tests, increaseVmmSizeNoFreeBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 64),
    BlockSpec(VmmCodeBlockType, 144),
    BlockSpec(VmmCodeBlockType, 144),
    BlockSpec(VmmCodeBlockType, 128),
  };
  
  layoutBlocks(memory, blockStructure);

  EXPECT_EQ(increaseVmmSize(memory), 0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  EXPECT_EQ(currentVmmSize(memory), 2048);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(vmmHeapSize(memory), 512 + 1024);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 8);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 2048);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterIncrease{
    BlockSpec(VmmCodeBlockType, 64, 512),
    BlockSpec(VmmCodeBlockType, 144, 584),
    BlockSpec(VmmCodeBlockType, 144, 736),
    BlockSpec(VmmCodeBlockType, 128, 888),
    BlockSpec(VmmFreeBlockType, 1016, 1024),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterIncrease));
  
  const std::vector<uint64_t> freeBlockAddresses{ 1024 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyVmMemory(memory);
}

// Increase the VM heap size, clamping to its maximum size
TEST(vmmem_tests, clampToMaximumVmmSize) {
  VmMemory memory = createVmMemory(1024, 3072);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  ASSERT_EQ(increaseVmmSize(memory), 0);  // 1024 -> 2048
  EXPECT_EQ(increaseVmmSize(memory), 0);  // 2048 -> 3072
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");
  
  EXPECT_EQ(currentVmmSize(memory), 3072);
  EXPECT_EQ(maxVmmSize(memory), 3072);
  EXPECT_EQ(vmmHeapSize(memory), 3072 - 512);
  EXPECT_EQ(vmmBytesFree(memory), 3072 - 512 - 8);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 3072);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType, 3072 - 512 - 8, 512),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));
  
  const std::vector<uint64_t> freeBlockAddresses{ 512 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyVmMemory(memory);
}

// Increase the VM heap size beyond its maximum
TEST(vmmem_tests, increaseVmmSizeBeyondMax) {
  VmMemory memory = createVmMemory(1024, 2048);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  ASSERT_EQ(increaseVmmSize(memory), 0);  // 1024 -> 2048
  EXPECT_NE(increaseVmmSize(memory), 0);  // Fail.  Max size is 2048
  EXPECT_EQ(getVmmStatus(memory), VmmMaxSizeExceededError);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)),
	    "Maximum memory size exceeded");
  
  EXPECT_EQ(currentVmmSize(memory), 2048);
  EXPECT_EQ(maxVmmSize(memory), 2048);
  EXPECT_EQ(vmmHeapSize(memory), 2048 - 512);
  EXPECT_EQ(vmmBytesFree(memory), 2048 - 512 - 8);
  EXPECT_NE(ptrToVmMemory(memory), (void*)0);
  EXPECT_EQ(ptrToVmMemoryEnd(memory) - ptrToVmMemory(memory), 2048);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 512);
  EXPECT_EQ(getProgramStartInVmm(memory), ptrToVmMemory(memory));
  EXPECT_EQ(getVmmHeapStart(memory) - ptrToVmMemory(memory), 512);

  const std::vector<BlockSpec> structAfterCollection{
    BlockSpec(VmmFreeBlockType, 2048 - 512 - 8, 512),
  };
  EXPECT_TRUE(verifyBlockStructure(memory, structAfterCollection));
  
  const std::vector<uint64_t> freeBlockAddresses{ 512 };
  EXPECT_TRUE(verifyFreeBlockList(memory, freeBlockAddresses));

  destroyVmMemory(memory);
}

namespace {
  HeapBlock* saveVisitedBlock(VmMemory memory, HeapBlock* block,
			      void* context) {
    std::vector<BlockSpec>& blocks = *(std::vector<BlockSpec>*)context;
    blocks.emplace_back(
      getVmmBlockType(block), getVmmBlockSize(block),
      vmmAddressForPtr(memory, reinterpret_cast<uint8_t*>(block))
    );
    return NULL;
  }

  FreeBlock* saveVisitedFreeBlock(VmMemory memory, FreeBlock* block,
				  void* context) {
    std::vector<BlockSpec>& blocks = *(std::vector<BlockSpec>*)context;
    blocks.emplace_back(getVmmBlockType((HeapBlock*)block),
			getVmmBlockSize((HeapBlock*)block),
			vmmAddressForPtr(memory, (uint8_t*)block));
    return NULL;
  }
}

// Iterate over all blocks on the heap
TEST(vmmem_tests, iterateOverAllHeapBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmStateBlockType, 240),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmFreeBlockType, 64),
    BlockSpec(VmmCodeBlockType, 64),
  };
  
  layoutBlocks(memory, blockStructure);

  std::vector<BlockSpec> blocks;
  EXPECT_EQ(forEachVmmBlock(memory, saveVisitedBlock, &blocks), (void*)0);

  const std::vector<BlockSpec> trueBlocks{
    BlockSpec(VmmCodeBlockType, 16, 512),
    BlockSpec(VmmCodeBlockType, 24, 536),
    BlockSpec(VmmStateBlockType, 240, 568),
    BlockSpec(VmmFreeBlockType, 32, 816),
    BlockSpec(VmmCodeBlockType, 16, 856),
    BlockSpec(VmmFreeBlockType, 64, 880),
    BlockSpec(VmmCodeBlockType, 64, 952),
  };
  EXPECT_EQ(blocks, trueBlocks);
}

// Iterate over all free blocks on the heap
TEST(vmmem_tests, iterateOverAllFreeBlocks) {
  VmMemory memory = createVmMemory(1024, 4096);
  
  ASSERT_NE(memory, (void*)0);
  EXPECT_EQ(getVmmStatus(memory), 0);
  EXPECT_EQ(std::string(getVmmStatusMsg(memory)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(memory, 512), 0);

  std::vector<BlockSpec> blockStructure{
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmCodeBlockType, 24),
    BlockSpec(VmmStateBlockType, 240),
    BlockSpec(VmmFreeBlockType, 32),
    BlockSpec(VmmCodeBlockType, 16),
    BlockSpec(VmmFreeBlockType, 64),
    BlockSpec(VmmCodeBlockType, 64),
  };
  
  layoutBlocks(memory, blockStructure);

  std::vector<BlockSpec> blocks;
  EXPECT_EQ(forEachFreeBlockInVmm(memory, saveVisitedFreeBlock, &blocks),
	    (void*)0);

  const std::vector<BlockSpec> trueBlocks{
    BlockSpec(VmmFreeBlockType, 32, 816),
    BlockSpec(VmmFreeBlockType, 64, 880),
  };
  EXPECT_EQ(blocks, trueBlocks);
}

// TODO: Test early-stopping in forEachVmmBlock and forEachFreeBlockInVmm
//       by returning a non-NULL value from f.
