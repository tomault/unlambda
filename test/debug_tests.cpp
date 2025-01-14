extern "C" {
#include <dbgcmd.h>
#include <debug.h>
#include <vm.h>
#include <vm_instructions.h>
}

#include <testing_utils.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

namespace unl_test = unlambda::testing;

namespace {
  std::string toString(DebugCommand cmd) {
    char buf[1024];
    sprintDebugCommand(cmd, buf, sizeof(buf));
    return std::string(buf);
  }
  
  ::testing::AssertionResult verifyExecutionFailure(
      Debugger dbg, DebugCommand cmd, int32_t errorCode,
      const std::string& errorMsg
  ) {
    if (!executeDebugCommand(dbg, cmd)) {
      auto result = ::testing::AssertionFailure()
	<< "Command [" << toString(cmd) << "] succeeded, but it should have "
	<< "failed with code " << errorCode << " and error message ["
	<< errorMsg << "]";
      destroyDebugCommand(cmd);
      return result;
    }

    if ((getDebuggerStatus(dbg) != errorCode)
	  || (std::string(getDebuggerStatusMsg(dbg)) != errorMsg)){
      auto result = ::testing::AssertionFailure()
	<< "Command [" << toString(cmd) << "] failed with code "
	<< getDebuggerStatus(dbg) << " and message ["
	<< std::string(getDebuggerStatusMsg(dbg)) << "], but it should "
	<< "have failed with code " << errorCode << " and message ["
	<< errorMsg << "]";
      destroyDebugCommand(cmd);
      return result;
    }

    destroyDebugCommand(cmd);
    return ::testing::AssertionSuccess();
  }

  ::testing::AssertionResult verifyBreakpoints(BreakpointList breakpoints,
					       const uint64_t* trueBreakpoints,
					       const uint64_t trueSize) {
    // Verify size
    if (breakpointListSize(breakpoints) != trueSize) {
      return ::testing::AssertionFailure()
	<< "Breakpoint list has size " << breakpointListSize(breakpoints)
	<< ", but it should have size " << trueSize;
    }

    // Verify content
    const uint64_t* blAddresses = startOfBreakpointList(breakpoints);
    for (uint32_t i = 0; i < trueSize; ++i) {
      if (blAddresses[i] != trueBreakpoints[i]) {
	return ::testing::AssertionFailure()
	  << "Breakpoint list is "
	  << unl_test::toString(blAddresses, breakpointListSize(breakpoints))
	  << ", but it should be "
	  << unl_test::toString(trueBreakpoints, trueSize);
      }
    }

    return ::testing::AssertionSuccess();
  }
  
}

TEST(debug_tests, executeDisassembleCmd) {
  const Symbol SYMBOLS[] = {
    { "V_IMPL", 128 },
    { "C_IMPL", 140 },
    { "AX0", 9 },
    { NULL, 0 },
  };
  const uint8_t PROGRAM[] = {
    PUSH_INSTRUCTION, 128, 0, 0, 0, 0, 0, 0, 0,
    MKK_INSTRUCTION,
    PUSH_INSTRUCTION, 0, 3, 0, 0, 0, 0, 0, 0,
    PCALL_INSTRUCTION,
    RET_INSTRUCTION,   
  };

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  SymbolTable symtab = getVmSymbolTable(vm);

  for (const Symbol* s = SYMBOLS; s->name; ++s) {
    ASSERT_EQ(addSymbolToTable(symtab, s->name, s->address), 0);
  }

  ASSERT_EQ(loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				    sizeof(PROGRAM)), 0);

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createDisassembleCommand(9, 3);
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);

  destroyDebugCommand(cmd);

  // Disassemble from invalid address
  EXPECT_TRUE(verifyExecutionFailure(dbg, createDisassembleCommand(1025, 1),
				     DebuggerInvalidCommandError,
				     "Invalid address 1025"));
  
  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeDumpBytesCmd) {
  const uint8_t DATA[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED, 0xBE, 0xAD,
    0xDD, 0xEE, 0xAA, 0xDD, 0xBB, 0xEE, 0xEE, 0xFF,
  };
  const uint64_t VM_SIZE = 1024;
  const uint64_t DATA_ADDRESS = VM_SIZE - sizeof(DATA);
  UnlambdaVM vm = createUnlambdaVM(16, 16, VM_SIZE, VM_SIZE);
  uint8_t* memp = ptrToVmAddress(vm, DATA_ADDRESS);

  ::memcpy(memp, DATA, sizeof(DATA));

  // TODO: Give the VM a stream for its output and have the debugger
  //       use that stream for its output.  Write command output to
  //       a file and verify that.  For now, just look at the output
  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createDumpBytesCommand(DATA_ADDRESS, sizeof(DATA));
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  std::cout << "\n******\n" << std::endl;
  
  // Test dump past the end of memory
  cmd = createDumpBytesCommand(DATA_ADDRESS, 2 * sizeof(DATA));
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  // Test dump from invalid address.  verifyExecutionFailure() destroys
  // the command
  cmd = createDumpBytesCommand(VM_SIZE + 1, 1);
  EXPECT_TRUE(verifyExecutionFailure(dbg, cmd, DebuggerInvalidCommandError,
				     "Invalid address 1025"));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeModifyBytesCmd) {
  const uint8_t DATA[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED, 0xBE, 0xAD,
    0xDD, 0xEE, 0xAA, 0xDD, 0xBB, 0xEE, 0xEE, 0xFF,
  };
  const uint8_t NEW_DATA[] = {
    0x11, 0x33, 0x55, 0x77, 0x88, 0x66, 0x44, 0x22,
  };
  const uint8_t RESULT[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x33, 0x55, 0x77,
    0x88, 0x66, 0x44, 0x22, 0xBB, 0xEE, 0xEE, 0xFF,
  };
  const uint64_t VM_SIZE = 1024;
  const uint64_t DATA_ADDRESS = VM_SIZE - sizeof(DATA);
  UnlambdaVM vm = createUnlambdaVM(16, 16, VM_SIZE, VM_SIZE);
  uint8_t* memp = ptrToVmAddress(vm, DATA_ADDRESS);

  ::memcpy(memp, DATA, sizeof(DATA));

  // TODO: Give the VM a stream for its output and have the debugger
  //       use that stream for its output.  Write command output to
  //       a file and verify that.  For now, just look at the output
  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createWriteBytesCommand(
    DATA_ADDRESS + 4, sizeof(NEW_DATA), NEW_DATA
  );
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  EXPECT_TRUE(unl_test::verifyBytes(memp, RESULT, sizeof(RESULT)));
  destroyDebugCommand(cmd);

  // Test write past the end of memory.  verifyExecutionFailure() destroys
  // the command
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createWriteBytesCommand(VM_SIZE - sizeof(NEW_DATA) + 1,
				 sizeof(NEW_DATA), NEW_DATA),
    DebuggerInvalidCommandError, "Write extends outside VM memory"
  ));

  // Test write to invalid address.  verifyExecutionFailure() destroys
  // the command
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createWriteBytesCommand(VM_SIZE + 1, 1, NEW_DATA),
    DebuggerInvalidCommandError, "Invalid address 1025"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeDumpAddressStackCmd) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,
				  0xDDEEAADDBBEEEEFF,
				  0xFFEEEEDDBBEEAADD,
				  0x1133557788664422 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmAddressStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createDumpAddressStackCommand(1, 2);
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  std::cout << "\n******\n" << std::endl;

  // Test dumpping past the bottom of the stack stops at the bottom
  cmd = createDumpAddressStackCommand(1, ARRAY_SIZE(STACK_DATA));
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  // Test dumping a frame outside the stack
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createDumpAddressStackCommand(ARRAY_SIZE(STACK_DATA), 1),
    DebuggerInvalidCommandError, "Address stack only has 4 addresses"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeModifyAddressStackCommand) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,
				  0xDDEEAADDBBEEEEFF,
				  0xFFEEEEDDBBEEAADD,
				  0x1133557788664422 };
  const uint64_t NEW_ADDRESS = 0x8866442200AABBCC;
  const uint64_t NEW_STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,
				      NEW_ADDRESS,
				      0xFFEEEEDDBBEEAADD,
				      0x1133557788664422 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmAddressStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createModifyAddressStackCommand(2, NEW_ADDRESS);
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  EXPECT_TRUE(unl_test::verifyStack("address", s, NEW_STACK_DATA,
				    ARRAY_SIZE(NEW_STACK_DATA)));
  destroyDebugCommand(cmd);

  // Modifying a frame past the bottom should fail
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createModifyAddressStackCommand(ARRAY_SIZE(STACK_DATA), NEW_ADDRESS),
    DebuggerInvalidCommandError, "Address stack only has 4 addresses"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executePushAddressStackCmd) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,
				  0xDDEEAADDBBEEEEFF,
				  0xFFEEEEDDBBEEAADD,
				  0x1133557788664422 };
  const uint64_t NEW_ADDRESS = 0x8866442200AABBCC;
  const uint64_t NEW_STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,
				      0xDDEEAADDBBEEEEFF,
				      0xFFEEEEDDBBEEAADD,
				      0x1133557788664422,
				      NEW_ADDRESS };  
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmAddressStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createPushAddressStackCommand(NEW_ADDRESS);
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  EXPECT_TRUE(unl_test::verifyStack("address", s, NEW_STACK_DATA,
				    ARRAY_SIZE(NEW_STACK_DATA)));
  destroyDebugCommand(cmd);

  ASSERT_EQ(stackMaxSize(s), 16 * 8);
  
  while (stackSize(s) < stackMaxSize(s)) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[0], sizeof(uint64_t)), 0);
  }

  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createPushAddressStackCommand(0), DebuggerCommandExecutionError,
    "Push to address stack failed (Stack overflow - increasing the size of "
    "the stack by 8 bytes would exceed the maximum size of 128 bytes)"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executePopAddressStackCmd) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,
				  0xDDEEAADDBBEEEEFF,
				  0xFFEEEEDDBBEEAADD,
				  0x1133557788664422 };
  const uint64_t NEW_ADDRESS = 0x8866442200AABBCC;
  const uint64_t NEW_STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,
				      0xDDEEAADDBBEEEEFF,
				      0xFFEEEEDDBBEEAADD };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmAddressStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createPopAddressStackCommand();
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  EXPECT_TRUE(unl_test::verifyStack("address", s, NEW_STACK_DATA,
				    ARRAY_SIZE(NEW_STACK_DATA)));
  destroyDebugCommand(cmd);

  // Empty the stack and try to pop
  clearStack(s);

  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createPopAddressStackCommand(), DebuggerCommandExecutionError,
    "Pop from address stack failed (Cannot pop 8 bytes from a stack with only "
    "0 bytes on it)"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeDumpCallStackCmd) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,  71,
				  0xDDEEAADDBBEEEEFF,  42,
				  0xFFEEEEDDBBEEAADD, 129,
				  0x1133557788664422,  14 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmCallStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createDumpCallStackCommand(1, 2);
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  std::cout << "\n******\n" << std::endl;

  // Test dumpping past the bottom of the stack stops at the bottom
  cmd = createDumpCallStackCommand(1, ARRAY_SIZE(STACK_DATA));
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);
  
  // Test dump a frame outside the stack
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createDumpCallStackCommand(ARRAY_SIZE(STACK_DATA) / 2, 1),
    DebuggerInvalidCommandError, "Call stack only has 4 frames"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeModifyCallStackCmd) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,  71,
				  0xDDEEAADDBBEEEEFF,  42,
				  0xFFEEEEDDBBEEAADD, 129,
				  0x1133557788664422,  14 };
  const uint64_t NEW_BLOCK_ADDRESS = 0x88664422AABBCCDD;
  const uint64_t NEW_RETURN_ADDRESS = 99;
  const uint64_t NEW_STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,  71,
				      0xDDEEAADDBBEEEEFF,  42,
				      NEW_BLOCK_ADDRESS,   NEW_RETURN_ADDRESS,
				      0x1133557788664422,  14 };
  
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmCallStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createModifyCallStackCommand(1, NEW_BLOCK_ADDRESS,
						  NEW_RETURN_ADDRESS);
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  EXPECT_TRUE(unl_test::verifyStack("call", s, NEW_STACK_DATA,
				    ARRAY_SIZE(NEW_STACK_DATA)));
  destroyDebugCommand(cmd);

  // Test modifying a frame outside the stack
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createModifyCallStackCommand(ARRAY_SIZE(STACK_DATA) / 2,
				      NEW_BLOCK_ADDRESS, NEW_RETURN_ADDRESS),
    DebuggerInvalidCommandError, "Call stack only has 4 frames"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);  
}

TEST(debug_tests, executePushCallStackCmd) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,  71,
				  0xDDEEAADDBBEEEEFF,  42,
				  0xFFEEEEDDBBEEAADD, 129,
				  0x1133557788664422,  14 };
  const uint64_t NEW_BLOCK_ADDRESS = 0x88664422AABBCCDD;
  const uint64_t NEW_RETURN_ADDRESS = 99;
  const uint64_t NEW_STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,  71,
				      0xDDEEAADDBBEEEEFF,  42,
				      0xFFEEEEDDBBEEAADD, 129,
				      0x1133557788664422,  14,
				      NEW_BLOCK_ADDRESS,   NEW_RETURN_ADDRESS };
  
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmCallStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createPushCallStackCommand(NEW_BLOCK_ADDRESS,
						NEW_RETURN_ADDRESS);
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  EXPECT_TRUE(unl_test::verifyStack("call", s, NEW_STACK_DATA,
				    ARRAY_SIZE(NEW_STACK_DATA)));
  destroyDebugCommand(cmd);

  // Fill the stack and try to push a new frame
  ASSERT_EQ(stackMaxSize(s), 16 * 16);
  while(stackSize(s) < stackMaxSize(s)) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[0], 2 * sizeof(uint64_t)), 0);
  }
  
  // Test modifying a frame outside the stack
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createPushCallStackCommand(NEW_BLOCK_ADDRESS, NEW_RETURN_ADDRESS),
    DebuggerCommandExecutionError,
    "Push to call stack failed (Stack overflow - increasing the size of "
    "the stack by 8 bytes would exceed the maximum size of 256 bytes)"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);  
}

TEST(debug_tests, executePopCallStackCmd) {
  const uint64_t STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,  71,
				  0xDDEEAADDBBEEEEFF,  42,
				  0xFFEEEEDDBBEEAADD, 129,
				  0x1133557788664422,  14 };
  const uint64_t NEW_STACK_DATA[] = { 0xDEADBEEFFEEDBEAD,  71,
				      0xDDEEAADDBBEEEEFF,  42,
				      0xFFEEEEDDBBEEAADD, 129 };
  
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Stack s = getVmCallStack(vm);

  for (int i = 0; i < ARRAY_SIZE(STACK_DATA); ++i) {
    ASSERT_EQ(pushStack(s, &STACK_DATA[i], sizeof(uint64_t)), 0);
  }

  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createPopCallStackCommand();
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  EXPECT_TRUE(unl_test::verifyStack("call", s, NEW_STACK_DATA,
				    ARRAY_SIZE(NEW_STACK_DATA)));
  destroyDebugCommand(cmd);

  // Empty the stack then try to pop
  clearStack(s);
  
  // Test modifying a frame outside the stack
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createPopCallStackCommand(), DebuggerCommandExecutionError,
    "Pop from call stack failed (Cannot pop 8 bytes from a stack with "
    "only 0 bytes on it)"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);  
}

TEST(debug_tests, executeListBreakpoints) {
  const uint64_t PERSISTENT_BREAKPOINTS[] = { 5, 17, 104, 99 };
  const uint64_t TRANSIENT_BREAKPOINTS[] = { 128, 89 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Debugger dbg = createDebugger(vm, 32);
  BreakpointList persistentBreakpoints = getDebuggerPersistentBreakpoints(dbg);
  BreakpointList transientBreakpoints = getDebuggerTransientBreakpoints(dbg);

  for (int i = 0; i < ARRAY_SIZE(PERSISTENT_BREAKPOINTS); ++i) {
    addBreakpointToList(persistentBreakpoints, PERSISTENT_BREAKPOINTS[i]);
  }

  for (int i = 0; i < ARRAY_SIZE(TRANSIENT_BREAKPOINTS); ++i) {
    addBreakpointToList(transientBreakpoints, TRANSIENT_BREAKPOINTS[i]);
  }

  DebugCommand cmd = createListBreakpointsCommand();
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeAddBreakpoint) {
  const uint64_t PERSISTENT_BREAKPOINTS[] = { 5, 17, 104, 99 };
  const uint64_t TRANSIENT_BREAKPOINTS[] = { 128, 89 };
  const uint64_t NEW_BREAKPOINT = 55;
  const uint64_t NEW_PERSISTENT_BREAKPOINTS[] = { 5, 17, 55, 99, 104 };
  const uint64_t ORDERED_TRANSIENT_BREAKPOINTS[] = { 89, 128 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Debugger dbg = createDebugger(vm, ARRAY_SIZE(PERSISTENT_BREAKPOINTS) + 1);
  BreakpointList persistentBreakpoints = getDebuggerPersistentBreakpoints(dbg);
  BreakpointList transientBreakpoints = getDebuggerTransientBreakpoints(dbg);

  for (int i = 0; i < ARRAY_SIZE(PERSISTENT_BREAKPOINTS); ++i) {
    addBreakpointToList(persistentBreakpoints, PERSISTENT_BREAKPOINTS[i]);
  }

  for (int i = 0; i < ARRAY_SIZE(TRANSIENT_BREAKPOINTS); ++i) {
    addBreakpointToList(transientBreakpoints, TRANSIENT_BREAKPOINTS[i]);
  }

  // Add a new breakpoint & verify
  DebugCommand cmd = createAddBreakpointCommand(NEW_BREAKPOINT);
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  EXPECT_TRUE(verifyBreakpoints(persistentBreakpoints,
				NEW_PERSISTENT_BREAKPOINTS,
				ARRAY_SIZE(NEW_PERSISTENT_BREAKPOINTS)));
  EXPECT_TRUE(verifyBreakpoints(transientBreakpoints,
				ORDERED_TRANSIENT_BREAKPOINTS,
				ARRAY_SIZE(ORDERED_TRANSIENT_BREAKPOINTS)));
  

  // Breakpoint list should be full, so adding another should fail
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createAddBreakpointCommand(1), DebuggerCommandExecutionError,
    "Failed to add breakpoint (Breakpoint list is full)"
  ));

  EXPECT_TRUE(verifyBreakpoints(persistentBreakpoints,
				NEW_PERSISTENT_BREAKPOINTS,
				ARRAY_SIZE(NEW_PERSISTENT_BREAKPOINTS)));
  EXPECT_TRUE(verifyBreakpoints(transientBreakpoints,
				ORDERED_TRANSIENT_BREAKPOINTS,
				ARRAY_SIZE(ORDERED_TRANSIENT_BREAKPOINTS)));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeRemoveBreakpoint) {
  const uint64_t PERSISTENT_BREAKPOINTS[] = { 5, 17, 104, 99 };
  const uint64_t TRANSIENT_BREAKPOINTS[] = { 128, 89 };
  const uint64_t NEW_PERSISTENT_BREAKPOINTS[] = { 5, 99, 104 };
  const uint64_t ORDERED_TRANSIENT_BREAKPOINTS[] = { 89, 128 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Debugger dbg = createDebugger(vm, ARRAY_SIZE(PERSISTENT_BREAKPOINTS) + 1);
  BreakpointList persistentBreakpoints = getDebuggerPersistentBreakpoints(dbg);
  BreakpointList transientBreakpoints = getDebuggerTransientBreakpoints(dbg);

  for (int i = 0; i < ARRAY_SIZE(PERSISTENT_BREAKPOINTS); ++i) {
    addBreakpointToList(persistentBreakpoints, PERSISTENT_BREAKPOINTS[i]);
  }

  for (int i = 0; i < ARRAY_SIZE(TRANSIENT_BREAKPOINTS); ++i) {
    addBreakpointToList(transientBreakpoints, TRANSIENT_BREAKPOINTS[i]);
  }

  // Remove an existing breakpoint & verify
  DebugCommand cmd = createRemoveBreakpointCommand(PERSISTENT_BREAKPOINTS[1]);
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  EXPECT_TRUE(verifyBreakpoints(persistentBreakpoints,
				NEW_PERSISTENT_BREAKPOINTS,
				ARRAY_SIZE(NEW_PERSISTENT_BREAKPOINTS)));
  EXPECT_TRUE(verifyBreakpoints(transientBreakpoints,
				ORDERED_TRANSIENT_BREAKPOINTS,
				ARRAY_SIZE(ORDERED_TRANSIENT_BREAKPOINTS)));

  // Remove a non-exsitent breakpoint (no-op)
  cmd = createRemoveBreakpointCommand(100);
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  EXPECT_TRUE(verifyBreakpoints(persistentBreakpoints,
				NEW_PERSISTENT_BREAKPOINTS,
				ARRAY_SIZE(NEW_PERSISTENT_BREAKPOINTS)));
  EXPECT_TRUE(verifyBreakpoints(transientBreakpoints,
				ORDERED_TRANSIENT_BREAKPOINTS,
				ARRAY_SIZE(ORDERED_TRANSIENT_BREAKPOINTS)));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeRunCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Debugger dbg = createDebugger(vm, 32);

  ASSERT_EQ(getDebuggerStatus(dbg), 0);
  ASSERT_EQ(getVmPC(vm), 0);
  ASSERT_FALSE(shouldBreakExecution(dbg));

  DebugCommand cmd = createRunCommand(52);
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0);
  destroyDebugCommand(cmd);

  EXPECT_EQ(getDebuggerStatus(dbg), DebuggerResumeExecution);
  EXPECT_FALSE(shouldBreakExecution(dbg));
  EXPECT_EQ(getVmPC(vm), 52);

  // Cannot resume execution at an invalid address
  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createRunCommand(1025), DebuggerInvalidCommandError,
    "Cannot resume execution at invalid address 1025"
  ));
  
  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeRunUntilReturnCmd) {
  const uint64_t CALL_STACK_FRAMES[] = { 0xDEADBEEFFEEDBEAD, 52,
					 0x11223344AABBCCDD, 75 };
  const uint64_t NEW_TRANSIENT_BREAKPOINTS[] = { 75 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Debugger dbg = createDebugger(vm, 32);
  BreakpointList persistentBreakpoints = getDebuggerPersistentBreakpoints(dbg);
  BreakpointList transientBreakpoints = getDebuggerTransientBreakpoints(dbg);
  Stack s = getVmCallStack(vm);

  ASSERT_EQ(pushStack(s, CALL_STACK_FRAMES, sizeof(CALL_STACK_FRAMES)), 0);

  ASSERT_EQ(getDebuggerStatus(dbg), 0);
  ASSERT_EQ(getVmPC(vm), 0);
  ASSERT_EQ(shouldBreakExecution(dbg), 0);

  DebugCommand cmd = createRunUntilReturnCommand();
  EXPECT_EQ(executeDebugCommand(dbg, cmd), 0)
    << "executeDebugCommand(dbg, cmd) failed (code = "
    << getDebuggerStatus(dbg)
    << ", details=[" << getDebuggerStatusMsg(dbg) << "]" << std::endl;
  destroyDebugCommand(cmd);

  EXPECT_TRUE(verifyBreakpoints(persistentBreakpoints, NULL, 0));
  EXPECT_TRUE(verifyBreakpoints(transientBreakpoints, NEW_TRANSIENT_BREAKPOINTS,
				ARRAY_SIZE(NEW_TRANSIENT_BREAKPOINTS)));

  EXPECT_EQ(getVmPC(vm), 0);
  EXPECT_EQ(getDebuggerStatus(dbg), DebuggerResumeExecution);
  EXPECT_FALSE(shouldBreakExecution(dbg));

  // Clear the call stack and try again.  The command should fail on an
  // empty call stack
  clearStack(s);
  clearDebuggerStatus(dbg);

  EXPECT_TRUE(verifyExecutionFailure(
    dbg, createRunUntilReturnCommand(), DebuggerCommandExecutionError,
    "Call stack is empty"
  ));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeSingleStepIntoCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Debugger dbg = createDebugger(vm, 32);

  ASSERT_EQ(getDebuggerStatus(dbg), 0);
  ASSERT_EQ(getVmPC(vm), 0);
  ASSERT_EQ(shouldBreakExecution(dbg), 0);
  
  DebugCommand cmd = createSingleStepIntoCommand();
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0)
    << "executeDebugCommand(dbg, cmd) failed (code = " << getDebuggerStatus(dbg)
    << ", details=[" << getDebuggerStatusMsg(dbg) << "]" << std::endl;
  destroyDebugCommand(cmd);

  EXPECT_EQ(getVmPC(vm), 0);
  EXPECT_EQ(getDebuggerStatus(dbg), DebuggerResumeExecution);
  EXPECT_TRUE(shouldBreakExecution(dbg));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeSingleStepOverCmd) {
  const uint8_t PROGRAM[] = {
    PUSH_INSTRUCTION, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    PCALL_INSTRUCTION,
    RET_INSTRUCTION
  };
  const uint64_t TRANSIENT_BREAKPOINTS[] = { 9 };
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  ASSERT_TRUE(!loadVmProgramFromMemory(vm, "test_program", PROGRAM,
				       sizeof(PROGRAM)));
  setVmPC(vm, 0);

  Debugger dbg = createDebugger(vm, 32);
  BreakpointList persistentBreakpoints = getDebuggerPersistentBreakpoints(dbg);
  BreakpointList transientBreakpoints = getDebuggerTransientBreakpoints(dbg);

  DebugCommand cmd = createSingleStepOverCommand();
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0)
    << "executeDebugCommand(dbg, cmd) failed (code = " << getDebuggerStatus(dbg)
    << ", details=[" << getDebuggerStatusMsg(dbg) << "]" << std::endl;
  destroyDebugCommand(cmd);

  EXPECT_EQ(getDebuggerStatus(dbg), DebuggerResumeExecution);
  EXPECT_EQ(getVmPC(vm), 0);
  EXPECT_FALSE(shouldBreakExecution(dbg));
  EXPECT_TRUE(verifyBreakpoints(persistentBreakpoints, NULL, 0));
  EXPECT_TRUE(verifyBreakpoints(transientBreakpoints, TRANSIENT_BREAKPOINTS,
				ARRAY_SIZE(TRANSIENT_BREAKPOINTS)));

  // Test step over at end of memory
  clearBreakpointList(transientBreakpoints);
  clearDebuggerStatus(dbg);
  
  *(ptrToVmAddress(vm, 1023)) = PCALL_INSTRUCTION;
  setVmPC(vm, 1023);
  cmd = createSingleStepOverCommand();
  
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0)
    << "executeDebugCommand(dbg, cmd) failed (code = " << getDebuggerStatus(dbg)
    << ", details=[" << getDebuggerStatusMsg(dbg) << "]" << std::endl;
  destroyDebugCommand(cmd);

  EXPECT_EQ(getDebuggerStatus(dbg), DebuggerResumeExecution);
  EXPECT_EQ(getVmPC(vm), 1023);
  EXPECT_FALSE(shouldBreakExecution(dbg));
  EXPECT_TRUE(verifyBreakpoints(persistentBreakpoints, NULL, 0));

  // 1024 is not a valid address, so it should not be in the transient
  // breakpoint list
  EXPECT_TRUE(verifyBreakpoints(transientBreakpoints, NULL, 0));

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);  
}

TEST(debug_tests, executeQuitVmCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  Debugger dbg = createDebugger(vm, 32);

  ASSERT_EQ(getDebuggerStatus(dbg), 0);

  DebugCommand cmd = createQuitVMCommand();
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0)
    << "executeDebugCommand(dbg, cmd) failed (code = " << getDebuggerStatus(dbg)
    << ", details=[" << getDebuggerStatusMsg(dbg) << "]" << std::endl;
  destroyDebugCommand(cmd);

  EXPECT_EQ(getDebuggerStatus(dbg), DebuggerQuitVm);

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
}

TEST(debug_tests, executeLookupSymbolCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  SymbolTable symtab = getVmSymbolTable(vm);

  ASSERT_TRUE(!addSymbolToTable(symtab, "cow", 52));
  ASSERT_TRUE(!addSymbolToTable(symtab, "penguin", 99));
  
  Debugger dbg = createDebugger(vm, 32);
  DebugCommand cmd = createLookupSymbolCommand("cow");
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0)
    << "executeDebugCommand(dbg, cmd) failed (code = " << getDebuggerStatus(dbg)
    << ", details=[" << getDebuggerStatusMsg(dbg) << "]" << std::endl;
  destroyDebugCommand(cmd);

  cmd = createLookupSymbolCommand("moo");
  ASSERT_EQ(executeDebugCommand(dbg, cmd), 0)
    << "executeDebugCommand(dbg, cmd) failed (code = " << getDebuggerStatus(dbg)
    << ", details=[" << getDebuggerStatusMsg(dbg) << "]" << std::endl;
  destroyDebugCommand(cmd);
  
  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);  
}
