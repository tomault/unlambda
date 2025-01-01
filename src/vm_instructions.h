#ifndef __VM_INSTRUCTIONS_H__
#define __VM_INSTRUCTIONS_H__

#include <stdint.h>
#include <stdio.h>
#include <symtab.h>

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

/** Disassemble a line of code 
 *
 *  Disassemble a line of code starting at "code" and print the result to
 *  "out."  The "startOfMemory," "startOfHeap" and "endOfMemory" are used
 *  to keep pointers in-bounds, to detect errors and to decide how to
 *  express addresses.  Addresses between the start of memory and the start
 *  of the heap are written as "nearest symbol + displacement"  while
 *  addresses between the start of the heap and the end of memory are
 *  written as decimal numbers.
 *
 *  The disassembleVmCode() function prints diagnostics to "out" if an
 *  error occurs, such as "code" being outside the bounds of memory or
 *  an argument being cut off by the end of memory.
 *
 *  Arguments:
 *    code            Start disassembly here
 *    startOfMemory   Start of the VMs memory
 *    startOfHeap     Start of the heap.  Everything in [startOfMemory,
 *                      startOfHeap) is code space, while everything
 *                      in [startOfHeap, endOfMemory) is heap space.
 *    endOfMemory     End of the VMs memory.
 *    symtab          Table used to look up symbols to produce a cleaner
 *                      looking disassembly.  May be NULL, in which case
 *                      all addresses are written as decimal numbers
 *    out             Where to write the disassembly
 *
 *  Returns:
 *    A pointer to the next line of code, or endOfMemory if the end of
 *    memory has been reached.  Returns NULL if an error occurs.
 */
const uint8_t* disassembleVmCode(const uint8_t* code,
				 const uint8_t* startOfMemory,
				 const uint8_t* startOfHeap,
				 const uint8_t* endOfMemory,
				 SymbolTable symtab, FILE* out);
#endif
