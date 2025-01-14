extern "C" {
#include <symtab.h>
#include <vm_instructions.h>
}

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>

namespace {
  ::testing::AssertionResult verifyDisassemblyWithHeap(
      const uint8_t* startOfMemory, const uint8_t* startOfHeap,
      const uint8_t* endOfMemory, const uint8_t* program,
      const size_t programLength, const Symbol* symbols,
      const char* disassembly, const int maxLines = 100
  ) {
    SymbolTable symtab = symbols ? createSymbolTable(1024) : NULL;

    if (symtab) {
      for (const Symbol* s = symbols; s->name; ++s) {
	if (addSymbolToTable(symtab, s->name, s->address)) {
	  auto result = ::testing::AssertionFailure()
	    << "Could not add symbol (" << s->name << ", " << s->address
	    << ") to symbol table: " << getSymbolTableStatusMsg(symtab);
	  destroySymbolTable(symtab);
	  return result;
	}
      }
    }

    char* output = NULL;
    size_t outputLen;
    FILE* memstream = open_memstream(&output, &outputLen);

    if (!memstream) {
      if (symtab) {
	destroySymbolTable(symtab);
      }
      return ::testing::AssertionFailure() << "Could not open memstream";
    }

    const uint8_t* p = program;
    const uint8_t* endOfProgram = p + programLength;

    int iterCnt = 0;
    while ((p < endOfProgram) && (iterCnt < maxLines)) {
      p = disassembleVmCode(p, startOfMemory, startOfHeap, endOfMemory, symtab,
			    memstream);
    }

    fclose(memstream);

    if (!output) {
      if (symtab) {
	destroySymbolTable(symtab);
      }
      return ::testing::AssertionFailure() << "output is NULL";
    }

    if ((outputLen != strlen(disassembly)) || (strcmp(output, disassembly))) {
      auto result = ::testing::AssertionFailure()
	<< "Output does not match expected disassembly (length = "
	<< outputLen << " vs. " << strlen(disassembly) << "):\n\n"
	<< (const char*)output << "\n**** VS ****\n\n" << disassembly;
      ::free(output);
      if (symtab) {
	destroySymbolTable(symtab);
      }
      return result;
    }
    
    ::free(output);
    if (symtab) {
      destroySymbolTable(symtab);
    }
    return ::testing::AssertionSuccess();
  }

  ::testing::AssertionResult verifyDisassembly(const uint8_t* program,
					       const size_t programLength,
					       const Symbol* symbols,
					       const char* disassembly,
					       const int maxLines = 100) {
    return verifyDisassemblyWithHeap(program, program + programLength,
				     program + programLength, program,
				     programLength, symbols, disassembly,
				     maxLines);
  }
}

TEST(vm_instructions_tests, disassembleVmCode) {
  const uint8_t PROGRAM[] = {
    PANIC_INSTRUCTION,
    PUSH_INSTRUCTION, 0xAD, 0xBE, 0xED, 0xFE, 0xEF, 0xBE, 0xAD, 0xDE,
    POP_INSTRUCTION,
    SWAP_INSTRUCTION,
    DUP_INSTRUCTION,
    PCALL_INSTRUCTION,
    RET_INSTRUCTION,
    MKK_INSTRUCTION,
    MKS0_INSTRUCTION,
    MKS1_INSTRUCTION,
    MKS2_INSTRUCTION,
    MKD_INSTRUCTION,
    MKC_INSTRUCTION,
    SAVE_INSTRUCTION, 14,
    RESTORE_INSTRUCTION, 21,
    PRINT_INSTRUCTION, 65,
    HALT_INSTRUCTION,
    255,  // Test disassembler's ability to handle invalid instructions
  };
  const Symbol SYMBOLS[] = { { "AX0", 10 }, { "BX0", 18 }, { "CX0", 29 },
			     { NULL, 0 } };
  const char DISASSEMBLY[] =
    // "XXXXXXXXXXXXXXXXXXXXX  XX XX XX XX XX XX XX XX XX   XXXX"
    "                    0  00                           PANIC\n"
    "                    1  01 AD BE ED FE EF BE AD DE   PUSH 16045690985374400173\n"
    "                                                  AX0:\n"
    "                   10  02                           POP\n"
    "                   11  03                           SWAP\n"
    "                   12  04                           DUP\n"
    "                   13  05                           PCALL\n"
    "                   14  06                           RET\n"
    "                   15  07                           MKK\n"
    "                   16  08                           MKS0\n"
    "                   17  09                           MKS1\n"
    "                                                  BX0:\n"
    "                   18  0A                           MKS2\n"
    "                   19  0B                           MKD\n"
    "                   20  0C                           MKC\n"
    "                   21  0D 0E                        SAVE 14\n"
    "                   23  0E 15                        RESTORE 21\n"
    "                   25  0F 41                        PRINT 'A'\n"
    "                   27  10                           HALT\n"
    "                   28  FF                           ???\n";

  EXPECT_TRUE(verifyDisassembly(PROGRAM, sizeof(PROGRAM), SYMBOLS,
				DISASSEMBLY));
}

TEST(vm_instructions_tests, disassembleSymbolicAddress) {
  const uint8_t PROGRAM[] = {
    PUSH_INSTRUCTION, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    PCALL_INSTRUCTION,
    RET_INSTRUCTION,
    RESTORE_INSTRUCTION, 1,
    RET_INSTRUCTION,
  };
  const Symbol SYMBOLS[] = { { "MOO", 11 }, { NULL, 0 } };
  const char DISASSEMBLY[] =
    // "XXXXXXXXXXXXXXXXXXXXX  XX XX XX XX XX XX XX XX XX   XXXX"
    "                    0  01 0B 00 00 00 00 00 00 00   PUSH MOO\n"
    "                    9  05                           PCALL\n"
    "                   10  06                           RET\n"
    "                                                  MOO:\n"
    "                   11  0E 01                        RESTORE 1\n"
    "                   13  06                           RET\n";

  EXPECT_TRUE(verifyDisassembly(PROGRAM, sizeof(PROGRAM), SYMBOLS,
				DISASSEMBLY));
}


TEST(vm_instructions_tests, disassembleSymbolicAddressPlusOffset) {
  const uint8_t PROGRAM[] = {
    PUSH_INSTRUCTION, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    PCALL_INSTRUCTION,
    RET_INSTRUCTION,
    RESTORE_INSTRUCTION, 1,
    RET_INSTRUCTION,
  };
  const Symbol SYMBOLS[] = { { "MOO", 11 }, { NULL, 0 } };
  const char DISASSEMBLY[] =
    // "XXXXXXXXXXXXXXXXXXXXX  XX XX XX XX XX XX XX XX XX   XXXX"
    "                    0  01 0D 00 00 00 00 00 00 00   PUSH MOO+2\n"
    "                    9  05                           PCALL\n"
    "                   10  06                           RET\n"
    "                                                  MOO:\n"
    "                   11  0E 01                        RESTORE 1\n"
    "                   13  06                           RET\n";

  EXPECT_TRUE(verifyDisassembly(PROGRAM, sizeof(PROGRAM), SYMBOLS,
				DISASSEMBLY));
}

TEST(vm_instructions_tests, disassemblePushArgumentInHeap) {
  const uint8_t PROGRAM[] = {
    PUSH_INSTRUCTION, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    MKK_INSTRUCTION,
    RET_INSTRUCTION
  };
  const Symbol SYMBOLS[] = { { "COW", 512 }, { NULL, 0 } };
  const char DISASSEMBLY[] =
    // "XXXXXXXXXXXXXXXXXXXXX  XX XX XX XX XX XX XX XX XX   XXXX"
    "                   64  01 00 02 00 00 00 00 00 00   PUSH 512\n"
    "                   73  07                           MKK\n"
    "                   74  06                           RET\n";
  const size_t MEMORY_SIZE = 1024;
  const size_t HEAP_SIZE = 768;
  const size_t PROGRAM_START = 64;
  const size_t HEAP_START = MEMORY_SIZE - HEAP_SIZE;
  uint8_t memory[MEMORY_SIZE];
  
  memcpy(memory + PROGRAM_START, PROGRAM, sizeof(PROGRAM));

  EXPECT_TRUE(verifyDisassemblyWithHeap(memory, memory + HEAP_START,
					memory + MEMORY_SIZE,
					memory + PROGRAM_START,
					sizeof(PROGRAM), SYMBOLS,
					DISASSEMBLY));
}

TEST(vm_instructions_tests, disassemblePrintCharacterWithNoGlyph) {
  const uint8_t PROGRAM[] = {
    PRINT_INSTRUCTION, 0x00,
    PRINT_INSTRUCTION, 0x1F,
    PRINT_INSTRUCTION, 0x20,
    PRINT_INSTRUCTION, 0x7E,
    PRINT_INSTRUCTION, 0x7F,
    PRINT_INSTRUCTION, 0x9F,
    PRINT_INSTRUCTION, 0xA0,
  };
  const char DISASSEMBLY[] =
    // "XXXXXXXXXXXXXXXXXXXXX  XX XX XX XX XX XX XX XX XX   XXXX"
    "                    0  0F 00                        PRINT '\\x00'\n"
    "                    2  0F 1F                        PRINT '\\x1f'\n"
    "                    4  0F 20                        PRINT ' '\n"
    "                    6  0F 7E                        PRINT '\x7e'\n"
    "                    8  0F 7F                        PRINT '\\x7f'\n"
    "                   10  0F 9F                        PRINT '\\x9f'\n"
    "                   12  0F A0                        PRINT '\xa0'\n";
  EXPECT_TRUE(verifyDisassembly(PROGRAM, sizeof(PROGRAM), NULL, DISASSEMBLY));
}
