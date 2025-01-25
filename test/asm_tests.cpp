extern "C" {
#include <asm.h>
#include <symtab.h>
#include <vm_instructions.h>
}

#include <gtest/gtest.h>
#include <iostream>
#include <string>

#include <assert.h>
#include <stdint.h>

namespace {
  ::testing::AssertionResult verifySuccessfulParse(
    AssemblyLine* asml, AsmParseError* parseError, uint16_t expectedType,
    uint64_t expectedAddress, uint32_t expectedLine, uint16_t expectedColumn,
    const char* expectedComment
  ) {
    if (parseError) {
      return ::testing::AssertionFailure()
	<< "Parse produced an error at column " << parseError->column
	<< " (" << parseError->message << ")";
    }
    
    if (!asml) {
      return ::testing::AssertionFailure() << "Parse result is NULL";
    }

    if (asml->type != expectedType) {
      return ::testing::AssertionFailure()
	<< "Parse produced an AssemblyLine of type " << asml->type
	<< ", but it should be " << expectedType;
    }

    if (asml->address != expectedAddress) {
      return ::testing::AssertionFailure()
	<< "Parse produced an AssemblyLine at address " << asml->address
	<< ", but it should be at address " << expectedAddress;
    }

    if (asml->line != expectedLine) {
      return ::testing::AssertionFailure()
	<< "Parse produced an AssemblyLine at line " << asml->line
	<< ", but it should be at line " << expectedLine;
    }

    if (asml->column != expectedColumn) {
      return ::testing::AssertionFailure()
	<< "Parse produced an AssemblyLine at column " << asml->column
	<< ", but it should be at column " << expectedColumn;
    }

    if (!expectedComment && asml->comment) {
      return ::testing::AssertionFailure()
	<< "Parse produced an AssemblyLine with a comment \""
	<< asml->comment << "\" but the comment should be NULL";
    } else if (expectedComment && !asml->comment) {
      return ::testing::AssertionFailure()
	<< "Parse produced an AssemblyLine with a NULL comment, but the "
	<< "comment should be \"" << expectedComment << "\"";
    } else if (expectedComment && strcmp(asml->comment, expectedComment)) {
      return ::testing::AssertionFailure()
	<< "Parse produced an AssemblyLine with a comment \""
	<< asml->comment << "\", but that comment should be \""
	<< expectedComment << "\"";
    }

    return ::testing::AssertionSuccess();
  }

  ::testing::AssertionResult verifyParseError(
    AssemblyLine* asml, AsmParseError* parseError, uint16_t expectedColumn,
    const char* expectedMessage
  ) {
    if (parseError && asml) {
      char buf[200];
      sprintAssemblyLine(buf, sizeof(buf), asml);
      return ::testing::AssertionFailure()
	<< "Parse retured an error at column " << parseError->column
	<< " (" << parseError->message << "), but it also returned a "
	<< "non-NULL AssemblyLine [" << buf << "]";
    } else if (parseError) {
      if ((parseError->column != expectedColumn)
	    || strcmp(parseError->message, expectedMessage)) {
	return ::testing::AssertionFailure()
	  << "Parse returned an error at column " << parseError->column
	  << "  with message \"" << parseError->message << "\", but it "
	  "should have returned an error at column " << expectedColumn
	  << " with message \"" << expectedMessage << "\"";
      }
      return ::testing::AssertionSuccess();
    } else if (asml) {
      char buf[200];
      sprintAssemblyLine(buf, sizeof(buf), asml);
      return ::testing::AssertionFailure()
	<< "Parse succeeded and returned [" << buf << "]";
    } else {
      return ::testing::AssertionFailure()
	<< "Parse returned both a NULL AssemblyLine and a NULL ParseError";
    }
  }

  ::testing::AssertionResult verifyInstrctionWithoutOperand(
    const char* instruction, uint8_t expectedOpcode
  ) {
    AsmParseError* parseError = NULL;
    AssemblyLine* asml = parseAssemblyLine(instruction, 200, 4, &parseError);
    auto result = verifySuccessfulParse(asml, parseError,
					ASM_LINE_TYPE_INSTRUCTION, 200, 4, 0,
					NULL);
    if (!result) {
      return result;
    } else if (asml->value.instruction.opcode != expectedOpcode) {
      return ::testing::AssertionFailure()
	<< "Parse returned opcode " << asml->value.instruction.opcode
	<< ", but it should have returned " << expectedOpcode;
    } else if (asml->value.instruction.operand.type != ASM_VALUE_TYPE_NONE) {
      return ::testing::AssertionFailure()
	<< "Parse returned a value of type "
	<< asml->value.instruction.operand.type
	<< ", but it should have returned a value of type "
	<< ASM_VALUE_TYPE_NONE;
    } else {
      destroyAssemblyLine(asml);
      return ::testing::AssertionSuccess();
    }
  }

  ::testing::AssertionResult verifyPrintCharacterEscape(
    char ch, uint8_t expectedValue
  ) {
    char buf[15];
    snprintf(buf, sizeof(buf), "PRINT '\\%c'", ch);

    AsmParseError* parseError = NULL;
    AssemblyLine* asml = parseAssemblyLine(buf, 200, 4, &parseError);
    auto result = verifySuccessfulParse(asml, parseError,
					ASM_LINE_TYPE_INSTRUCTION,
					200, 4, 0, NULL);
    if (!result) {
      return result;
    }
    assert(asml);
    if (asml->value.instruction.opcode != PRINT_INSTRUCTION) {
      return ::testing::AssertionFailure()
        << "Parse produced an instruction with opcode "
        << asml->value.instruction.opcode
        << ", but it should have produced an opcode of " << PRINT_INSTRUCTION;
    } else if (asml->value.instruction.operand.type != ASM_VALUE_TYPE_UINT64) {
      return ::testing::AssertionFailure()
        << "Parse produced an instruction with an operand of type "
        << asml->value.instruction.operand.type
        << ", but it should have produced an operand of type "
        << ASM_VALUE_TYPE_UINT64;
    } else if (asml->value.instruction.operand.value.u64 != expectedValue) {
      return ::testing::AssertionFailure()
        << "Parse produced a value of "
        << asml->value.instruction.operand.value.u64
        << ", but it should have produced a value of "
        << (uint32_t)expectedValue;
    } else {
      destroyAssemblyLine(asml);
      return ::testing::AssertionSuccess();
    }
  }
}

TEST(asm_tests, initEmptyAsmValue) {
  TypedAsmValue value;
  EXPECT_EQ(initEmptyAsmValue(&value), &value);
  EXPECT_EQ(value.type, ASM_VALUE_TYPE_NONE);
  EXPECT_EQ(cleanAsmValue(&value), &value);
}

TEST(asm_tests, initUInt64AsmValue) {
  TypedAsmValue value;
  EXPECT_EQ(initUInt64AsmValue(&value, 42), &value);
  EXPECT_EQ(value.type, ASM_VALUE_TYPE_UINT64);
  EXPECT_EQ(value.value.u64, 42);
  EXPECT_EQ(cleanAsmValue(&value), &value);
}

TEST(asm_tests, initSymbolAndOffsetAsmValue) {
  TypedAsmValue value;
  EXPECT_EQ(initSymbolAndOffsetAsmValue(&value, "cow", 42), &value);
  EXPECT_EQ(value.type, ASM_VALUE_TYPE_SYMBOL_OFFSET);
  EXPECT_EQ(std::string(value.value.sando.symbolName), "cow");
  EXPECT_EQ(value.value.sando.offset, 42);
  EXPECT_EQ(cleanAsmValue(&value), &value);
}

TEST(asm_tests, initStringAsmValue) {
  TypedAsmValue value;
  EXPECT_EQ(initStringAsmValue(&value, "42"), &value);
  EXPECT_EQ(value.type, ASM_VALUE_TYPE_STRING);
  EXPECT_EQ(std::string(value.value.str), "42");
  EXPECT_EQ(cleanAsmValue(&value), &value);  
}

TEST(asm_tests, initAsmValueFromOther) {
  TypedAsmValue empty;
  TypedAsmValue u64;
  TypedAsmValue sando;
  TypedAsmValue str;
  TypedAsmValue target;

  initEmptyAsmValue(&empty);
  initUInt64AsmValue(&u64, 42);
  initSymbolAndOffsetAsmValue(&sando, "cow", 22);
  initStringAsmValue(&str, "penguin");

  initAsmValueFromOther(&target, &empty);
  EXPECT_EQ(target.type, ASM_VALUE_TYPE_NONE);
  EXPECT_EQ(empty.type, ASM_VALUE_TYPE_NONE);
  cleanAsmValue(&target);

  initAsmValueFromOther(&target, &u64);
  EXPECT_EQ(target.type, ASM_VALUE_TYPE_UINT64);
  EXPECT_EQ(target.value.u64, 42);
  EXPECT_EQ(u64.type, ASM_VALUE_TYPE_UINT64);
  EXPECT_EQ(u64.value.u64, 42);
  cleanAsmValue(&target);
  
  initAsmValueFromOther(&target, &sando);
  EXPECT_EQ(target.type, ASM_VALUE_TYPE_SYMBOL_OFFSET);
  EXPECT_EQ(std::string(target.value.sando.symbolName), "cow");
  EXPECT_EQ(target.value.sando.offset, 22);
  EXPECT_EQ(sando.type, ASM_VALUE_TYPE_SYMBOL_OFFSET);
  EXPECT_EQ(std::string(sando.value.sando.symbolName), "cow");
  EXPECT_EQ(sando.value.sando.offset, 22);  
  cleanAsmValue(&target);

  initAsmValueFromOther(&target, &str);
  EXPECT_EQ(target.type, ASM_VALUE_TYPE_STRING);
  EXPECT_EQ(std::string(target.value.str), "penguin");
  EXPECT_EQ(str.type, ASM_VALUE_TYPE_STRING);
  EXPECT_EQ(std::string(str.value.str), "penguin");
  cleanAsmValue(&target);

  cleanAsmValue(&str);
  cleanAsmValue(&sando);
  cleanAsmValue(&u64);
  cleanAsmValue(&empty);
}

TEST(asm_tests, resolveAsmValue) {
  SymbolTable symtab = createSymbolTable(16);
  ASSERT_EQ(addSymbolToTable(symtab, "cow", 14), 0);
  ASSERT_EQ(addSymbolToTable(symtab, "penguin", 100), 0);
  
  TypedAsmValue empty;
  TypedAsmValue u64;
  TypedAsmValue sando;
  TypedAsmValue str;

  initEmptyAsmValue(&empty);
  initUInt64AsmValue(&u64, 42);
  initSymbolAndOffsetAsmValue(&sando, "cow", 22);
  initStringAsmValue(&str, "penguin");

  const char* errorMessage = NULL;

  EXPECT_EQ(resolveAsmValueToAddress(&empty, symtab, &errorMessage), 0);
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage),
	    "Cannot resolve value of type NONE to address");
  free((void*)errorMessage);

  EXPECT_EQ(resolveAsmValueToAddress(&u64, symtab, &errorMessage), 42);
  EXPECT_EQ(errorMessage, (void*)0);

  EXPECT_EQ(resolveAsmValueToAddress(&sando, symtab, &errorMessage), 36);
  EXPECT_EQ(errorMessage, (void*)0);
  
  EXPECT_EQ(resolveAsmValueToAddress(&str, symtab, &errorMessage), 100);
  EXPECT_EQ(errorMessage, (void*)0);

  TypedAsmValue unknownSymbol;
  initStringAsmValue(&unknownSymbol, "cat");
  EXPECT_EQ(resolveAsmValueToAddress(&unknownSymbol, symtab, &errorMessage), 0);
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage), "Cannot resolve unknown symbol \"cat\"");
  free((void*)errorMessage);

  cleanAsmValue(&unknownSymbol);
  cleanAsmValue(&str);
  cleanAsmValue(&sando);
  cleanAsmValue(&u64);
  cleanAsmValue(&empty);
  destroySymbolTable(symtab);
}

TEST(asm_tests, createEmptyAsmLine) {
  AssemblyLine *asml = createEmptyAsmLine(20, 3, 32, "A comment");
  ASSERT_NE(asml, (void*)0);
  EXPECT_EQ(asml->type, ASM_LINE_TYPE_EMPTY);
  EXPECT_EQ(asml->address, 20);
  EXPECT_EQ(asml->line, 3);
  EXPECT_EQ(asml->column, 32);
  ASSERT_NE(asml->comment, (void*)0);
  EXPECT_EQ(std::string(asml->comment), "A comment");
  destroyAssemblyLine(asml);
}

TEST(asm_tests, createInstructionAsmLine) {
  TypedAsmValue operand;
  initUInt64AsmValue(&operand, 42);
  
  AssemblyLine *asml = createInstructionAsmLine(
    20, 3, 32, PUSH_INSTRUCTION, &operand, "A comment"
  );
  ASSERT_NE(asml, (void*)0);
  EXPECT_EQ(asml->type, ASM_LINE_TYPE_INSTRUCTION);
  EXPECT_EQ(asml->address, 20);
  EXPECT_EQ(asml->line, 3);
  EXPECT_EQ(asml->column, 32);
  EXPECT_EQ(asml->value.instruction.opcode, PUSH_INSTRUCTION);
  EXPECT_EQ(asml->value.instruction.operand.type, ASM_VALUE_TYPE_UINT64);
  EXPECT_EQ(asml->value.instruction.operand.value.u64, 42);
  ASSERT_NE(asml->comment, (void*)0);
  EXPECT_EQ(std::string(asml->comment), "A comment");
  destroyAssemblyLine(asml);
  cleanAsmValue(&operand);
}

TEST(asm_tests, createDirectiveAsmLine) {
  TypedAsmValue operand;
  initUInt64AsmValue(&operand, 42);
  
  AssemblyLine *asml = createDirectiveAsmLine(
    20, 3, 32, START_ADDRESS_DIRECTIVE, &operand, "A comment"
  );
  ASSERT_NE(asml, (void*)0);
  EXPECT_EQ(asml->type, ASM_LINE_TYPE_DIRECTIVE);
  EXPECT_EQ(asml->address, 20);
  EXPECT_EQ(asml->line, 3);
  EXPECT_EQ(asml->column, 32);
  EXPECT_EQ(asml->value.directive.code, START_ADDRESS_DIRECTIVE);
  EXPECT_EQ(asml->value.directive.operand.type, ASM_VALUE_TYPE_UINT64);
  EXPECT_EQ(asml->value.directive.operand.value.u64, 42);
  ASSERT_NE(asml->comment, (void*)0);
  EXPECT_EQ(std::string(asml->comment), "A comment");
  destroyAssemblyLine(asml);
  cleanAsmValue(&operand);
}

TEST(asm_tests, createLabelAsmLine) {
  AssemblyLine *asml = createLabelAsmLine(
    20, 3, 32, "cow", "A comment"
  );
  ASSERT_NE(asml, (void*)0);
  EXPECT_EQ(asml->type, ASM_LINE_TYPE_LABEL);
  EXPECT_EQ(asml->address, 20);
  EXPECT_EQ(asml->line, 3);
  EXPECT_EQ(asml->column, 32);
  ASSERT_NE(asml->value.label.labelName, (void*)0);
  EXPECT_EQ(std::string(asml->value.label.labelName), "cow");
  ASSERT_NE(asml->comment, (void*)0);
  EXPECT_EQ(std::string(asml->comment), "A comment");
  destroyAssemblyLine(asml);
}

TEST(asm_tests, createSymbolAssignmentAsmLine) {
  TypedAsmValue operand;
  initSymbolAndOffsetAsmValue(&operand, "penguin", -2);
  
  AssemblyLine *asml = createSymbolAssignmentAsmLine(
    20, 3, 32, "cow", &operand, "A comment"
  );
  cleanAsmValue(&operand);

  ASSERT_NE(asml, (void*)0);
  EXPECT_EQ(asml->type, ASM_LINE_TYPE_SYMBOL_ASSIGNMENT);
  EXPECT_EQ(asml->address, 20);
  EXPECT_EQ(asml->line, 3);
  EXPECT_EQ(asml->column, 32);
  ASSERT_NE(asml->value.symassign.symbolName, (void*)0);
  EXPECT_EQ(std::string(asml->value.symassign.symbolName), "cow");
  EXPECT_EQ(asml->value.symassign.value.type, ASM_VALUE_TYPE_SYMBOL_OFFSET);
  ASSERT_NE(asml->value.symassign.value.value.sando.symbolName, (void*)0);
  EXPECT_EQ(std::string(asml->value.symassign.value.value.sando.symbolName),
	    "penguin");
  EXPECT_EQ(asml->value.symassign.value.value.sando.offset, -2);
  ASSERT_NE(asml->comment, (void*)0);
  EXPECT_EQ(std::string(asml->comment), "A comment");
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseEmptyLine) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("", 200, 4, &parseError);

  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_EMPTY,
				    200, 4, 0, NULL));
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseWhitespaceLine) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  \t  ", 200, 4, &parseError);

  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_EMPTY,
				    200, 4, 0, NULL));
  destroyAssemblyLine(asml);

}

TEST(asm_tests, parseLineWithCommentOnly) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("# Cows rule", 200, 4, &parseError);

  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_EMPTY,
				    200, 4, 0, " Cows rule"));
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseStartDirective) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  .start 512\n", 200, 4, &parseError);

  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_DIRECTIVE,
				    200, 4, 2, NULL));

  if (asml->type == ASM_LINE_TYPE_DIRECTIVE) {
    EXPECT_EQ(asml->value.directive.code, START_ADDRESS_DIRECTIVE);
    EXPECT_EQ(asml->value.directive.operand.type, ASM_VALUE_TYPE_UINT64);
    EXPECT_EQ(asml->value.directive.operand.value.u64, 512);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseStartDirectiveWithComment) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  .start 512 # Penguins are cute",
					 200, 4, &parseError);

  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_DIRECTIVE,
				    200, 4, 2, " Penguins are cute"));

  if (asml->type == ASM_LINE_TYPE_DIRECTIVE) {
    EXPECT_EQ(asml->value.directive.code, START_ADDRESS_DIRECTIVE);
    EXPECT_EQ(asml->value.directive.operand.type, ASM_VALUE_TYPE_UINT64);
    EXPECT_EQ(asml->value.directive.operand.value.u64, 512);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseDirectiveWithMissingOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  .start", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 8, "Operand missing"));
  destroyAsmParseError(parseError);

  parseError = NULL;
  asml = parseAssemblyLine(".start # Cows rule", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 7, "Operand missing"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseDirectiveWithInvalidOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  .start $F0", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 9, "Syntax error"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseDirctieWithTooBigAddress) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml =
    parseAssemblyLine("  .start 18446744073709551616", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 9, "Value is too large"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseDirectieWithGarbageAfterOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  .start 512(A)", 200, 4,
					 &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 12, "Syntax error"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseDirectiveWithMissingName) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  . # Foo", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 3, "Directive name missing"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseUnknownDirective) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("  .end", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 3, "Unknown directive"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseSymbolicAddress) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(".start COW", 200, 4, &parseError);
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_DIRECTIVE,
				    200, 4, 0, NULL));
  if (asml) {
    EXPECT_EQ(asml->value.directive.code, START_ADDRESS_DIRECTIVE);
    EXPECT_EQ(asml->value.directive.operand.type, ASM_VALUE_TYPE_SYMBOL_OFFSET);
    EXPECT_EQ(std::string(asml->value.directive.operand.value.sando.symbolName),
	      "COW");
    EXPECT_EQ(asml->value.directive.operand.value.sando.offset, 0);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseSymbolicAddressWithPositiveOffset) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(".start COW+2", 200, 4, &parseError);
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_DIRECTIVE,
				    200, 4, 0, NULL));
  if (asml) {
    EXPECT_EQ(asml->value.directive.code, START_ADDRESS_DIRECTIVE);
    EXPECT_EQ(asml->value.directive.operand.type, ASM_VALUE_TYPE_SYMBOL_OFFSET);
    EXPECT_EQ(std::string(asml->value.directive.operand.value.sando.symbolName),
	      "COW");
    EXPECT_EQ(asml->value.directive.operand.value.sando.offset, 2);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseSymbolicAddressWithNegativeOffset) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(".start COW-2", 200, 4, &parseError);
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_DIRECTIVE,
				    200, 4, 0, NULL));
  if (asml) {
    EXPECT_EQ(asml->value.directive.code, START_ADDRESS_DIRECTIVE);
    EXPECT_EQ(asml->value.directive.operand.type, ASM_VALUE_TYPE_SYMBOL_OFFSET);
    EXPECT_EQ(std::string(asml->value.directive.operand.value.sando.symbolName),
	      "COW");
    EXPECT_EQ(asml->value.directive.operand.value.sando.offset, -2);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseSymbolicAddressWithInvalidOffset) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(".start COW+MOO", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 11,
			       "Operand must be an integer"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseSymbolicAddressWithGarbageAfter) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(".start COW?6", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 10, "Syntax error"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parsePanicInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("PANIC", PANIC_INSTRUCTION));
}

TEST(asm_tests, parsePushInstruction) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(
    "  PUSH 18446744073709551615 #Last possible address \n", 200, 4,
    &parseError
  );
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_INSTRUCTION,
				    200, 4, 2, "Last possible address \n"));
  if (asml) {
    EXPECT_EQ(asml->value.instruction.opcode, PUSH_INSTRUCTION);
    EXPECT_EQ(asml->value.instruction.operand.type, ASM_VALUE_TYPE_UINT64);
    EXPECT_EQ(asml->value.instruction.operand.value.u64, 0xFFFFFFFFFFFFFFFF);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parsePushInstructionWithoutOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("PUSH", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 4, "Operand missing"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parsePopInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("POP", POP_INSTRUCTION));
}

TEST(asm_tests, parseSwapInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("SWAP", SWAP_INSTRUCTION));  
}

TEST(asm_tests, parseDupInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("DUP", DUP_INSTRUCTION));
}

TEST(asm_tests, parsePCallInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("PCALL", PCALL_INSTRUCTION));
}

TEST(asm_tests, parseRetInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("RET", RET_INSTRUCTION));
}

TEST(asm_tests, parseMkkInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("MKK", MKK_INSTRUCTION));
}

TEST(asm_tests, parseMKs0Instruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("MKS0", MKS0_INSTRUCTION));
}

TEST(asm_tests, parseMks1Instruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("MKS1", MKS1_INSTRUCTION));
}

TEST(asm_tests, parseMks2Instruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("MKS2", MKS2_INSTRUCTION));
}

TEST(asm_tests, parseMkdInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("MKD", MKD_INSTRUCTION));
}

TEST(asm_tests, parseMkcInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("MKC", MKC_INSTRUCTION));
}

TEST(asm_tests, parseSaveInstruction) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("SAVE 2", 200, 4, &parseError);
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_INSTRUCTION,
				    200, 4, 0, NULL));
  if (asml) {
    EXPECT_EQ(asml->value.instruction.opcode, SAVE_INSTRUCTION);
    EXPECT_EQ(asml->value.instruction.operand.type, ASM_VALUE_TYPE_UINT64);
    EXPECT_EQ(asml->value.instruction.operand.value.u64, 2);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parseSaveWithNoOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("SAVE ", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 5, "Operand missing"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseSaveWithTooBigOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("SAVE 256", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 5,
			       "Operand must be in the range 0-255"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseRestoreInstruction) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("RESTORE 10", 200, 4, &parseError);
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_INSTRUCTION,
				    200, 4, 0, NULL));
  if (asml) {
    EXPECT_EQ(asml->value.instruction.opcode, RESTORE_INSTRUCTION);
    EXPECT_EQ(asml->value.instruction.operand.type, ASM_VALUE_TYPE_UINT64);
    EXPECT_EQ(asml->value.instruction.operand.value.u64, 10);
  }
  destroyAssemblyLine(asml);  
}

TEST(asm_tests, parseRestoreWithNoOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("RESTORE", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 7, "Operand missing"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseRestoreWithTooBigOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("RESTORE 256", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 8,
			       "Operand must be in the range 0-255"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parsePrintSingleCharacterLiteral) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(
    "PRINT 'a' # Go cows go", 200, 4, &parseError
  );
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_INSTRUCTION,
				    200, 4, 0, " Go cows go"));
  if (asml) {
    EXPECT_EQ(asml->value.instruction.opcode, PRINT_INSTRUCTION);
    EXPECT_EQ(asml->value.instruction.operand.type, ASM_VALUE_TYPE_UINT64);
    EXPECT_EQ(asml->value.instruction.operand.value.u64, 97);
  }
  destroyAssemblyLine(asml);
}

TEST(asm_tests, parsePrintSingleCharacterEsacpe) {
  EXPECT_TRUE(verifyPrintCharacterEscape('n', 10));
  EXPECT_TRUE(verifyPrintCharacterEscape('t', 9));
  EXPECT_TRUE(verifyPrintCharacterEscape('r', 13));
  EXPECT_TRUE(verifyPrintCharacterEscape('\'', '\''));
  EXPECT_TRUE(verifyPrintCharacterEscape('\\', '\\'));
}

TEST(asm_tests, parsePrintHexEscape) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine(
    "PRINT '\x41'", 200, 4, &parseError
  );
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_INSTRUCTION,
				    200, 4, 0, NULL));
  if (asml) {
    EXPECT_EQ(asml->value.instruction.opcode, PRINT_INSTRUCTION);
    EXPECT_EQ(asml->value.instruction.operand.type, ASM_VALUE_TYPE_UINT64);
    EXPECT_EQ(asml->value.instruction.operand.value.u64, 65);
  }
  destroyAssemblyLine(asml);  
}

TEST(asm_tests, parsePrintWithNoOperand) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("PRINT", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 5,
			       "Operand missing"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parsePrintMssingOpeningQuote) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("PRINT a'", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (must start with "
                               "\"\'\")"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, printWithMissingClosingQuote) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("PRINT 'a", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (Closing "
                               "\"\'\" missing)"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parsePrintWithEmptyLiteral) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("PRINT ''", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (literal "
                               "is empty)"));
  destroyAsmParseError(parseError);

  parseError = NULL;
  asml = parseAssemblyLine("PRINT '", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (literal "
                               "is empty)"));
  destroyAsmParseError(parseError);

  parseError = NULL;
  asml = parseAssemblyLine("PRINT '\n", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (literal "
                               "is empty)"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parsePrintWithInvalidCharacterEscape) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("PRINT '\\u'", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (Unknown "
                               "escape sequence)"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parsePrintWithInvalidHexEscape) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("PRINT '\\xg1'", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (Invalid hex "
                               "escape sequence)"));
  destroyAsmParseError(parseError);

  parseError = NULL;
  asml = parseAssemblyLine("PRINT '\\x4 '", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (Invalid hex "
                               "escape sequence)"));
  destroyAsmParseError(parseError);

  parseError = NULL;
  asml = parseAssemblyLine("PRINT '\\x'", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (Invalid hex "
                               "escape sequence)"));
  destroyAsmParseError(parseError);

  parseError = NULL;
  asml = parseAssemblyLine("PRINT '\\x412'", 200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 6,
			       "Invalid character literal (Closing \"'\" "
                               "missing)"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseHaltInstruction) {
  EXPECT_TRUE(verifyInstrctionWithoutOperand("HALT", HALT_INSTRUCTION));
}

TEST(asm_tests, parseUnknownInstruction) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("GOTO 512 # Unknown instruction",
                                         200, 4, &parseError);
  EXPECT_TRUE(verifyParseError(asml, parseError, 0, "Invalid instruction"));
  destroyAsmParseError(parseError);
}

TEST(asm_tests, parseLabel) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("MOO:", 200, 4, &parseError);
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_LABEL,
				    200, 4, 0, NULL));
  if (asml) {
    ASSERT_NE(asml->value.label.labelName, (void*)0);
    EXPECT_EQ(std::string(asml->value.label.labelName), "MOO");
  }
  destroyAssemblyLine(asml);  
}

TEST(asm_tests, parseLabelWithComment) {
  AsmParseError* parseError = NULL;
  AssemblyLine* asml = parseAssemblyLine("MOO: # This is a comment",
                                         200, 4, &parseError);
  EXPECT_TRUE(verifySuccessfulParse(asml, parseError, ASM_LINE_TYPE_LABEL,
				    200, 4, 0, " This is a comment"));
  if (asml) {
    ASSERT_NE(asml->value.label.labelName, (void*)0);
    EXPECT_EQ(std::string(asml->value.label.labelName), "MOO");
  }
  destroyAssemblyLine(asml);  
}
