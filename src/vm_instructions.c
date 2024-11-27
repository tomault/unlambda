#include "vm_instructions.h"

static const uint8_t INSTRUCTION_SIZE[] = {
    1, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1,
};

static const char* INSTRUCTION_NAME[] = {
    "PANIC", "PUSH", "POP", "SWAP", "DUP", "PCALL", "RET", "MKK", "MKS0",
    "MKS1", "MKS2", "MKD", "MKC", "SAVE", "RESTORE", "PRINT", "HALT"
};

uint8_t instructionSize(uint8_t instruction) {
  return INSTRUCTION_SIZE[instruction];
}

const char* instructionName(uint8_t instruction) {
  return INSTRUCTION_NAME[instruction];
}
