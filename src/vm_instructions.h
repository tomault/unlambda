#ifndef __VM_INSTRUCTIONS_H__
#define __VM_INSTRUCTIONS_H__

#include <stdint.h>

/** Opcodes for instructions */
#define PANIC_INSTRUCTION   (uint8_t)0
#define PUSH_INSTRUCTION    (uint8_t)1
#define POP_INSTRUCTION     (uint8_t)2
#define SWAP_INSTRUCTION    (uint8_t)3
#define DUP_INSTRUCTION     (uint8_t)4
#define PCALL_INSTRUCTION   (uint8_t)5
#define RET_INSTRUCTION     (uint8_t)6
#define MKK_INSTRUCTION     (uint8_t)7
#define MKS0_INSTRUCTION    (uint8_t)8
#define MKS1_INSTRUCTION    (uint8_t)9
#define MKS2_INSTRUCTION    (uint8_t)10
#define MKD_INSTRUCTION     (uint8_t)11
#define MKC_INSTRUCTION     (uint8_t)12
#define SAVE_INSTRUCTION    (uint8_t)13
#define RESTORE_INSTRUCTION (uint8_t)14
#define PRINT_INSTRUCTION   (uint8_t)15
#define HALT_INSTRUCTION    (uint8_t)16


/** Number of bytes the instruction occupies, including both operator and
 *  operand
 */
uint8_t instructionSize(uint8_t instruction);

/** Human-readable instruction mnemonic */
const char* instructionName(uint8_t instruction);

#endif
