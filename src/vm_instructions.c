#include "vm_instructions.h"
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>

static const uint8_t INSTRUCTION_SIZE[] = {
    1, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1,
};

static const char* INSTRUCTION_NAME[] = {
    "PANIC", "PUSH", "POP", "SWAP", "DUP", "PCALL", "RET", "MKK", "MKS0",
    "MKS1", "MKS2", "MKD", "MKC", "SAVE", "RESTORE", "PRINT", "HALT"
};

static const char UNKNOWN_INSTRUCTION_NAME[] = "???";

static const size_t NUM_INSTRUCTIONS =
  sizeof(INSTRUCTION_NAME) / sizeof(const char*);

uint8_t instructionSize(uint8_t instruction) {
  return (instruction < NUM_INSTRUCTIONS) ? INSTRUCTION_SIZE[instruction] : 1;
}

const char* instructionName(uint8_t instruction) {
  return (instruction < NUM_INSTRUCTIONS) ? INSTRUCTION_NAME[instruction]
                                          : UNKNOWN_INSTRUCTION_NAME;
}

static void writeRawHex(const uint8_t* instruction,
			const uint8_t* endOfMemory,
			char* out, size_t outSize) {
  char* q = out;
  char* end = out + outSize;

  if (instruction < endOfMemory) {
    uint8_t numBytes = instructionSize(*instruction);
    size_t remaining = outSize;

    if ((instruction + numBytes) > endOfMemory) {
      numBytes = endOfMemory - instruction;
    }

    for (uint8_t i = 0; (i < numBytes) && (q < end); ++i) {
      snprintf(q, end - q, " %02X", (uint32_t)instruction[i]);
      q += 3;
    }
  }

  while (q < (end - 1)) {
    *q++ = ' ';
  }
  *q = 0;
}

const uint8_t* disassembleVmCode(const uint8_t* code,
				 const uint8_t* startOfMemory,
				 const uint8_t* startOfHeap,
				 const uint8_t* endOfMemory,
				 SymbolTable symtab, FILE* out) {
  if (!code) {
    fprintf(out, "  **ERROR: \"code\" is NULL\n");
    return NULL;
  }

  if (!startOfMemory) {
    fprintf(out, "  **ERROR: \"startOfMemory\" is NULL\n");
    return NULL;
  }

  if (!startOfHeap) {
    fprintf(out, "  **ERROR: \"startOfHeap\" is NULL\n");
    return NULL;
  }

  if (!endOfMemory) {
    fprintf(out, "  **ERROR: \"endOfMemory\" is NULL\n");
    return NULL;
  }

  if (endOfMemory < startOfMemory) {
    fprintf(out, "  **ERROR: \"endOfMemory\" < \"startOfMemory\"\n");
    return NULL;
  }

  if (startOfHeap < startOfMemory) {
    fprintf(out, "  **ERROR: \"startOfHeap\" < \"startOfMemory\"\n");
    return NULL;
  }

  if (startOfHeap > endOfMemory) {
    fprintf(out, "  **ERROR: \"startOfHeap\" > \"endOfMemory\"\n");
    return NULL;
  }

  if (code < startOfMemory) {
    fprintf(out, "  **ERROR: \"code\" < \"startOfMemory\"\n");
    return NULL;
  }

  if (code >= endOfMemory) {
    fprintf(out, "  **ERROR: \"code\" >= \"endOfMemory\"\n");
    return NULL;
  }

  const Symbol* sym =
    symtab ? getSymbolAtAddress(symtab, (uint64_t)(code - startOfMemory))
           : NULL;
  if (sym) {
    fprintf(out, "%21s  %27s%s:\n", " ", " ", sym->name);
  }

  char rawHex[28];
  writeRawHex(code, endOfMemory, rawHex, sizeof(rawHex));
  fprintf(out, "%21" PRIu64 " %27s  ", (uint64_t)(code - startOfMemory),
	  rawHex);
	  
  switch (*code) {
    case PUSH_INSTRUCTION: {
      if ((code + 9) > endOfMemory) {
	fprintf(out, " **ERROR: Address for %s trucated by end of memory\n",
		INSTRUCTION_NAME[*code]);
	return NULL;
      }

      uint64_t address = *(uint64_t*)(code + 1);
      if (!symtab || (address >= (startOfHeap - startOfMemory))) {
	fprintf(out, " %s %" PRIu64 "\n", INSTRUCTION_NAME[*code], address);
      } else {
	sym = getSymbolAtAddress(symtab, address);
	if (sym) {
	  fprintf(out, " %s %s\n", INSTRUCTION_NAME[*code], sym->name);
	} else {
	  sym = getSymbolBeforeAddress(symtab, address);
	  if (sym) {
	    fprintf(out, " %s %s+%" PRIu64 "\n", INSTRUCTION_NAME[*code],
		    sym->name, address - sym->address);
	  } else {
	    fprintf(out, " %s %" PRIu64 "\n", INSTRUCTION_NAME[*code], address);
	  }
	}
      }

      return code + 9;
    }

    case SAVE_INSTRUCTION:
    case RESTORE_INSTRUCTION:
      if ((code + 2) > endOfMemory) {
	fprintf(out, " **ERROR: Argument for %s truncated by end of memory\n",
		INSTRUCTION_NAME[*code]);
	return NULL;
      } else {
	fprintf(out, " %s %" PRId8 "\n", INSTRUCTION_NAME[*code], code[1]);
	return code + 2;
      }

    case PRINT_INSTRUCTION:
      if ((code + 2) > endOfMemory) {
	fprintf(out, " **ERROR: Argument for %s truncated by end of memory\n",
		INSTRUCTION_NAME[*code]);
	return NULL;
      } else if ((code[1] < 32) || ((code[1] >= 127) && (code[1] < 160))) {
	fprintf(out, " %s '\\x%02" PRIx8 "'\n", INSTRUCTION_NAME[*code],
		code[1]);
      } else {
	fprintf(out, " %s '%c'\n", INSTRUCTION_NAME[*code], code[1]);
      }
      return code + 2;

    default:
      if (*code < NUM_INSTRUCTIONS) {
	fprintf(out, " %s\n", INSTRUCTION_NAME[*code]);
      } else {
	fprintf(out, " %s\n", UNKNOWN_INSTRUCTION_NAME);
      }
      return code + 1;
  }
}

const char* disassembleOneLine(const uint8_t* code,
			       const uint8_t* startOfMemory,
			       const uint8_t* startOfHeap,
			       const uint8_t* endOfMemory,
			       SymbolTable symtab) {
  char* text = NULL;
  size_t textLength = 0;
  FILE* memstream = open_memstream(&text, &textLength);
  if (!memstream) {
    return NULL;
  }
  disassembleVmCode(code, startOfMemory, startOfHeap, endOfMemory,
		    symtab, memstream);
  fclose(memstream);
  return text;
}

void writeAddressWithSymbol(uint64_t address, int pad, uint64_t startOfHeap,
			    SymbolTable symtab, FILE* out) {
  if (pad) {
    fprintf(out, "%20" PRIu64, address);
  } else {
    fprintf(out, "%" PRIu64, address);
  }
  
  if (address < startOfHeap) {
    const Symbol* symbol = getSymbolAtOrBeforeAddress(symtab, address);
    if (symbol && symbol->address == address) {
      fprintf(out, " (%s)", symbol->name);
    } else if (symbol) {
      assert(symbol->address < address);
      fprintf(out, " (%s+%" PRIu64 ")", symbol->name,
	      (uint64_t)(address - symbol->address));
    }
  }
}
