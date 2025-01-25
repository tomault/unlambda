#ifndef __UNLAMBDA__ASM_H__
#define __UNLAMBDA__ASM_H__

#include <symtab.h>
#include <stdint.h>
#include <stdio.h>

typedef struct AsmSymbolAndOffset_ {
  const char* symbolName;
  int64_t offset;
} AsmSymbolAndOffset;

typedef union AnyAsmValue_ {
  uint64_t u64;
  AsmSymbolAndOffset sando;
  const char* str;
} AnyAsmValue;

typedef struct TypedAsmValue_ {
  uint32_t type;
  AnyAsmValue value;
} TypedAsmValue;

TypedAsmValue* initEmptyAsmValue(TypedAsmValue* tv);
TypedAsmValue* initUInt64AsmValue(TypedAsmValue* tv, uint64_t value);
TypedAsmValue* initSymbolAndOffsetAsmValue(TypedAsmValue* tv,
					   const char* symbolName,
					   int64_t offset);
TypedAsmValue* initStringAsmValue(TypedAsmValue* tv, const char* value);
TypedAsmValue* initAsmValueFromOther(TypedAsmValue *tv, TypedAsmValue* other);

TypedAsmValue* setEmptyAsmValue(TypedAsmValue *tv);
TypedAsmValue* setUInt64AsmValue(TypedAsmValue* tv, uint64_t value);
TypedAsmValue* setSymbolAndOffsetAsmValue(TypedAsmValue* tv,
					  const char* symbolName,
					  int64_t offset);
TypedAsmValue* setStringAsmValue(TypedAsmValue* tv, const char* value);
TypedAsmValue* setAsmValueFromOther(TypedAsmValue* tv, TypedAsmValue *other);

TypedAsmValue* cleanAsmValue(TypedAsmValue* tv);

int printAsmValue(TypedAsmValue* tv, FILE* out);
int sprintAsmValue(char* buf, size_t size, TypedAsmValue *tv);

uint64_t resolveAsmValueToAddress(TypedAsmValue* tv, SymbolTable symtab,
				  const char** errorMessage);

#define ASM_VALUE_TYPE_NONE           0
#define ASM_VALUE_TYPE_UINT64         1
#define ASM_VALUE_TYPE_SYMBOL_OFFSET  2
#define ASM_VALUE_TYPE_STRING         3

typedef struct AsmInstruction_ {
  uint8_t opcode;
  TypedAsmValue operand;
} AsmInstruction;

typedef struct AsmDirective_ {
  uint8_t code;
  TypedAsmValue operand;
} AsmDirective;

typedef struct AsmLabel_ {
  const char* labelName;
} AsmLabel;

typedef struct AsmSymbolAssignment_ {
  const char* symbolName;
  TypedAsmValue value;
} AsmSymbolAssignment;

typedef union AssemblyLineData_ {
  AsmInstruction instruction;
  AsmDirective directive;
  AsmLabel label;
  AsmSymbolAssignment symassign;
} AssemblyLineData;

typedef struct AssemblyLine_ {
  uint32_t line;
  uint16_t column;
  uint16_t type;
  uint64_t address;
  const char* comment;
  AssemblyLineData value;
} AssemblyLine;

AssemblyLine* createEmptyAsmLine(uint64_t address, uint32_t line,
				 uint16_t column, const char* comment);
AssemblyLine* createInstructionAsmLine(uint64_t address, uint32_t line,
				       uint16_t column, uint8_t opcode,
				       TypedAsmValue* operand,
				       const char* comment);
AssemblyLine* createDirectiveAsmLine(uint64_t address, uint32_t line,
				     uint16_t column, uint8_t directiveCode,
				     TypedAsmValue* operand,
				     const char* comment);
AssemblyLine* createLabelAsmLine(uint64_t address, uint32_t line,
				 uint16_t column,
				 const char* labelName,
				 const char* comment);
AssemblyLine* createSymbolAssignmentAsmLine(uint64_t address, uint32_t line,
					    uint16_t column,
					    const char* symbolName,
					    TypedAsmValue* value,
					    const char* comment);

void destroyAssemblyLine(AssemblyLine* asmLine);

int printAssemblyLine(AssemblyLine* line, FILE* out);
int sprintAssemblyLine(char* buf, size_t size, AssemblyLine* asml);

typedef struct AsmParseError_ {
  uint32_t column;
  const char* message;
} AsmParseError;

void destroyAsmParseError(AsmParseError* parseError);

AssemblyLine* parseAssemblyLine(const char* text, uint64_t address,
				uint32_t lineNum, AsmParseError** parseError);

#define ASM_LINE_TYPE_EMPTY              0
#define ASM_LINE_TYPE_INSTRUCTION        1
#define ASM_LINE_TYPE_DIRECTIVE          2
#define ASM_LINE_TYPE_LABEL              3
#define ASM_LINE_TYPE_SYMBOL_ASSIGNMENT  4

#define START_ADDRESS_DIRECTIVE          1

#endif
