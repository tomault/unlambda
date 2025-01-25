extern "C" {
#include <dbgcmd.h>
#include <vm.h>
}

#include <testing_utils.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>

namespace {
  class TestFailedException {
  public:
    TestFailedException(const std::string& details): details_(details) { }

    const std::string& details() const { return details_; }

  private:
    std::string details_;
  };

  std::string makeString(const char* s) {
    return s ? std::string(s) : "";
  }

  DebugCommand parseAndCheckCommand(UnlambdaVM vm,
				    const char* cmdText,
				    int32_t trueCmdType) {
    DebugCommand cmd = parseDebugCommand(vm, cmdText);
    if (!cmd) {
      throw TestFailedException("parseDebugCommand returned NULL");
    }

    if (cmd->cmd == DEBUG_CMD_PARSE_ERROR) {
      std::ostringstream msg;
      msg << "Parse error -- code = " << cmd->args.errorDetails.code
	  << ", details = [" << makeString(cmd->args.errorDetails.details)
	  << "]";
      destroyDebugCommand(cmd);
      throw TestFailedException(msg.str());
    }

    if (cmd->cmd != trueCmdType) {
      std::ostringstream msg;
      msg << "cmd->cmd is " << cmd->cmd << ", but it should be " << trueCmdType;
      destroyDebugCommand(cmd);
      throw TestFailedException(msg.str());
    }

    return cmd;
  }

  ::testing::AssertionResult testParseError(UnlambdaVM vm,
					    const char* cmdText,
					    int32_t errCode,
					    const std::string& errDetails) {
    DebugCommand cmd = parseDebugCommand(vm, cmdText);
    if (!cmd) {
      return ::testing::AssertionFailure()
	<< "parseDebugCommand() returned NULL";
    }
    
    if (cmd->cmd != DEBUG_CMD_PARSE_ERROR) {
      auto result = ::testing::AssertionFailure()
	<< "Parsing [" << cmdText << "] succeeded and produced a command "
	<< "with code " << cmd->cmd << ", but it should have failed "
	<< "with error code " << errCode << " and message [" << errDetails
	<< "]";
      destroyDebugCommand(cmd);
      return result;
    }

    if ((cmd->args.errorDetails.code != errCode)
	  || (makeString(cmd->args.errorDetails.details) != errDetails)) {
      auto result = ::testing::AssertionFailure()
	<< "Parsing [" << cmdText << "] failed with code="
	<< cmd->args.errorDetails.code << " and details ["
	<< makeString(cmd->args.errorDetails.details)
	<< "], but it should have failed with code = " << errCode
	<< " and details [" << errDetails << "]";
      destroyDebugCommand(cmd);
      return result;
    }

    destroyDebugCommand(cmd);
    return ::testing::AssertionSuccess();
  }
  
#define PARSE_AND_CHECK_COMMAND(VM, CMD_TEXT, CMD_TYPE)         \
  DebugCommand cmd = NULL;                                      \
  try {                                                         \
    cmd = parseAndCheckCommand(VM, CMD_TEXT, CMD_TYPE);         \
  } catch(const TestFailedException& e) {                       \
    return ::testing::AssertionFailure() << e.details();        \
  }

#define CHECK_CMD_ARGUMENT(CMD_ARG, TRUE_VALUE)	                \
  if (CMD_ARG != TRUE_VALUE) {                                  \
    auto result = ::testing::AssertionFailure()                 \
      << #CMD_ARG " is " << CMD_ARG                             \
      << ", but it should be " << TRUE_VALUE;                   \
    destroyDebugCommand(cmd);                                   \
    return result;                                              \
  }

#define CHECK_CMD_ARGUMENT_CSTR(CMD_ARG, TRUE_VALUE)            \
  if (!(TRUE_VALUE) && (CMD_ARG)) {				\
    auto result = ::testing::AssertionFailure()                 \
      << #CMD_ARG " is " << std::string(CMD_ARG)                \
      << ", but it should be NULL";                             \
    destroyDebugCommand(cmd);                                   \
    return result;                                              \
  } else if ((TRUE_VALUE) && !(CMD_ARG)) {			\
    auto result = ::testing::AssertionFailure()                 \
      << #CMD_ARG " is NULL, but it should be ["                \
      << std::string((TRUE_VALUE)) << "]";			\
    destroyDebugCommand(cmd);                                   \
    return result;                                              \
  } else if ((TRUE_VALUE) && (CMD_ARG)                          \
	      && (std::string((TRUE_VALUE)) != std::string((CMD_ARG)))) { \
    auto result = ::testing::AssertionFailure()                 \
      << #CMD_ARG " is [" << std::string((CMD_ARG))             \
      << "], but it should be [" << std::string((TRUE_VALUE))   \
      << "]";                                                   \
    destroyDebugCommand(cmd);                                   \
    return result;                                              \
  }

#define SUCCEED_AT_TEST()                                       \
  destroyDebugCommand(cmd);                                     \
  return ::testing::AssertionSuccess();
  
  ::testing::AssertionResult testAnyNoArgumentCmd(UnlambdaVM vm,
						  const char* cmdText,
						  int32_t trueCmdType) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, trueCmdType);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testDisassembleCmd(UnlambdaVM vm,
						const char* cmdText,
						uint64_t trueAddress,
						uint64_t trueNumLines) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, DISASSEMBLE_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.disassemble.address, trueAddress);
    CHECK_CMD_ARGUMENT(cmd->args.disassemble.numLines, trueNumLines);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testDumpBytesCmd(UnlambdaVM vm,
					      const char* cmdText,
					      uint64_t trueAddress,
					      uint32_t trueLength) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, DUMP_BYTES_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.dumpBytes.address, trueAddress);
    CHECK_CMD_ARGUMENT(cmd->args.dumpBytes.length, trueLength);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testWriteBytesCmd(UnlambdaVM vm,
					       const char* cmdText,
					       uint64_t trueAddress,
					       uint64_t trueLength,
					       const uint8_t* trueData) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, WRITE_BYTES_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.writeBytes.address, trueAddress);
    CHECK_CMD_ARGUMENT(cmd->args.writeBytes.length, trueLength);

    if (::memcmp(cmd->args.writeBytes.data, trueData, trueLength)) {
      auto result = ::testing::AssertionFailure()
	<< "cmd->args.writeBytes.data is "
	<< unlambda::testing::toString(cmd->args.writeBytes.data,
				       cmd->args.writeBytes.length)
	<< ", but it should be "
	<< unlambda::testing::toString(trueData, trueLength);
      destroyDebugCommand(cmd);
      return result;
    }
		 
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testDumpAddressStackCmd(UnlambdaVM vm,
						     const char* cmdText,
						     uint64_t trueDepth,
						     uint64_t trueCount) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, DUMP_ADDRESS_STACK_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.dumpStack.depth, trueDepth);
    CHECK_CMD_ARGUMENT(cmd->args.dumpStack.count, trueCount);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testModifyAddressStackCmd(UnlambdaVM vm,
							const char* cmdText,
							uint64_t trueDepth,
							uint64_t trueAddress) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, MODIFY_ADDRESS_STACK_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.modifyAddrStack.depth, trueDepth);
    CHECK_CMD_ARGUMENT(cmd->args.modifyAddrStack.address, trueAddress);
    SUCCEED_AT_TEST();
  }
  
  ::testing::AssertionResult testPushAddressStackCmd(UnlambdaVM vm,
						      const char* cmdText,
						      uint64_t trueAddress) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, PUSH_ADDRESS_STACK_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.pushAddrStack.address, trueAddress);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testDumpCallStackCmd(UnlambdaVM vm,
						  const char* cmdText,
						  uint64_t trueDepth,
						  uint64_t trueCount) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, DUMP_CALL_STACK_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.dumpStack.depth, trueDepth);
    CHECK_CMD_ARGUMENT(cmd->args.dumpStack.count, trueCount);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testModifyCallStackCmd(UnlambdaVM vm,
						    const char* cmdText,
						    uint64_t trueDepth,
						    uint64_t trueBlockAddress,
						    uint64_t trueRetAddress) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, MODIFY_CALL_STACK_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.modifyCallStack.depth, trueDepth);
    CHECK_CMD_ARGUMENT(cmd->args.modifyCallStack.blockAddress,
		       trueBlockAddress);
    CHECK_CMD_ARGUMENT(cmd->args.modifyCallStack.returnAddress, trueRetAddress);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testPushCallStackCmd(UnlambdaVM vm,
						  const char* cmdText,
						  uint64_t trueBlockAddress,
						  uint64_t trueRetAddress) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, PUSH_CALL_STACK_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.pushCallStack.blockAddress,
		       trueBlockAddress);
    CHECK_CMD_ARGUMENT(cmd->args.pushCallStack.returnAddress, trueRetAddress);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testAddBreakpointCmd(UnlambdaVM vm,
						  const char* cmdText,
						  uint64_t trueAddress) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, ADD_BREAKPOINT_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.addBreakpoint.address, trueAddress);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testRemoveBreakpointCmd(UnlambdaVM vm,
						     const char* cmdText,
						     uint64_t trueAddress) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, REMOVE_BREAKPOINT_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.removeBreakpoint.address, trueAddress);
    SUCCEED_AT_TEST();
  }
  
  ::testing::AssertionResult testRunCmd(UnlambdaVM vm,
					const char* cmdText,
					uint64_t trueAddress) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, RUN_PROGRAM_CMD);
    CHECK_CMD_ARGUMENT(cmd->args.run.address, trueAddress);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testHeapDumpCmd(UnlambdaVM vm,
					     const char* cmdText,
					     const char* trueFilename) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, HEAP_DUMP_CMD);
    CHECK_CMD_ARGUMENT_CSTR(cmd->args.heapDump.filename, trueFilename);
    SUCCEED_AT_TEST();
  }

  ::testing::AssertionResult testLookupSymbolCmd(UnlambdaVM vm,
						 const char* cmdText,
						 const char* trueName) {
    PARSE_AND_CHECK_COMMAND(vm, cmdText, LOOKUP_SYMBOL_CMD);
    CHECK_CMD_ARGUMENT_CSTR(cmd->args.lookupSymbol.name, trueName);
    SUCCEED_AT_TEST();
  }
}

TEST(dbgcmd_tests, parseDisassembleCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  setVmPC(vm, 82);

  EXPECT_TRUE(testDisassembleCmd(vm, "l 512 5\n", 512, 5));
  EXPECT_TRUE(testDisassembleCmd(vm, "l 512", 512, 10));
  EXPECT_TRUE(testDisassembleCmd(vm, "  l", getVmPC(vm), 10));
  EXPECT_TRUE(testParseError(vm, "l 1 2 3", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));

  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseDumpByesCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testDumpBytesCmd(vm, "d 638 16", 638, 16));
  EXPECT_TRUE(testDumpBytesCmd(vm, "d 99", 99, 256));
  EXPECT_TRUE(testParseError(vm, "d  ", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"address\" is missing"));
  EXPECT_TRUE(testParseError(vm, "d", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"address\" is missing"));
  EXPECT_TRUE(testParseError(vm, "d 683 16 24", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));

  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseWriteBytesCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  const uint8_t testData1[] = { 100, 255, 52, 48 };
  const uint8_t testData2[] = { 0 };

  EXPECT_TRUE(testWriteBytesCmd(vm, "w 768 100 255 52 48", 768,
				sizeof(testData1), testData1));
  EXPECT_TRUE(testWriteBytesCmd(vm, "w 1016 0", 1016, sizeof(testData2),
				testData2));
  EXPECT_TRUE(testParseError(vm, "w", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"address\" is missing"));
  EXPECT_TRUE(testParseError(vm, "w 756", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Bytes to write are missing"));
  EXPECT_TRUE(testParseError(vm, "w 128 256", DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid bytes to write (Value is too large)"));
  destroyUnlambdaVM(vm);
  // TODO: Test writing more than 65536 bytes
}

TEST(dbgcmd_tests, parseDumpAddressStackCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testDumpAddressStackCmd(vm, "as", 0, 16));
  EXPECT_TRUE(testDumpAddressStackCmd(vm, "as 4", 4, 16));
  EXPECT_TRUE(testDumpAddressStackCmd(vm, "as 4 2", 4, 2));
  EXPECT_TRUE(testParseError(vm, "as 4 2 24", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseModifyAddrStackCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testModifyAddressStackCmd(vm, "was 2 985", 2, 985));
  EXPECT_TRUE(testParseError(vm, "was", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"depth\" is missing"));
  EXPECT_TRUE(testParseError(vm, "was 0", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"address\" is missing"));
  EXPECT_TRUE(testParseError(vm, "was 2 985 4", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));	      
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parsePushAddreessCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testPushAddressStackCmd(vm, "pas 684", 684));
  EXPECT_TRUE(testParseError(vm, "pas", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"address\" is missing"));
  EXPECT_TRUE(testParseError(vm, "pas 684 685", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));	      
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parsePopAddressCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "ppas", POP_ADDRESS_STACK_CMD));
  EXPECT_TRUE(testParseError(vm, "ppas 2", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseDumpCallStackCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testDumpCallStackCmd(vm, "cs", 0, 16));
  EXPECT_TRUE(testDumpCallStackCmd(vm, "cs 3", 3, 16));
  EXPECT_TRUE(testDumpCallStackCmd(vm, "cs 3 8", 3, 8));
  EXPECT_TRUE(testParseError(vm, "cs 3 8 1", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseModifyCallStackCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testModifyCallStackCmd(vm, "wcs 1 540 17", 1, 540, 17));
  EXPECT_TRUE(testParseError(vm, "wcs", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"depth\" is missing"));
  EXPECT_TRUE(testParseError(vm, "wcs 1", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"block address\" is missing"));
  EXPECT_TRUE(testParseError(vm, "wcs 1 540", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"return address\" is "
			     "missing"));
  EXPECT_TRUE(testParseError(vm, "wcs 1 540 17 x", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parsePushCallStackCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testPushCallStackCmd(vm, "pcs 564 32", 564, 32));
  EXPECT_TRUE(testParseError(vm, "pcs", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"block address\" is missing"));
  EXPECT_TRUE(testParseError(vm, "pcs 564", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Required argument \"return address\" is "
			     "missing"));
  EXPECT_TRUE(testParseError(vm, "pcs 564 32 0", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parsePopCallStackCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "ppcs", POP_CALL_STACK_CMD));
  EXPECT_TRUE(testParseError(vm, "ppcs +1", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseListBreakpointsCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "b", LIST_BREAKPOINTS_CMD));
  EXPECT_TRUE(testParseError(vm, "b 1", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseAddBreakpointCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  setVmPC(vm, 123);

  EXPECT_TRUE(testAddBreakpointCmd(vm, "ba", 123));
  EXPECT_TRUE(testAddBreakpointCmd(vm, "ba 178", 178));
  EXPECT_TRUE(testParseError(vm, "ba 178 disabled",
			     DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseRemoveBreakpointCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  setVmPC(vm, 54);

  EXPECT_TRUE(testRemoveBreakpointCmd(vm, "bd", 54));
  EXPECT_TRUE(testRemoveBreakpointCmd(vm, "bd 999", 999));
  EXPECT_TRUE(testParseError(vm, "bd 999 54", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseRunCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  setVmPC(vm, 946);

  EXPECT_TRUE(testRunCmd(vm, "r", 946));
  EXPECT_TRUE(testRunCmd(vm, "r 123456", 123456));
  EXPECT_TRUE(testParseError(vm, "r 123 456", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseRunUntilReturnCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "rr", RUN_UNTIL_RETURN_CMD));
  EXPECT_TRUE(testParseError(vm, "rr 2", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseSingleStepIntoCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "s", SINGLE_STEP_INTO_CMD));
  EXPECT_TRUE(testParseError(vm, "s 2", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseSingleStepOverCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "ss", SINGLE_STEP_OVER_CMD));
  EXPECT_TRUE(testParseError(vm, "ss 2", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseHeapDumpCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testHeapDumpCmd(vm, "hd", NULL));
  EXPECT_TRUE(testHeapDumpCmd(vm, "hd abc.txt", "abc.txt"));
  EXPECT_TRUE(testHeapDumpCmd(vm, "hd abc.txt ", "abc.txt"));
  EXPECT_TRUE(testParseError(vm, "hd abc.txt full",
			     DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseQuitVmCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "q", QUIT_VM_CMD));
  EXPECT_TRUE(testParseError(vm, "q now", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));  
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseShowHelpCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testAnyNoArgumentCmd(vm, "h", SHOW_HELP_CMD));
  EXPECT_TRUE(testParseError(vm, "h breakpoints", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseLookupSymbolCmd) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testLookupSymbolCmd(vm, "sym dave_123", "dave_123"));
  EXPECT_TRUE(testParseError(vm, "sym", DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
			     "Symbol name missing"));
  EXPECT_TRUE(testParseError(vm, "sym 5abc", DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid symbol name"));
  EXPECT_TRUE(testParseError(vm, "sym dave+52",
			     DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid symbol name"));
  EXPECT_TRUE(testParseError(vm, "sym dave carol",
			     DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Too many arguments"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, resolveSymbolicAddress) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  SymbolTable symtab = getVmSymbolTable(vm);
  ASSERT_EQ(addSymbolToTable(symtab, "cow", 100), 0);
  ASSERT_EQ(addSymbolToTable(symtab, "penguin", 200), 0);

  EXPECT_TRUE(testPushAddressStackCmd(vm, "pas cow", 100));
  EXPECT_TRUE(testPushAddressStackCmd(vm, "pas penguin+50", 250));
  EXPECT_TRUE(testPushAddressStackCmd(vm, "pas penguin-10", 190));
  EXPECT_TRUE(testParseError(vm, "pas +50", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Invalid address (Syntax error)"));
  EXPECT_TRUE(testParseError(vm, "pas cat50", DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid address (Unknown symbol \"cat50\")"));
  EXPECT_TRUE(testParseError(vm, "pas cow[5]", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Invalid address (Syntax error)"));
  EXPECT_TRUE(testParseError(vm, "pas cow+", DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid address (Missing displacement value)"));
  EXPECT_TRUE(testParseError(vm, "pas cow-", DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid address (Missing displacement value)"));
  EXPECT_TRUE(testParseError(vm, "pas cow+penguin",
			     DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid address (Invalid displacement "
			     "(Value is not a number))"));
  EXPECT_TRUE(testParseError(vm, "pas penguin-cow",
			     DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid address (Invalid displacement "
			     "(Value is not a number))"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseInvalidIntegers) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testParseError(vm, "as abc", DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid depth (Value is not a number)"));
  EXPECT_TRUE(testParseError(vm, "as 52+5", DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid depth (Value is not a number)"));
  // Test input larger than 64 bits (21 decimal digits)
  EXPECT_TRUE(testParseError(vm, "as 12345678912345678954227",
			     DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid depth (Value is too large)"));

  // Test number too large to be a 32-bit integer
  EXPECT_TRUE(testParseError(vm, "l 0 4294967296",
			     DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
			     "Invalid number of lines (Value is too large)"));
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseEmptyCommands) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  DebugCommand cmd = parseDebugCommand(vm, "");
  EXPECT_EQ(cmd, (void*)0);

  cmd = parseDebugCommand(vm, " ");
  EXPECT_EQ(cmd, (void*)0);

  cmd = parseDebugCommand(vm, "\t   \t ");
  EXPECT_EQ(cmd, (void*)0);
  destroyUnlambdaVM(vm);
}

TEST(dbgcmd_tests, parseInvalidCommands) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  EXPECT_TRUE(testParseError(vm, "?", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Syntax error"));
  EXPECT_TRUE(testParseError(vm, "+b 50", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Syntax error"));
  EXPECT_TRUE(testParseError(vm, "as=50", DEBUG_CMD_PARSE_SYNTAX_ERROR,
			     "Syntax error"));
  EXPECT_TRUE(testParseError(vm, "badcmd", DEBUG_CMD_UNKNOWN_CMD_ERROR,
			     "Unknown command \"badcmd\".  Use h to print "
			     "a list of commands"));
  destroyUnlambdaVM(vm);
}
