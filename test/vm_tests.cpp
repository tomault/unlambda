extern "C" {
#include <stack.h>
#include <vm.h>
#include <vm_instructions.h>
#include <vmmem.h>
}

#include <gtest/gtest.h>
#include <testing_utils.hpp>
#include <assert.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <vector>

namespace unl_test = unlambda::testing;

// TODO: Remove a lot of the boilerplate and put it into testing_utils
// TODO: Use testing_utils functions to make tests more concise
// TODO: Insert address of state block into addressStackData[] rather\
//         than pushing & verifying it separately
TEST(vm_tests, createVM) {
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(std::string(getVmProgramName(vm)), "");
  EXPECT_EQ(getVmPC(vm), 0);

  ASSERT_NE(getVmCallStack(vm), (void*)0);
  EXPECT_EQ(stackSize(getVmCallStack(vm)), 0);
  EXPECT_EQ(stackMaxSize(getVmCallStack(vm)), 16 * 16);

  ASSERT_NE(getVmAddressStack(vm), (void*)0);
  EXPECT_EQ(stackSize(getVmAddressStack(vm)), 0);
  EXPECT_EQ(stackMaxSize(getVmAddressStack(vm)), 24 * 8);

  ASSERT_NE(getVmSymbolTable(vm), (void*)0);
  EXPECT_EQ(symbolTableSize(getVmSymbolTable(vm)), 0);

  ASSERT_NE(getVmMemory(vm), (void*)0);
  EXPECT_EQ(currentVmmSize(getVmMemory(vm)), 1024);
  EXPECT_EQ(maxVmmSize(getVmMemory(vm)), 4096);

  EXPECT_EQ(ptrToVmPC(vm), ptrToVmMemory(getVmMemory(vm)) + getVmPC(vm));
  EXPECT_EQ(ptrToVmAddress(vm, 512), ptrToVmmAddress(getVmMemory(vm), 512));
  
  destroyUnlambdaVM(vm);
}

// Load a program from memory, configuring the program area
TEST(vm_tests, loadProgramFromMemoryConfiguringProgramArea) {
  static const uint8_t PROGRAM[] = {
    PCALL_INSTRUCTION,
    PUSH_INSTRUCTION, 0xAD, 0xBE, 0xED, 0xFE, 0xEF, 0xBE, 0xAD, 0xDE,
    MKS1_INSTRUCTION,
    RET_INSTRUCTION
  };
  /** Next power of 8 larger than the program size.  This is how much memory
   *  should be reserved for the program area
   */
  static const uint64_t PROGRAM_AREA_SIZE =
    (sizeof(PROGRAM) + 7) & ~(uint64_t)7;
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(loadVmProgramFromMemory(vm, "test-program", PROGRAM,
				    sizeof(PROGRAM)), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(std::string(getVmProgramName(vm)), "test-program");
  EXPECT_EQ(getVmPC(vm), 0);

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - PROGRAM_AREA_SIZE - 8);

  uint8_t* program = getProgramStartInVmm(memory);
  ASSERT_NE(program, (void*)0);
  EXPECT_FALSE(memcmp(program, PROGRAM, sizeof(PROGRAM)));
  
  destroyUnlambdaVM(vm);
}

// Load a program from memory into a proconfigured program area
TEST(vm_tests, loadProgramFromMemoryIntoPreconfiguredArea) {
  static const uint8_t PROGRAM[] = {
    PCALL_INSTRUCTION,
    PUSH_INSTRUCTION, 0xAD, 0xBE, 0xED, 0xFE, 0xEF, 0xBE, 0xAD, 0xDE,
    MKS1_INSTRUCTION,
    RET_INSTRUCTION
  };
  /** Large enough to store the program and then some */
  static const uint64_t PROGRAM_AREA_SIZE = 128;
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);
  
  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(getVmMemory(vm), PROGRAM_AREA_SIZE), 0);
  EXPECT_EQ(loadVmProgramFromMemory(vm, "test-program", PROGRAM,
				    sizeof(PROGRAM)), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");
  
  EXPECT_EQ(std::string(getVmProgramName(vm)), "test-program");
  EXPECT_EQ(getVmPC(vm), 0);

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - PROGRAM_AREA_SIZE - 8);

  uint8_t* program = getProgramStartInVmm(memory);
  ASSERT_NE(program, (void*)0);
  EXPECT_FALSE(memcmp(program, PROGRAM, sizeof(PROGRAM)));
  
  destroyUnlambdaVM(vm);
}

// Load a program that is too large into a preconfigured program area
TEST(vm_tests, loadProgramFromMemoryIntoTooSmallArea) {
  static const uint8_t PROGRAM[] = {
    PCALL_INSTRUCTION,
    PUSH_INSTRUCTION, 0xAD, 0xBE, 0xED, 0xFE, 0xEF, 0xBE, 0xAD, 0xDE,
    MKS1_INSTRUCTION,
    RET_INSTRUCTION
  };
  /** Too small to hold the program */
  static const uint64_t PROGRAM_AREA_SIZE = 8;
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);
  
  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(reserveVmMemoryForProgram(getVmMemory(vm), PROGRAM_AREA_SIZE), 0);
  EXPECT_NE(loadVmProgramFromMemory(vm, "test-program", PROGRAM,
				    sizeof(PROGRAM)), 0);
  EXPECT_EQ(getVmStatus(vm), VmIllegalArgumentError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "Cannot store a program of 12 bytes in a program area of 8 bytes");

  EXPECT_EQ(std::string(getVmProgramName(vm)), "");
  EXPECT_EQ(getVmPC(vm), 0);

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 8 - 8);

  destroyUnlambdaVM(vm);
}

// TODO: Load a program from disk

// Execute a PUSH instruction
TEST(vm_tests, executePushInstruction) {
  static const uint8_t PROGRAM[] = {
    PUSH_INSTRUCTION, 0xAD, 0xBE, 0xED, 0xFE, 0xEF, 0xBE, 0xAD, 0xDE,
  };
  static const uint64_t PROGRAM_AREA_SIZE =
    (sizeof(PROGRAM) + 7) & ~(uint64_t)7;
  UnlambdaVM vm = createUnlambdaVM(16, 1, 1024, 4096);
  
  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test-program", PROGRAM,
				    sizeof(PROGRAM)), 0);
  
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");
  EXPECT_EQ(getVmPC(vm), 9);

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - PROGRAM_AREA_SIZE - 8);

  Stack addressStack = getVmAddressStack(vm);
  ASSERT_EQ(stackSize(addressStack), 8);
  
  // TODO: Fix this for big-endian machines
  EXPECT_EQ(*(uint64_t*)(topOfStack(addressStack) - 8), 0xDEADBEEFFEEDBEAD);

  EXPECT_EQ(stackSize(getVmCallStack(vm)), 0);

  destroyUnlambdaVM(vm);
}

// Execute a PUSH instruction causing address stack overflow
TEST(vm_tests, executePushInstructionCausingOverflow) {
  static const uint8_t PROGRAM[] = {
    PUSH_INSTRUCTION, 0xAD, 0xBE, 0xED, 0xFE, 0xEF, 0xBE, 0xAD, 0xDE,
    PUSH_INSTRUCTION, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
  };
  static const uint64_t PROGRAM_AREA_SIZE =
    (sizeof(PROGRAM) + 7) & ~(uint64_t)7;
  UnlambdaVM vm = createUnlambdaVM(16, 1, 1024, 4096);
  
  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test-program", PROGRAM,
				    sizeof(PROGRAM)), 0);
  
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");
  EXPECT_EQ(getVmPC(vm), 9);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackOverflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack overflow");

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(maxVmmSize(memory), 4096);
  EXPECT_EQ(getVmmProgramMemorySize(memory), PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - PROGRAM_AREA_SIZE);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - PROGRAM_AREA_SIZE - 8);

  Stack addressStack = getVmAddressStack(vm);
  ASSERT_EQ(stackSize(addressStack), 8);
  
  // TODO: Fix this for big-endian machines
  EXPECT_EQ(*(uint64_t*)(topOfStack(addressStack) - 8), 0xDEADBEEFFEEDBEAD);

  EXPECT_EQ(stackSize(getVmCallStack(vm)), 0);

  destroyUnlambdaVM(vm);
}

// Execute a PUSH instruction with the operand cut off by the end of memory

// Execute a POP instruction
TEST(vm_tests, executePopInstruction) {
  static const uint64_t VALUE = 0xDEADCAFEFEEDBEEF;
  static const uint8_t PROGRAM[] = { POP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  Stack addressStack = getVmAddressStack(vm);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test-program", PROGRAM,
				    sizeof(PROGRAM)), 0);
  
  pushStack(addressStack, &VALUE, sizeof(VALUE));

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");
  EXPECT_EQ(getVmPC(vm), 1);

  EXPECT_EQ(stackSize(addressStack), 0);

  destroyUnlambdaVM(vm);
}

// Execute a POP instruction causing address stack underflow
TEST(vm_tests, executePopInstructionCausingUnderflow) {
  static const uint8_t PROGRAM[] = { POP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  Stack addressStack = getVmAddressStack(vm);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test-program", PROGRAM,
				    sizeof(PROGRAM)), 0);
  
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");
  EXPECT_EQ(getVmPC(vm), 0);

  EXPECT_EQ(stackSize(addressStack), 0);

  destroyUnlambdaVM(vm);
}

// Execute a SWAP instruction
TEST(vm_tests, executeSwapInstruction) {
  static const uint8_t PROGRAM[] = { SWAP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  Stack addressStack = getVmAddressStack(vm);
  const uint64_t value_1 = 0xDEADBEEFBEEDBEAD;
  const uint64_t value_2 = 0x1122334455667788;

  ASSERT_EQ(pushStack(addressStack, &value_2, sizeof(value_2)), 0);
  ASSERT_EQ(pushStack(addressStack, &value_1, sizeof(value_1)), 0);
  ASSERT_EQ(stackSize(addressStack), 2 * sizeof(value_1));

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);
  ASSERT_EQ(stackSize(addressStack), 2 * sizeof(value_1));
  
  uint64_t* stackTop = reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(stackTop[-1], value_2);
  EXPECT_EQ(stackTop[-2], value_1);

  destroyUnlambdaVM(vm);
}

// Execute a SWAP instruction on an empty address stack
TEST(vm_tests, executeSwapOnEmptyStack) {
  static const uint8_t PROGRAM[] = { SWAP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  Stack addressStack = getVmAddressStack(vm);
  ASSERT_EQ(stackSize(addressStack), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "Cannot SWAP a stack with only 0 entries");

  EXPECT_EQ(getVmPC(vm), 0);
  EXPECT_EQ(stackSize(addressStack), 0);

  destroyUnlambdaVM(vm);
}

// Execute a SWAP instruction on an address stack with one value
TEST(vm_tests, executeSwapOnStackWithOneValue) {
  static const uint8_t PROGRAM[] = { SWAP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  Stack addressStack = getVmAddressStack(vm);
  const uint64_t value = 0xDEADBEEFBEEDBEAD;

  ASSERT_EQ(pushStack(addressStack, &value, sizeof(value)), 0);
  ASSERT_EQ(stackSize(addressStack), sizeof(value));

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "Cannot SWAP a stack with only 1 entries");

  EXPECT_EQ(getVmPC(vm), 0);
  ASSERT_EQ(stackSize(addressStack), sizeof(value));
  
  uint64_t* stackTop = reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(stackTop[-1], value);

  destroyUnlambdaVM(vm);
}

// Execute a DUP instruction
TEST(vm_tests, executeDupInstruction) {
  static const uint8_t PROGRAM[] = { DUP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");
  
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t value_1 = 0xDEADBEEFFEEDBEAD;
  const uint64_t value_2 = 0x1122334455667788;

  ASSERT_EQ(pushStack(addressStack, &value_2, sizeof(value_2)), 0);
  ASSERT_EQ(pushStack(addressStack, &value_1, sizeof(value_1)), 0);
  ASSERT_EQ(stackSize(addressStack), 2 * sizeof(value_1));

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);
  ASSERT_EQ(stackSize(addressStack), 3 * sizeof(value_1));
  
  uint64_t* stackTop = reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(stackTop[-1], value_1);
  EXPECT_EQ(stackTop[-2], value_1);
  EXPECT_EQ(stackTop[-3], value_2);

  destroyUnlambdaVM(vm);
}

// Execute a DUP instruction on an empty stack
TEST(vm_tests, executeDupOnEmptyStack) {
  static const uint8_t PROGRAM[] = { DUP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 24, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");
  
  Stack addressStack = getVmAddressStack(vm);
  ASSERT_EQ(stackSize(addressStack), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "Cannot DUP the top of an empty stack");

  EXPECT_EQ(getVmPC(vm), 0);
  EXPECT_EQ(stackSize(addressStack), 0);

  destroyUnlambdaVM(vm);
}

// Execute a DUP instruction causing the address stack to overflow
TEST(vm_tests, executeDupOnFullStack) {
  static const uint8_t PROGRAM[] = { DUP_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");
  
  Stack addressStack = getVmAddressStack(vm);

  // Fill up the address stack
  for (uint64_t i = 1; i < 9; ++i) {
    ASSERT_EQ(pushStack(addressStack, &i, sizeof(i)), 0);
  }
  ASSERT_EQ(stackSize(addressStack), 8 * sizeof(uint64_t));

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackOverflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack overflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should be unchanged
  ASSERT_EQ(stackSize(addressStack), 8 * sizeof(uint64_t));

  uint64_t* stackTop = reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  for (uint64_t i = 1; i < 9; ++i) {
    EXPECT_EQ(stackTop[i - 9], i)
      << "Value at depth " << (9 - i) << " is incorrect.  It should be "
      << i << ", but it is " << stackTop[i - 9];
  }

  destroyUnlambdaVM(vm);
}

// Execute a PCALL instruction
TEST(vm_tests, executePCallInstruction) {
  static const uint8_t PROGRAM[] = { PCALL_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  Stack addressStack = getVmAddressStack(vm);
  const uint64_t address = 512 + sizeof(HeapBlock);

  ASSERT_EQ(pushStack(addressStack, &address, sizeof(address)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), address);
  EXPECT_EQ(stackSize(addressStack), 0);

  Stack callStack = getVmCallStack(vm);
  ASSERT_EQ(stackSize(callStack), 2 * sizeof(uint64_t));

  uint64_t* callStackTop = reinterpret_cast<uint64_t*>(topOfStack(callStack));

  // Return address
  EXPECT_EQ(callStackTop[-1], 1);

  // Block called into
  EXPECT_EQ(callStackTop[-2], 512 + sizeof(HeapBlock));

  destroyUnlambdaVM(vm);
}

// Execute a PCALL instruction causing the address stack to underflow
TEST(vm_tests, executePCallOnEmptyAddressStack) {
  static const uint8_t PROGRAM[] = { PCALL_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);
  EXPECT_EQ(stackSize(getVmAddressStack(vm)), 0);
  EXPECT_EQ(stackSize(getVmCallStack(vm)), 0);
  
  destroyUnlambdaVM(vm);
}

// Execute a PCALL instruction causing the call stack to overflow
TEST(vm_tests, executePCallOnFullCallStack) {
  static const uint8_t PROGRAM[] = { PCALL_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(8, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put the target address onto the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t address = 512 + sizeof(HeapBlock);

  ASSERT_EQ(pushStack(addressStack, &address, sizeof(address)), 0);

  // Fill the call stack
  Stack callStack = getVmCallStack(vm);

  for (uint64_t i = 8; i > 0; --i) {
    const uint64_t blockAddr = 512 + (16 * i) + sizeof(HeapBlock);
    ASSERT_EQ(pushStack(callStack, &blockAddr, sizeof(blockAddr)), 0);
    ASSERT_EQ(pushStack(callStack, &i, sizeof(i)), 0);
  }

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmCallStackOverflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Call stack overflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify address stack is unchanged
  EXPECT_EQ(stackSize(addressStack), sizeof(address));

  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1], address);

  // Verify call stack is unchanged
  ASSERT_EQ(stackSize(callStack), 16 * sizeof(uint64_t));

  uint64_t* callStackTop = reinterpret_cast<uint64_t*>(topOfStack(callStack));
  for (uint64_t i = 1; i < 9; ++i) {
    const uint64_t blockAddr = 512 + (16 * i) + sizeof(HeapBlock);
    // Block called into
    EXPECT_EQ(callStackTop[-(2 * i)], blockAddr)
      << "Block address at depth " << (2 * i) << " is incorrect.  It is "
      << callStackTop[-(2 * i)] << ", but it should be " << blockAddr;

    // Return address
    EXPECT_EQ(callStackTop[1 - (2 * i)], i)
      << "Return address at depth " << (1 - (2 * i)) << " is incorrect.  It is "
      << callStackTop[1 - (2 * i)] << ", but it should be " << i;
  }

  destroyUnlambdaVM(vm);
}

// Execute a PCALL instruction to an invalid address
TEST(vm_tests, executePCallToInvalidAddress) {
  static const uint8_t PROGRAM[] = { PCALL_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  Stack addressStack = getVmAddressStack(vm);
  const uint64_t address = 8192;

  ASSERT_EQ(pushStack(addressStack, &address, sizeof(address)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmIllegalAddressError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "PCALL to invalid address 0x2000");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify address stack is unchanged
  ASSERT_EQ(stackSize(addressStack), sizeof(address));

  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1], address);

  // Verify call stack is unchanged
  EXPECT_EQ(stackSize(getVmCallStack(vm)), 0);
  
  destroyUnlambdaVM(vm);
}

// Execute a RET instruction
TEST(vm_tests, executeReturnInstruction) {
  static const uint8_t PROGRAM[] = { RET_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put return address onto call stack
  Stack callStack = getVmCallStack(vm);
  const uint64_t blockAddr = 722;
  const uint64_t address = 16;

  ASSERT_EQ(pushStack(callStack, &blockAddr, sizeof(blockAddr)), 0);
  ASSERT_EQ(pushStack(callStack, &address, sizeof(address)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), address);
  EXPECT_EQ(stackSize(callStack), 0);

  destroyUnlambdaVM(vm);
}

// Execute a RET instruction causing the call stack to underflow
TEST(vm_tests, executeReturnInstructionCausingUnderflow) {
  static const uint8_t PROGRAM[] = { RET_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave call stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmCallStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Call stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);
  EXPECT_EQ(stackSize(getVmCallStack(vm)), 0);

  destroyUnlambdaVM(vm);
}

// Execute an MKK instruction
TEST(vm_tests, executeMkkInstruction) {
  static const uint8_t PROGRAM[] = { MKK_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put address function generated by MKK should return onto the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t address = 17;

  ASSERT_EQ(pushStack(addressStack, &address, sizeof(address)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Address stack should have the address of the block allocated on the
  // heap to hold the new constant function
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify the location of the code block and its contents
  VmMemory memory = getVmMemory(vm);
  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1],
	    vmmAddressForPtr(memory,
			     getVmmHeapStart(memory)) + sizeof(HeapBlock));

  uint8_t mkkByteCode[] = {
    PCALL_INSTRUCTION, POP_INSTRUCTION,
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    RET_INSTRUCTION
  };
  *(uint64_t*)(mkkByteCode + 3) = address;
  ASSERT_TRUE(unl_test::verifyProgram(
    "MKK-generated code", ptrToVmmAddress(memory, addrStackTop[-1]),
    mkkByteCode, sizeof(mkkByteCode)
  ));

  // Verify heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 16, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 40, 32)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 32 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 40);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKK instruction on an empty address stack
TEST(vm_tests, executeMkkOnEmptyStack) {
  static const uint8_t PROGRAM[] = { MKK_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still be empty
  Stack addressStack = getVmAddressStack(vm);
  EXPECT_EQ(stackSize(addressStack), 0);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKSO instruction
TEST(vm_tests, executeMks0Instruction) {
  static const uint8_t PROGRAM[] = { MKS0_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put argument to MKS0 on the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t address = 17;

  ASSERT_EQ(pushStack(addressStack, &address, sizeof(address)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Address stack should have the address of the block allocated on the
  // heap to hold the function MKS0 generated
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify the location of the code block and its contents
  VmMemory memory = getVmMemory(vm);
  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1],
	    vmmAddressForPtr(memory,
			     getVmmHeapStart(memory)) + sizeof(HeapBlock));

  uint8_t mks0ByteCode[] = {
    PCALL_INSTRUCTION,
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    MKS1_INSTRUCTION, RET_INSTRUCTION
  };
  *(uint64_t*)(mks0ByteCode + 2) = address;
  ASSERT_TRUE(unl_test::verifyProgram(
    "MKS0-generated code", ptrToVmmAddress(memory, addrStackTop[-1]),
    mks0ByteCode, sizeof(mks0ByteCode)
  ));

  // Verify heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 16, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 40, 32)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 32 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 40);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);  
}

// Execute an MKS0 instruction on an empty address stack
TEST(vm_tests, executeMks0OnEmptyStack) {
  static const uint8_t PROGRAM[] = { MKS0_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still be empty
  Stack addressStack = getVmAddressStack(vm);
  EXPECT_EQ(stackSize(addressStack), 0);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKS1 instruction
TEST(vm_tests, executeMks1Instruction) {
  static const uint8_t PROGRAM[] = { MKS1_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put arguments to MKS1 on the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t arg1 = 17;
  const uint64_t arg2 = 24;

  ASSERT_EQ(pushStack(addressStack, &arg2, sizeof(arg2)), 0);
  ASSERT_EQ(pushStack(addressStack, &arg1, sizeof(arg1)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Address stack should have the address of the block allocated on the
  // heap to hold the function MKS0 generated
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify the location of the code block and its contents
  VmMemory memory = getVmMemory(vm);
  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1],
	    vmmAddressForPtr(memory,
			     getVmmHeapStart(memory)) + sizeof(HeapBlock));

  uint8_t mks1ByteCode[] = {
    PCALL_INSTRUCTION, DUP_INSTRUCTION,
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    MKS2_INSTRUCTION, SWAP_INSTRUCTION,
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    PCALL_INSTRUCTION, PCALL_INSTRUCTION, RET_INSTRUCTION
  };
  *(uint64_t*)(mks1ByteCode + 3) = arg2;
  *(uint64_t*)(mks1ByteCode + 14) = arg1;
  ASSERT_TRUE(unl_test::verifyProgram(
    "MKS1-generated code", ptrToVmmAddress(memory, addrStackTop[-1]),
    mks1ByteCode, sizeof(mks1ByteCode)
  ));

  // Verify heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 32, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 48 - 8, 48)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 48 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 48 - 8);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);  
}

// Execute an MKS1 instruction on an empty address stack
TEST(vm_tests, executeMks1OnEmptyStack) {
  static const uint8_t PROGRAM[] = { MKS1_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still be empty
  Stack addressStack = getVmAddressStack(vm);
  EXPECT_EQ(stackSize(addressStack), 0);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKS1 instruction on an address stack with one address
TEST(vm_tests, executeMks1OnOneArgumentStack) {
  static const uint8_t PROGRAM[] = { MKS1_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put only one argument on the stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t arg = 99;
  ASSERT_EQ(pushStack(addressStack, &arg, sizeof(arg)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still have one value
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKS2 instruction
TEST(vm_tests, executeMks2Instruction) {
  static const uint8_t PROGRAM[] = { MKS2_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put arguments to MKS1 on the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t arg1 = 17;
  const uint64_t arg2 = 24;

  ASSERT_EQ(pushStack(addressStack, &arg2, sizeof(arg2)), 0);
  ASSERT_EQ(pushStack(addressStack, &arg1, sizeof(arg1)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Address stack should have the address of the block allocated on the
  // heap to hold the function MKS0 generated
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify the location of the code block and its contents
  VmMemory memory = getVmMemory(vm);
  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1],
	    vmmAddressForPtr(memory,
			     getVmmHeapStart(memory)) + sizeof(HeapBlock));

  uint8_t mks2ByteCode[] = {
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    PCALL_INSTRUCTION, RET_INSTRUCTION
  };
  *(uint64_t*)(mks2ByteCode + 1) = arg2;
  *(uint64_t*)(mks2ByteCode + 10) = arg1;
  ASSERT_TRUE(unl_test::verifyProgram(
    "MKS2-generated code", ptrToVmmAddress(memory, addrStackTop[-1]),
    mks2ByteCode, sizeof(mks2ByteCode)
  ));

  // Verify heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 24, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 40 - 8, 40)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 40 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 40 - 8);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);  
}

// Execute an MKS2 instruction on an empty address stack
TEST(vm_tests, executeMks2OnEmptyStack) {
  static const uint8_t PROGRAM[] = { MKS2_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still be empty
  Stack addressStack = getVmAddressStack(vm);
  EXPECT_EQ(stackSize(addressStack), 0);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKS2 instruction on an address stack with one address
TEST(vm_tests, executeMks2OnOneArgumentStack) {
  static const uint8_t PROGRAM[] = { MKS2_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put only one argument on the stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t arg = 99;
  ASSERT_EQ(pushStack(addressStack, &arg, sizeof(arg)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still have one value
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKD instruction
TEST(vm_tests, executeMkdInstruction) {
  static const uint8_t PROGRAM[] = { MKD_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put argument for MKD on the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t arg = 767;

  ASSERT_EQ(pushStack(addressStack, &arg, sizeof(arg)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Address stack should have the address of the block allocated on the
  // heap to hold the function MKS0 generated
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify the location of the code block and its contents
  VmMemory memory = getVmMemory(vm);
  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1],
	    vmmAddressForPtr(memory,
			     getVmmHeapStart(memory)) + sizeof(HeapBlock));

  uint8_t mkdByteCode[] = {
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    PCALL_INSTRUCTION, SWAP_INSTRUCTION,
    PCALL_INSTRUCTION, SWAP_INSTRUCTION,
    PCALL_INSTRUCTION,
    RET_INSTRUCTION
  };
  *(uint64_t*)(mkdByteCode + 1) = arg;
  ASSERT_TRUE(unl_test::verifyProgram(
    "MKD-generated code", ptrToVmmAddress(memory, addrStackTop[-1]),
    mkdByteCode, sizeof(mkdByteCode)
  ));

  // Verify heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 16, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 32 - 8, 32)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 32 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 32 - 8);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);  
}

// Execute an MKD instruction on an empty stack
TEST(vm_tests, executeMkdOnEmptyStack) {
  static const uint8_t PROGRAM[] = { MKD_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still be empty
  Stack addressStack = getVmAddressStack(vm);
  EXPECT_EQ(stackSize(addressStack), 0);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute an MKC instruction
TEST(vm_tests, executeMkcInsruction) {
  static const uint8_t PROGRAM[] = { MKC_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Put argument for MKC on the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t arg = 911;

  ASSERT_EQ(pushStack(addressStack, &arg, sizeof(arg)), 0);

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Address stack should have the address of the block allocated on the
  // heap to hold the function MKS0 generated
  EXPECT_EQ(stackSize(addressStack), 8);

  // Verify the location of the code block and its contents
  VmMemory memory = getVmMemory(vm);
  uint64_t* addrStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addrStackTop[-1],
	    vmmAddressForPtr(memory,
			     getVmmHeapStart(memory)) + sizeof(HeapBlock));

  uint8_t mkcByteCode[] = {
    PCALL_INSTRUCTION,
    PUSH_INSTRUCTION, 0, 0, 0, 0, 0, 0, 0, 0,
    RESTORE_INSTRUCTION, 1,
    RET_INSTRUCTION
  };
  *(uint64_t*)(mkcByteCode + 2) = arg;
  ASSERT_TRUE(unl_test::verifyProgram(
    "MKC-generated code", ptrToVmmAddress(memory, addrStackTop[-1]),
    mkcByteCode, sizeof(mkcByteCode)
  ));

  // Verify heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 16, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 32 - 8, 32)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 32 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 32 - 8);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);    
}

// Execute an MKC instruction on an empty stack
TEST(vm_tests, executeMkcOnEmptyStack) {
  static const uint8_t PROGRAM[] = { MKC_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Address stack should still be empty
  Stack addressStack = getVmAddressStack(vm);
  EXPECT_EQ(stackSize(addressStack), 0);

  // Verify heap structure
  VmMemory memory = getVmMemory(vm);
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8)
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 8 };

  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));
  
  destroyUnlambdaVM(vm);
}

// Execute a SAVE instruction
TEST(vm_tests, executeSaveInstruction) {
  static const uint8_t PROGRAM[] = { SAVE_INSTRUCTION, 2 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Put four values on the address stack.  The top two will not get
  // saved but the other three will.
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackContent[] = { 128, 160, 500, 57 };
  const uint64_t trueAddressStackSize =
    sizeof(addressStackContent) / sizeof(addressStackContent[0]);
  for (int i = 0; i < trueAddressStackSize; ++i) {
    ASSERT_TRUE(unl_test::assertPushAddress(addressStack,
					    addressStackContent[i]));
  }

  // Put three frames on the call stack
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackContent[] = { 800, 2, 999, 3, 700, 4 };
  const uint64_t trueCallStackSize =
    sizeof(callStackContent) / sizeof(callStackContent[0]);
  for (int i = 0; i < trueCallStackSize; ++i) {
    ASSERT_TRUE(unl_test::assertPushAddress(callStack, callStackContent[i]));
  }

  // Execute the SAVE instruction
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 2);

  // Verify the heap structure
  const std::vector<unl_test::BlockSpec> trueHeapBlocks{
    unl_test::BlockSpec(VmmStateBlockType, 80, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 920, 96),
  };
  const std::vector<uint64_t> trueFreeBlocks{ 96 };

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 920);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapBlocks));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the values on the address stack
  ASSERT_EQ(stackSize(addressStack), (trueAddressStackSize + 1) * 8);
  
  uint64_t* addrStackBottom =
    reinterpret_cast<uint64_t*>(bottomOfStack(addressStack));
  // Top of stack should be address of newly-created saved state block
  EXPECT_EQ(addrStackBottom[trueAddressStackSize],
	    trueHeapBlocks[0].address + sizeof(HeapBlock));

  // Remaining four entries should be the same as before
  for (int i = 0; i < trueAddressStackSize; ++i) {
    EXPECT_EQ(addrStackBottom[i], addressStackContent[i])
      << "addrStackBottom[" << i << "] is " << addrStackBottom[i]
      << ", but it should be " << addressStackContent[i];
  }

  // Verify the values on the call stack.  The call stack should be unchanged.
  ASSERT_EQ(stackSize(callStack), trueCallStackSize * 8);

  uint64_t* callStackBottom =
    reinterpret_cast<uint64_t*>(bottomOfStack(callStack));
  for (int i = 0; i < trueCallStackSize; ++i) {
    EXPECT_EQ(callStackBottom[i], callStackContent[i])
      << "callStackBottom[" << i << "] is " << callStackBottom[i]
      << ", but it should be " << callStackContent[i];
  }

  // Verify the contents of the saved state block
  VmStateBlock* savedState = reinterpret_cast<VmStateBlock*>(
    ptrToVmmAddress(memory, addrStackBottom[trueAddressStackSize]
		              - sizeof(HeapBlock))
  );

  EXPECT_EQ(getVmmBlockType(&(savedState->header)), VmmStateBlockType);
  EXPECT_EQ(getVmmBlockSize(&(savedState->header)), 80);

  for (int i = 0; i < sizeof(savedState->guard); ++i) {
    EXPECT_EQ(savedState->guard[i], PANIC_INSTRUCTION)
      << "guard[" << i << " has value " << savedState->guard[i]
      << ", but it should have value " << PANIC_INSTRUCTION;
  }
  // savedState->callStackSize is number of frames.  Each frame is
  // two 64-bit values
  EXPECT_EQ(savedState->callStackSize, trueCallStackSize / 2);
  EXPECT_EQ(savedState->addressStackSize, trueAddressStackSize - 2);

  uint64_t* savedCallStack = reinterpret_cast<uint64_t*>(savedState->stacks);
  for (int i = 0; i < trueCallStackSize; ++i) {
    EXPECT_EQ(savedCallStack[i], callStackContent[i])
      << "Value " << i << " entries from the bottom of the saved call stack "
      << "is incorrect.  It is " << savedCallStack[i] << ", but it should be "
      << callStackContent[i];
  }

  uint64_t* savedAddrStack = reinterpret_cast<uint64_t*>(
    savedState->stacks + 16 * savedState->callStackSize
  );
  for (int i = 0; i < (trueAddressStackSize - 2); ++i) {
    EXPECT_EQ(savedAddrStack[i], addressStackContent[i])
      << "Value " << i << " entries from the bottom of the saved address "
      << "stack is incorrect.  It is " << savedAddrStack[i]
      << ", but it should be " << addressStackContent[i];
  }

  destroyUnlambdaVM(vm);
}

// Execute a SAVE instruction without enough addresses on the stack
TEST(vm_tests, executeSaveCausingUnderflow) {
  static const uint8_t PROGRAM[] = { SAVE_INSTRUCTION, 5 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Put four values on the address stack, but try to save 5
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackContent[] = { 128, 160, 500, 57 };
  const uint64_t trueAddressStackSize =
    sizeof(addressStackContent) / sizeof(addressStackContent[0]);
  for (int i = 0; i < trueAddressStackSize; ++i) {
    ASSERT_TRUE(unl_test::assertPushAddress(addressStack,
					    addressStackContent[i]));
  }

  // Put three frames on the call stack
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackContent[] = { 800, 2, 999, 3, 700, 4 };
  const uint64_t trueCallStackSize =
    sizeof(callStackContent) / sizeof(callStackContent[0]);
  for (int i = 0; i < trueCallStackSize; ++i) {
    ASSERT_TRUE(unl_test::assertPushAddress(callStack, callStackContent[i]));
  }

  // Execute the SAVE instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap structure
  const std::vector<unl_test::BlockSpec> trueHeapBlocks{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8),
  };
  const std::vector<uint64_t> trueFreeBlocks{ 8 };

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapBlocks));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the address stack is unchanged
  ASSERT_EQ(stackSize(addressStack), trueAddressStackSize * 8);
  
  uint64_t* addrStackBottom =
    reinterpret_cast<uint64_t*>(bottomOfStack(addressStack));
  for (int i = 0; i < trueAddressStackSize; ++i) {
    EXPECT_EQ(addrStackBottom[i], addressStackContent[i])
      << "addrStackBottom[" << i << "] is " << addrStackBottom[i]
      << ", but it should be " << addressStackContent[i];
  }

  // Verify the call stack is unchanged
  ASSERT_EQ(stackSize(callStack), trueCallStackSize * 8);

  uint64_t* callStackBottom =
    reinterpret_cast<uint64_t*>(bottomOfStack(callStack));
  for (int i = 0; i < trueCallStackSize; ++i) {
    EXPECT_EQ(callStackBottom[i], callStackContent[i])
      << "callStackBottom[" << i << "] is " << callStackBottom[i]
      << ", but it should be " << callStackContent[i];
  }

  destroyUnlambdaVM(vm);
}

// Execute a SAVE instruction causing the address stack to overflow
TEST(vm_tests, executeSaveCausingOverflow) {
  static const uint8_t PROGRAM[] = { SAVE_INSTRUCTION, 1 };
  UnlambdaVM vm = createUnlambdaVM(16, 4, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Put four values on the address stack, which leaves no room for the
  // pointer to the saved state block, since the address stack is limited to
  // four entries.
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackContent[] = { 128, 160, 500, 57 };
  const uint64_t trueAddressStackSize =
    sizeof(addressStackContent) / sizeof(addressStackContent[0]);
  for (int i = 0; i < trueAddressStackSize; ++i) {
    ASSERT_TRUE(unl_test::assertPushAddress(addressStack,
					    addressStackContent[i]));
  }

  // Put three frames on the call stack
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackContent[] = { 800, 2, 999, 3, 700, 4 };
  const uint64_t trueCallStackSize =
    sizeof(callStackContent) / sizeof(callStackContent[0]);
  for (int i = 0; i < trueCallStackSize; ++i) {
    ASSERT_TRUE(unl_test::assertPushAddress(callStack, callStackContent[i]));
  }

  // Execute the SAVE instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackOverflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack overflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap structure
  const std::vector<unl_test::BlockSpec> trueHeapBlocks{
    unl_test::BlockSpec(VmmFreeBlockType, 1024 - 16, 8),
  };
  const std::vector<uint64_t> trueFreeBlocks{ 8 };

  VmMemory memory = getVmMemory(vm);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapBlocks));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the address stack is unchanged
  ASSERT_EQ(stackSize(addressStack), trueAddressStackSize * 8);
  
  uint64_t* addrStackBottom =
    reinterpret_cast<uint64_t*>(bottomOfStack(addressStack));
  for (int i = 0; i < trueAddressStackSize; ++i) {
    EXPECT_EQ(addrStackBottom[i], addressStackContent[i])
      << "addrStackBottom[" << i << "] is " << addrStackBottom[i]
      << ", but it should be " << addressStackContent[i];
  }

  // Verify the call stack is unchanged
  ASSERT_EQ(stackSize(callStack), trueCallStackSize * 8);

  uint64_t* callStackBottom =
    reinterpret_cast<uint64_t*>(bottomOfStack(callStack));
  for (int i = 0; i < trueCallStackSize; ++i) {
    EXPECT_EQ(callStackBottom[i], callStackContent[i])
      << "callStackBottom[" << i << "] is " << callStackBottom[i]
      << ", but it should be " << callStackContent[i];
  }

  destroyUnlambdaVM(vm);
}

// Execute a RESTORE instruction
TEST(vm_tests, executeRestoreInstruction) {
  static const uint8_t PROGRAM[] = { RESTORE_INSTRUCTION, 1 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  const uint64_t restoredAddrStackData[] = { 16, 40, 160, 352, 640 };
  const uint64_t restoredAddrStackSize =
    sizeof(restoredAddrStackData) / sizeof(restoredAddrStackData[0]);
  const uint64_t restoredCallStackData[] = { 136, 4, 400, 2, 248, 3 };
  const uint64_t restoredCallStackSize =
    sizeof(restoredCallStackData) / sizeof(restoredCallStackData[0]);

  // Create a heap with one saved state block and one free block
  std::vector<unl_test::BlockSpec> heapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104),
    unl_test::BlockSpec(VmmFreeBlockType, 896),
  };

  VmMemory memory = getVmMemory(vm);
  unl_test::layoutBlocks(memory, heapStructure);

  // Initialize the saved state block
  unl_test::writeStateBlock(ptrToVmmAddress(memory, heapStructure[0].address),
			    restoredCallStackSize / 2, restoredCallStackData,
			    restoredAddrStackSize, restoredAddrStackData);

  // Initialize the address stack with four values plus the address of
  // the saved state block.  When the RESTORE instruction executes,
  // the second value from the top (first value behind the saved state
  // block address) will be kept and pushed onto the restored address stack.
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = { 72, 24, 912, 888 };
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, heapStructure[0].address + sizeof(HeapBlock)
  ));

  // Initialize the call stack with one frame
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = { 1008, 7 };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));
  
  // Execute the RESTORE instruction
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 2);

  // Verify the heap structure is unchanged
  const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 896, 120),
  };
  const std::vector<uint64_t> trueFreeBlockList{ 120 };

  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 896);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlockList));

  // Verify the address stack
  uint64_t* trueAddrStackData =
    (uint64_t*)::malloc(8 * (restoredAddrStackSize + 1));
  assert(trueAddrStackData);
  ::memcpy(trueAddrStackData, restoredAddrStackData, 8 * restoredAddrStackSize);
  trueAddrStackData[restoredAddrStackSize] =
    addressStackData[addressStackSize - 1];
  
  EXPECT_TRUE(unl_test::verifyStack("address stack", addressStack,
				    trueAddrStackData,
				    restoredAddrStackSize + 1));
  ::free(trueAddrStackData);
    
  // Verify the call stack
  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack,
				    restoredCallStackData,
				    restoredCallStackSize));
  
  destroyUnlambdaVM(vm);
}

// Execute a RESTORE instruction on an empty address stack
TEST(vm_tests, executeRestoreOnEmptyStack) {
  static const uint8_t PROGRAM[] = { RESTORE_INSTRUCTION, 0 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  const uint64_t restoredAddrStackData[] = { 16, 40, 160, 352, 640 };
  const uint64_t restoredAddrStackSize =
    sizeof(restoredAddrStackData) / sizeof(restoredAddrStackData[0]);
  const uint64_t restoredCallStackData[] = { 136, 4, 400, 2, 248, 3 };
  const uint64_t restoredCallStackSize =
    sizeof(restoredCallStackData) / sizeof(restoredCallStackData[0]);

  // Create a heap with one saved state block and one free block
  std::vector<unl_test::BlockSpec> heapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104),
    unl_test::BlockSpec(VmmFreeBlockType, 896),
  };

  VmMemory memory = getVmMemory(vm);
  unl_test::layoutBlocks(memory, heapStructure);

  // Initialize the saved state block
  unl_test::writeStateBlock(ptrToVmmAddress(memory, heapStructure[0].address),
			    restoredCallStackSize / 2, restoredCallStackData,
			    restoredAddrStackSize, restoredAddrStackData);

  // Leave address stack empty

  // Initialize the call stack with one frame
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = { 1008, 7 };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));
  
  // Execute the RESTORE instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap structure is unchanged
  const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 896, 120),
  };
  const std::vector<uint64_t> trueFreeBlockList{ 120 };

  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 896);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlockList));

  // Verify the address and call stacks are unchanged
  EXPECT_EQ(stackSize(getVmAddressStack(vm)), 0);
  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack,
				    callStackData, callStackSize));
  
  destroyUnlambdaVM(vm);  
}

// Execute a RESTORE instruction with an invalid address on the address stack
TEST(vm_tests, executeRestoreFromInvalidAddress) {
  static const uint8_t PROGRAM[] = { RESTORE_INSTRUCTION, 0 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  const uint64_t restoredAddrStackData[] = { 16, 40, 160, 352, 640 };
  const uint64_t restoredAddrStackSize =
    sizeof(restoredAddrStackData) / sizeof(restoredAddrStackData[0]);
  const uint64_t restoredCallStackData[] = { 136, 4, 400, 2, 248, 3 };
  const uint64_t restoredCallStackSize =
    sizeof(restoredCallStackData) / sizeof(restoredCallStackData[0]);

  // Create a heap with one saved state block and one free block
  std::vector<unl_test::BlockSpec> heapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104),
    unl_test::BlockSpec(VmmFreeBlockType, 896),
  };

  VmMemory memory = getVmMemory(vm);
  unl_test::layoutBlocks(memory, heapStructure);

  // Initialize the saved state block
  unl_test::writeStateBlock(ptrToVmmAddress(memory, heapStructure[0].address),
			    restoredCallStackSize / 2, restoredCallStackData,
			    restoredAddrStackSize, restoredAddrStackData);

  // Put an invalid address on the address stack top
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = { 416, 2048 + sizeof(HeapBlock)};
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));

  // Initialize the call stack with one frame
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = { 1008, 7 };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));
  
  // Execute the RESTORE instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmIllegalAddressError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Cannot read from address 0x800");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap structure is unchanged
  const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 896, 120),
  };
  const std::vector<uint64_t> trueFreeBlockList{ 120 };

  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 896);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlockList));

  // Verify the address and call stacks are unchanged
  EXPECT_TRUE(unl_test::verifyStack("address stack", addressStack,
				    addressStackData, addressStackSize));
  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack,
				    callStackData, callStackSize));
  
  destroyUnlambdaVM(vm);
}

// Execute a RESTORE instruction with the address on the stack pointing
// to a code block
TEST(vm_tests, executeRestoreFromCodeBlock) {
  static const uint8_t PROGRAM[] = { RESTORE_INSTRUCTION, 0 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Create a heap with one saved state block and one free block
  std::vector<unl_test::BlockSpec> heapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 104),
    unl_test::BlockSpec(VmmFreeBlockType, 896),
  };

  VmMemory memory = getVmMemory(vm);
  unl_test::layoutBlocks(memory, heapStructure);

  // Put address of code block on stack top
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = { 416 };
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, heapStructure[0].address + sizeof(HeapBlock)
  ));

  // Initialize the call stack with one frame
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = { 1008, 7 };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));
  
  // Execute the RESTORE instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmFatalError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "Block at address 0x10 is not a VmStateBlock.  It has type 1");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap structure is unchanged
  const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 104, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 896, 120),
  };
  const std::vector<uint64_t> trueFreeBlockList{ 120 };

  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 896);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlockList));

  // Verify the address and call stacks are unchanged
  uint64_t* trueAddrStackData =
    (uint64_t*)::malloc((addressStackSize + 1) * sizeof(uint64_t));
  ::memcpy(trueAddrStackData, addressStackData,
	   addressStackSize * sizeof(uint64_t));
  trueAddrStackData[addressStackSize] =
    heapStructure[0].address + sizeof(HeapBlock);
  EXPECT_TRUE(unl_test::verifyStack("address stack", addressStack,
				    trueAddrStackData, addressStackSize + 1));
  ::free((void*)trueAddrStackData);

  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack,
				    callStackData, callStackSize));
  
  destroyUnlambdaVM(vm);
}

// Execute a RESTORE instruction causing address stack underflow
TEST(vm_tests, executeRestoreCausingStackUnderflow) {
  // Try to restore 3 addresses from a stack with only 2 addresses on it
  static const uint8_t PROGRAM[] = { RESTORE_INSTRUCTION, 3 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  const uint64_t restoredAddrStackData[] = { 16, 40, 160, 352, 640 };
  const uint64_t restoredAddrStackSize =
    sizeof(restoredAddrStackData) / sizeof(restoredAddrStackData[0]);
  const uint64_t restoredCallStackData[] = { 136, 4, 400, 2, 248, 3 };
  const uint64_t restoredCallStackSize =
    sizeof(restoredCallStackData) / sizeof(restoredCallStackData[0]);

  // Create a heap with one saved state block and one free block
  std::vector<unl_test::BlockSpec> heapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104),
    unl_test::BlockSpec(VmmFreeBlockType, 896),
  };

  VmMemory memory = getVmMemory(vm);
  unl_test::layoutBlocks(memory, heapStructure);

  // Initialize the saved state block
  unl_test::writeStateBlock(ptrToVmmAddress(memory, heapStructure[0].address),
			    restoredCallStackSize / 2, restoredCallStackData,
			    restoredAddrStackSize, restoredAddrStackData);

  // Put address of code block on stack top
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = {
    416, heapStructure[0].address + sizeof(HeapBlock)
  };
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));

  // Initialize the call stack with one frame
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = { 1008, 7 };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));
  
  // Execute the RESTORE instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackUnderflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack underflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap structure is unchanged
  const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 896, 120),
  };
  const std::vector<uint64_t> trueFreeBlockList{ 120 };

  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 896);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlockList));

  // Verify the address and call stacks are unchanged
  EXPECT_TRUE(unl_test::verifyStack("address stack", addressStack,
				    addressStackData, addressStackSize));
  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack,
				    callStackData, callStackSize));
  
  destroyUnlambdaVM(vm);  
}

// Execute a RESTORE instruction causing address stack overflow
TEST(vm_tests, executeRestoreCausingStackOverflow) {
  static const uint8_t PROGRAM[] = { RESTORE_INSTRUCTION, 1 };
  UnlambdaVM vm = createUnlambdaVM(16, 5, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  const uint64_t restoredAddrStackData[] = { 16, 40, 160, 352, 640 };
  const uint64_t restoredAddrStackSize =
    sizeof(restoredAddrStackData) / sizeof(restoredAddrStackData[0]);
  const uint64_t restoredCallStackData[] = { 136, 4, 400, 2, 248, 3 };
  const uint64_t restoredCallStackSize =
    sizeof(restoredCallStackData) / sizeof(restoredCallStackData[0]);

  // Create a heap with one saved state block and one free block
  std::vector<unl_test::BlockSpec> heapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104),
    unl_test::BlockSpec(VmmFreeBlockType, 896),
  };

  VmMemory memory = getVmMemory(vm);
  unl_test::layoutBlocks(memory, heapStructure);

  // Initialize the saved state block
  unl_test::writeStateBlock(ptrToVmmAddress(memory, heapStructure[0].address),
			    restoredCallStackSize / 2, restoredCallStackData,
			    restoredAddrStackSize, restoredAddrStackData);

  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = {
    416, heapStructure[0].address + sizeof(HeapBlock)
  };
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));

  // Initialize the call stack with one frame
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = { 1008, 7 };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));
  
  // Execute the RESTORE instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmAddressStackOverflowError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "Address stack overflow");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap structure is unchanged
  const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmStateBlockType, 104, 8),
    unl_test::BlockSpec(VmmFreeBlockType, 896, 120),
  };
  const std::vector<uint64_t> trueFreeBlockList{ 120 };

  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 896);
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlockList));

  // Verify the address and call stacks are unchanged
  EXPECT_TRUE(unl_test::verifyStack("address stack", addressStack,
				    addressStackData, addressStackSize));
  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack,
				    callStackData, callStackSize));
  
  destroyUnlambdaVM(vm);  
}

// Execute a PRINT instruction
TEST(vm_tests, executePrintInstruction) {
  static const uint8_t PROGRAM[] = { PRINT_INSTRUCTION, 65 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Can't verify write to stdout easily, so just verify the instruction
  // executes
  // TODO: Add option to redirect VM's printed output to a stream
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 2);
  EXPECT_EQ(stackSize(getVmAddressStack(vm)), 0);
  EXPECT_EQ(stackSize(getVmCallStack(vm)), 0);
  EXPECT_EQ(vmmHeapSize(getVmMemory(vm)), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(getVmMemory(vm)), 1024 - 16);

  destroyUnlambdaVM(vm);
}

// Execute a HALT instruction
TEST(vm_tests, executeHaltInstruction) {
  static const uint8_t PROGRAM[] = { HALT_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmHalted);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "VM halted");
  EXPECT_EQ(getVmPC(vm), 0);

  destroyUnlambdaVM(vm);
}

// Execute a PANIC instruction
TEST(vm_tests, executePanicInstruction) {
  static const uint8_t PROGRAM[] = { PANIC_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmPanicError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "VM executed a PANIC instruction");
  EXPECT_EQ(getVmPC(vm), 0);

  destroyUnlambdaVM(vm);
}

// Execute an illegal instruction
TEST(vm_tests, executeIllegalInstruction) {
  static const uint8_t PROGRAM[] = { 255 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  // Leave address stack empty

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmIllegalInstructionError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "VM attempted to execute an illegal instruction");
  EXPECT_EQ(getVmPC(vm), 0);  

  destroyUnlambdaVM(vm);
}

// Execute a MKS2 instruction forcing garbage collection
TEST(vm_tests, executeMks2ForcingGarbageCollection) {
  static const uint8_t PROGRAM[] = { MKS2_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Heap consists of four code blocks.  The first and fourth blocks
  // are referenced by the arguments to MKS2, while the middle two are
  // unreferenced.  After garbage collection, there will be four blocks
  // on the heap: the original first block, the block containing the MKS2-
  // generated code, a free block and the original fourth block.
  std::vector<unl_test::BlockSpec> blockStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64),
    unl_test::BlockSpec(VmmCodeBlockType, 400),
    unl_test::BlockSpec(VmmCodeBlockType, 392),
    unl_test::BlockSpec(VmmCodeBlockType, 128),
  };

  VmMemory memory = getVmMemory(vm);
  layoutBlocks(memory, blockStructure);

  // First and last blocks are arguments to MKS2
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t arg1 = blockStructure[0].address + sizeof(HeapBlock);
  const uint64_t arg2 = blockStructure[3].address + sizeof(HeapBlock);
  ASSERT_EQ(pushStack(addressStack, &arg2, sizeof(arg2)), 0);
  ASSERT_EQ(pushStack(addressStack, &arg1, sizeof(arg1)), 0);

  // Call stack is empty

  // Execute the instruction
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Verify the heap
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 768);

  // Expected heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64, 8),
    unl_test::BlockSpec(VmmCodeBlockType, 24, 80),
    unl_test::BlockSpec(VmmFreeBlockType, 768, 112),
    unl_test::BlockSpec(VmmCodeBlockType, 128, 888),
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 112 };
  
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the result of MKS2 on the address stack
  ASSERT_EQ(stackSize(addressStack), 8);
  
  uint64_t* addressStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addressStackTop[-1],
	    trueHeapStructure[1].address + sizeof(HeapBlock));

  destroyUnlambdaVM(vm);
}

// Execute a MKS2 instruction forcing an increase in the size of the VM's memory
TEST(vm_tests, executeMks2ForcingMemoryIncrease) {
  static const uint8_t PROGRAM[] = { MKS2_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Heap consists of four code blocks.  The first and fourth blocks
  // are referenced by the arguments to MKS2, while the middle two are
  // referenced from the address stack.  The VM will need to increase
  // the size of its memory so it can accomodate the allocation request
  // from MKS2.
  std::vector<unl_test::BlockSpec> blockStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64),
    unl_test::BlockSpec(VmmCodeBlockType, 400),
    unl_test::BlockSpec(VmmCodeBlockType, 392),
    unl_test::BlockSpec(VmmCodeBlockType, 128),
  };

  VmMemory memory = getVmMemory(vm);
  layoutBlocks(memory, blockStructure);

  EXPECT_EQ(currentVmmSize(memory), 1024);

  // Second and third blocks are on the address stack.  First and last
  // blocks are arguments to MKS2
  Stack addressStack = getVmAddressStack(vm);
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[2].address + sizeof(HeapBlock)
  ));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[1].address + sizeof(HeapBlock)
  ));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[3].address + sizeof(HeapBlock)
  ));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[0].address + sizeof(HeapBlock)
  ));

  // Call stack is empty

  // Execute the instruction
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 1);

  // Verify the heap
  EXPECT_EQ(currentVmmSize(memory), 2048);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 2048 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 2048 - (1024 + 40));

  // Expected heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64, 8),
    unl_test::BlockSpec(VmmCodeBlockType, 400, 80),
    unl_test::BlockSpec(VmmCodeBlockType, 392, 488),
    unl_test::BlockSpec(VmmCodeBlockType, 128, 888),
    unl_test::BlockSpec(VmmCodeBlockType, 24, 1024),
    unl_test::BlockSpec(VmmFreeBlockType, 984, 1056),
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 1056 };
  
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the result of MKS2 on the address stack
  ASSERT_EQ(stackSize(addressStack), 24);
  
  uint64_t* addressStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addressStackTop[-1],
	    trueHeapStructure[4].address + sizeof(HeapBlock));
  EXPECT_EQ(addressStackTop[-2],
	    trueHeapStructure[1].address + sizeof(HeapBlock));
  EXPECT_EQ(addressStackTop[-3],
	    trueHeapStructure[2].address + sizeof(HeapBlock));

  destroyUnlambdaVM(vm);
}

// Execute a MKS2 instruction causing the VM's memory to exceed its max size
TEST(vm_tests, executeMks2ExceedingMaxSize) {
  static const uint8_t PROGRAM[] = { MKS2_INSTRUCTION };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 1024);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Heap consists of four code blocks.  The first and fourth blocks
  // are referenced by the arguments to MKS2, while the middle two are
  // referenced from the address stack.  The VM will need to increase
  // the size of its memory so it can accomodate the allocation request
  // from MKS2, but will fail because the memory is already at its maxumum
  // size.
  std::vector<unl_test::BlockSpec> blockStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64),
    unl_test::BlockSpec(VmmCodeBlockType, 400),
    unl_test::BlockSpec(VmmCodeBlockType, 392),
    unl_test::BlockSpec(VmmCodeBlockType, 128),
  };

  VmMemory memory = getVmMemory(vm);
  layoutBlocks(memory, blockStructure);

  EXPECT_EQ(currentVmmSize(memory), 1024);

  // Second and third blocks are on the address stack.  First and last
  // blocks are arguments to MKS2
  Stack addressStack = getVmAddressStack(vm);
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[2].address + sizeof(HeapBlock)
  ));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[1].address + sizeof(HeapBlock)
  ));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[3].address + sizeof(HeapBlock)
  ));
  ASSERT_TRUE(unl_test::assertPushAddress(
    addressStack, blockStructure[0].address + sizeof(HeapBlock)
  ));

  // Call stack is empty

  // Execute the instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmOutOfMemoryError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "Could not allocate block of size 20 for MKS2 (Maximum memory "
	    "size exceeded)");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 0);

  // Expected heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64, 8),
    unl_test::BlockSpec(VmmCodeBlockType, 400, 80),
    unl_test::BlockSpec(VmmCodeBlockType, 392, 488),
    unl_test::BlockSpec(VmmCodeBlockType, 128, 888),
  };
  static const std::vector<uint64_t> trueFreeBlocks{ };
  
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify address stack is unchanged
  ASSERT_EQ(stackSize(addressStack), 32);
  
  uint64_t* addressStackTop =
    reinterpret_cast<uint64_t*>(topOfStack(addressStack));
  EXPECT_EQ(addressStackTop[-1],
	    trueHeapStructure[0].address + sizeof(HeapBlock));
  EXPECT_EQ(addressStackTop[-2],
	    trueHeapStructure[3].address + sizeof(HeapBlock));
  EXPECT_EQ(addressStackTop[-3],
	    trueHeapStructure[1].address + sizeof(HeapBlock));
  EXPECT_EQ(addressStackTop[-4],
	    trueHeapStructure[2].address + sizeof(HeapBlock));

  destroyUnlambdaVM(vm);
}

// Execute a SAVE instruction forcing garbage collection
TEST(vm_tests, executeSaveForcingGarbageCollection) {
  static const uint8_t PROGRAM[] = { SAVE_INSTRUCTION, 0 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Heap consists of four code blocks.  The first and second blocks
  // are referenced by the address stack, while the third and fourth
  // blocks are unreferenced.  After garbage collection, there will be
  // four blocks on the heap: the original first and second blocks,
  // the new state block created by SAVE and a free block.n
  std::vector<unl_test::BlockSpec> blockStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64),
    unl_test::BlockSpec(VmmCodeBlockType, 400),
    unl_test::BlockSpec(VmmCodeBlockType, 392),
    unl_test::BlockSpec(VmmCodeBlockType, 128),
  };

  VmMemory memory = getVmMemory(vm);
  layoutBlocks(memory, blockStructure);

  // First and second blocks go onto the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = {
    blockStructure[0].address + sizeof(HeapBlock),
    blockStructure[1].address + sizeof(HeapBlock)
  };
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));

  // Put four frames onto the call stack.  Be careful not to reference
  // the third or fourth blocks
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = { 16, 4, 88, 2, 5, 20, 16, 3 };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));

  // Execute the instruction
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 2);

  // Verify the heap
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 424);

  // Expected heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64, 8),
    unl_test::BlockSpec(VmmCodeBlockType, 400, 80),
    unl_test::BlockSpec(VmmStateBlockType, 96, 488),
    unl_test::BlockSpec(VmmFreeBlockType, 424, 592),
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 592 };
  
  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the contents of the saved state block
  ASSERT_TRUE(unl_test::verifyStateBlock(
    ptrToVmmAddress(memory, trueHeapStructure[2].address + sizeof(HeapBlock)),
    callStackData, callStackSize / 2, addressStackData, addressStackSize
  ));
	      
  destroyUnlambdaVM(vm);
}

// Execute a SAVE instruction forcing an increase in the size of the VM's memory
TEST(vm_tests, executeSaveForcingVmMemoryIncrease) {
  static const uint8_t PROGRAM[] = { SAVE_INSTRUCTION, 0 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 4096);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Heap consists of four code blocks.  All four blocks are referenced
  // from the address stack and some are referenced from the call stack
  // as well.  
  std::vector<unl_test::BlockSpec> blockStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64),
    unl_test::BlockSpec(VmmCodeBlockType, 400),
    unl_test::BlockSpec(VmmCodeBlockType, 392),
    unl_test::BlockSpec(VmmCodeBlockType, 128),
  };

  VmMemory memory = getVmMemory(vm);
  layoutBlocks(memory, blockStructure);

  // First and second blocks go onto the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = {
    blockStructure[0].address + sizeof(HeapBlock),
    blockStructure[1].address + sizeof(HeapBlock),
    blockStructure[2].address + sizeof(HeapBlock),
    blockStructure[3].address + sizeof(HeapBlock),
  };
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));

  // Put four frames onto the call stack.
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = {
    blockStructure[0].address + sizeof(HeapBlock),  4,
    blockStructure[1].address + sizeof(HeapBlock),  2,
    blockStructure[1].address + sizeof(HeapBlock), 20,
    blockStructure[0].address + sizeof(HeapBlock),  3
  };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));

  // Execute the instruction
  EXPECT_EQ(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  EXPECT_EQ(getVmPC(vm), 2);

  // Verify the heap
  EXPECT_EQ(currentVmmSize(memory), 2048);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 2048 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 896);

  // Expected heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType,   64,    8),
    unl_test::BlockSpec(VmmCodeBlockType,  400,   80),
    unl_test::BlockSpec(VmmCodeBlockType,  392,  488),
    unl_test::BlockSpec(VmmCodeBlockType,  128,  888),
    unl_test::BlockSpec(VmmStateBlockType, 112, 1024),
    unl_test::BlockSpec(VmmFreeBlockType,  896, 1144),
  };
  static const std::vector<uint64_t> trueFreeBlocks{ 1144 };

  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the call stack is unchanged
  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack, callStackData,
				    callStackSize));

  // Address stack should have the address of the state block on top
  uint64_t* trueAddrStackData = (uint64_t*)::malloc(8 * (addressStackSize + 1));
  ::memcpy(trueAddrStackData, addressStackData, 8 * addressStackSize);
  trueAddrStackData[addressStackSize] =
    trueHeapStructure[4].address + sizeof(HeapBlock);
  EXPECT_TRUE(unl_test::verifyStack("address stack", addressStack,
				    trueAddrStackData, addressStackSize + 1));
  ::free(trueAddrStackData);
  
  // Verify the contents of the saved state block
  ASSERT_TRUE(unl_test::verifyStateBlock(
    ptrToVmmAddress(memory, trueHeapStructure[4].address + sizeof(HeapBlock)),
    callStackData, callStackSize / 2, addressStackData, addressStackSize
  ));
	      
  destroyUnlambdaVM(vm);
}

// Execute a SAVE instruction causing the VM's memory to exceed its max size
TEST(vm_tests, executeSaveExceedingMaximumMemorySize) {
  static const uint8_t PROGRAM[] = { SAVE_INSTRUCTION, 0 };
  UnlambdaVM vm = createUnlambdaVM(16, 8, 1024, 1024);

  ASSERT_NE(vm, (void*)0);
  EXPECT_EQ(getVmStatus(vm), 0);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)), "OK");

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  // Heap consists of four code blocks.  All four blocks are referenced
  // from the address stack and some are referenced from the call stack
  // as well.  
  std::vector<unl_test::BlockSpec> blockStructure{
    unl_test::BlockSpec(VmmCodeBlockType, 64),
    unl_test::BlockSpec(VmmCodeBlockType, 400),
    unl_test::BlockSpec(VmmCodeBlockType, 392),
    unl_test::BlockSpec(VmmCodeBlockType, 128),
  };

  VmMemory memory = getVmMemory(vm);
  layoutBlocks(memory, blockStructure);

  // First and second blocks go onto the address stack
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t addressStackData[] = {
    blockStructure[0].address + sizeof(HeapBlock),
    blockStructure[1].address + sizeof(HeapBlock),
    blockStructure[2].address + sizeof(HeapBlock),
    blockStructure[3].address + sizeof(HeapBlock),
  };
  const uint64_t addressStackSize = ARRAY_SIZE(addressStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(addressStack, addressStackData,
				      addressStackSize));

  // Put four frames onto the call stack.
  Stack callStack = getVmCallStack(vm);
  const uint64_t callStackData[] = {
    blockStructure[0].address + sizeof(HeapBlock),  4,
    blockStructure[1].address + sizeof(HeapBlock),  2,
    blockStructure[1].address + sizeof(HeapBlock), 20,
    blockStructure[0].address + sizeof(HeapBlock),  3
  };
  const uint64_t callStackSize = ARRAY_SIZE(callStackData);
  ASSERT_TRUE(unl_test::pushOntoStack(callStack, callStackData, callStackSize));

  // Execute the instruction
  EXPECT_NE(stepVm(vm), 0);
  EXPECT_EQ(getVmStatus(vm), VmOutOfMemoryError);
  EXPECT_EQ(std::string(getVmStatusMsg(vm)),
	    "Could not allocate block of size 112 for SAVE (Maximum memory "
	    "size exceeded)");

  EXPECT_EQ(getVmPC(vm), 0);

  // Verify the heap
  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmAddressForPtr(memory, getVmmHeapStart(memory)), 8);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 8);
  EXPECT_EQ(vmmBytesFree(memory), 0);

  // Expected heap structure
  static const std::vector<unl_test::BlockSpec> trueHeapStructure{
    unl_test::BlockSpec(VmmCodeBlockType,   64,    8),
    unl_test::BlockSpec(VmmCodeBlockType,  400,   80),
    unl_test::BlockSpec(VmmCodeBlockType,  392,  488),
    unl_test::BlockSpec(VmmCodeBlockType,  128,  888),
  };
  static const std::vector<uint64_t> trueFreeBlocks{ };

  EXPECT_TRUE(unl_test::verifyBlockStructure(memory, trueHeapStructure));
  EXPECT_TRUE(unl_test::verifyFreeBlockList(memory, trueFreeBlocks));

  // Verify the call and address stacks are unchanged
  EXPECT_TRUE(unl_test::verifyStack("call stack", callStack, callStackData,
				    callStackSize));
  EXPECT_TRUE(unl_test::verifyStack("address stack", addressStack,
				    addressStackData, addressStackSize));

  destroyUnlambdaVM(vm);
}

// TODO:  Write a test that requires more than one increase in the VM memory
//        size to accomodate a large state block or run out of memory.
