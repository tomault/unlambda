#include "vm_instructions.h"

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

  Symbol* sym = getSymbolAtAddress(symtab, code);
  if (sym) {
    fprintf(out, "%21" PRIu64 "    %s:\n", (uint64_t)(code - startOfMemory),
	    sym->name);
  }

  fprintf(out, "%21" PRIu64 " %02" PRIx8 "   ",
	  (uint64_t)(code - startOfMemory), *code);
	  
  switch (*code) {
    case PUSH_INSTRUCTION: {
      if ((code + 9) > endOfMemory) {
	fprintf(" **ERROR: Address for %s trucated by end of memory\n",
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
      } else if ((code[1] < 32) || ((code[1] >= 128) && (code[1] < 160))) {
	fprintf(out, " %s '\\x%02" PRIu8 "'\n", INSTRUCTION_NAME[*code],
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
