#include "asm.h"
#include "vm_instructions.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

TypedAsmValue* initEmptyAsmValue(TypedAsmValue* tv) {
  tv->type = ASM_VALUE_TYPE_NONE;
  tv->value.sando.symbolName = NULL;
  tv->value.sando.offset = 0;
  return tv;
}

TypedAsmValue* initUInt64AsmValue(TypedAsmValue* tv, uint64_t value) {
  tv->type = ASM_VALUE_TYPE_UINT64;
  tv->value.u64 = value;
  return tv;
}

TypedAsmValue* initSymbolAndOffsetAsmValue(TypedAsmValue* tv,
					   const char* symbolName,
					   int64_t offset) {
  tv->type = ASM_VALUE_TYPE_SYMBOL_OFFSET;
  tv->value.sando.symbolName = strdup(symbolName);
  tv->value.sando.offset = offset;
  return tv;
}

TypedAsmValue* initStringAsmValue(TypedAsmValue* tv, const char* value) {
  tv->type = ASM_VALUE_TYPE_STRING;
  tv->value.str = strdup(value);
  return tv;
}

TypedAsmValue* initAsmValueFromOther(TypedAsmValue *tv, TypedAsmValue* other) {
  switch (other->type) {
    case ASM_VALUE_TYPE_NONE:
      return initEmptyAsmValue(tv);

    case ASM_VALUE_TYPE_UINT64:
      return initUInt64AsmValue(tv, other->value.u64);

    case ASM_VALUE_TYPE_SYMBOL_OFFSET:
      return initSymbolAndOffsetAsmValue(tv, other->value.sando.symbolName,
					 other->value.sando.offset);

    case ASM_VALUE_TYPE_STRING:
      return initStringAsmValue(tv, other->value.str);

    default:
      /** Unknown type.  Return NULL to indicate this and initialize tv
       *    to a empty value
       */
      initEmptyAsmValue(tv);
      return NULL;
  }
}

TypedAsmValue* setEmptyAsmValue(TypedAsmValue *tv) {
  return initEmptyAsmValue(cleanAsmValue(tv));
}

TypedAsmValue* setUInt64AsmValue(TypedAsmValue* tv, uint64_t value) {
  return initUInt64AsmValue(cleanAsmValue(tv), value);
}

TypedAsmValue* setSymbolAndOffsetAsmValue(TypedAsmValue* tv,
					  const char* symbolName,
					  int64_t offset) {
  return initSymbolAndOffsetAsmValue(cleanAsmValue(tv), symbolName, offset);
}

TypedAsmValue* setStringAsmValue(TypedAsmValue* tv, const char* value) {
  return initStringAsmValue(cleanAsmValue(tv), value);
}

TypedAsmValue* setAsmValueFromOther(TypedAsmValue* tv, TypedAsmValue *other) {
  return initAsmValueFromOther(cleanAsmValue(tv), other);
}

TypedAsmValue* cleanAsmValue(TypedAsmValue* tv) {
  if (tv) {
    switch(tv->type) {
      case ASM_VALUE_TYPE_NONE:
      case ASM_VALUE_TYPE_UINT64:
	break;

      case ASM_VALUE_TYPE_SYMBOL_OFFSET:
	free((void*)tv->value.sando.symbolName);
	tv->value.sando.symbolName = NULL;
	break;

      case ASM_VALUE_TYPE_STRING:
	free((void*)tv->value.str);
	tv->value.str = NULL;
	break;

      default:
	/** Unknown type.  Do nothing */
	break;
    }

    tv->type = ASM_VALUE_TYPE_NONE;
  }
  return tv;
}

int printAsmValue(TypedAsmValue* tv, FILE* out) {
  if (!tv) {
    return 0;
  }
  
  switch(tv->type) {
    case ASM_VALUE_TYPE_NONE:
      return 0;

    case ASM_VALUE_TYPE_UINT64:
      return fprintf(out, "%" PRIu64, tv->value.u64);

    case ASM_VALUE_TYPE_SYMBOL_OFFSET:
      if (tv->value.sando.offset >= 0) {
	return fprintf(out, "%s+%" PRId64, tv->value.sando.symbolName,
		       tv->value.sando.offset);
      } else {
	return fprintf(out, "%s%" PRId64, tv->value.sando.symbolName,
		       tv->value.sando.offset);
      }
      
    case ASM_VALUE_TYPE_STRING:
      return fprintf(out, "%s", tv->value.str);

    default:
      return fprintf(out, "TypedAsmValue with unknown type %" PRIu32,
		     tv->type);
  }
  
}

int sprintAsmValue(char* buf, size_t size, TypedAsmValue *tv) {
  if (!tv) {
    return 0;
  }
  
  switch(tv->type) {
    case ASM_VALUE_TYPE_NONE:
      return 0;

    case ASM_VALUE_TYPE_UINT64:
      return snprintf(buf, size, "%" PRIu64, tv->value.u64);

    case ASM_VALUE_TYPE_SYMBOL_OFFSET:
      if (tv->value.sando.offset >= 0) {
	return snprintf(buf, size, "%s+%" PRId64, tv->value.sando.symbolName,
			tv->value.sando.offset);
      } else {
	return snprintf(buf, size, "%s%" PRId64, tv->value.sando.symbolName,
			tv->value.sando.offset);
      }
      
    case ASM_VALUE_TYPE_STRING:
      return snprintf(buf, size, "%s", tv->value.str);

    default:
      return snprintf(buf, size, "TypedAsmValue with unknown type %" PRIu32,
		      tv->type);
  }
}

uint64_t resolveAsmValueToAddress(TypedAsmValue* tv, SymbolTable symtab,
				  const char** errorMessage) {
  const Symbol* sym = NULL;
  const char* symbolName = NULL;
  *errorMessage = NULL;
  
  switch (tv->type) {
    case ASM_VALUE_TYPE_NONE:
      *errorMessage = strdup("Cannot resolve value of type NONE to address");
      return 0;

    case ASM_VALUE_TYPE_UINT64:
      return tv->value.u64;

    case ASM_VALUE_TYPE_SYMBOL_OFFSET:
    case ASM_VALUE_TYPE_STRING:
      if (tv->type == ASM_VALUE_TYPE_SYMBOL_OFFSET) {
	symbolName = tv->value.sando.symbolName;
      } else {
	symbolName = tv->value.str;
      }
      sym = findSymbol(symtab, symbolName);
      if (!sym) {
	char msg[200];
	snprintf(msg, sizeof(msg), "Cannot resolve unknown symbol \"%s\"",
		 symbolName);
	*errorMessage = strdup(msg);
	return 0;
      }
      if (tv->type == ASM_VALUE_TYPE_SYMBOL_OFFSET) {
	return sym->address + tv->value.sando.offset;
      } else {
	return sym->address;
      }
      break;

    default: {
      char msg[200];
      snprintf(msg, sizeof(msg), "Canot resolve value of unknown type %" PRIu32,
	       tv->type);
      *errorMessage = strdup(msg);
      return 0;
    }
  }      
}

static AssemblyLine* createAsmLine(uint64_t address, uint16_t type,
				   uint32_t line, uint16_t column,
				   const char* comment) {
  AssemblyLine* asml = (AssemblyLine*)malloc(sizeof(AssemblyLine));
  if (asml) {
    asml->address = address;
    asml->type = type;
    asml->line = line;
    asml->column = column;
    asml->comment = comment ? strdup(comment) : NULL;
  }
  return asml;
}

AssemblyLine* createEmptyAsmLine(uint64_t address, uint32_t line,
				 uint16_t column, const char* comment) {
  return createAsmLine(address, ASM_LINE_TYPE_EMPTY, line, column, comment);
}

AssemblyLine* createInstructionAsmLine(uint64_t address, uint32_t line,
				       uint16_t column, uint8_t opcode,
				       TypedAsmValue* operand,
				       const char* comment) {
  AssemblyLine* asml = createAsmLine(address, ASM_LINE_TYPE_INSTRUCTION,
				     line, column, comment);
  if (asml) {
    asml->value.instruction.opcode = opcode;
    initAsmValueFromOther(&asml->value.instruction.operand, operand);
  }
  return asml;
}

AssemblyLine* createDirectiveAsmLine(uint64_t address, uint32_t line,
				     uint16_t column, uint8_t directiveCode,
				     TypedAsmValue* operand,
				     const char* comment) {
  AssemblyLine* asml = createAsmLine(address, ASM_LINE_TYPE_DIRECTIVE, line,
				     column, comment);
  if (asml) {
    asml->value.directive.code = directiveCode;
    initAsmValueFromOther(&asml->value.directive.operand, operand);
  }
  return asml;
}

AssemblyLine* createLabelAsmLine(uint64_t address, uint32_t line,
				 uint16_t column, const char* labelName,
				 const char* comment) {
  AssemblyLine* asml = createAsmLine(address, ASM_LINE_TYPE_LABEL, line, column,
				     comment);
  if (asml) {
    asml->value.label.labelName = strdup(labelName);
  }
  return asml;
}

AssemblyLine* createSymbolAssignmentAsmLine(uint64_t address, uint32_t line,
					    uint16_t column,
					    const char* symbolName,
					    TypedAsmValue* value,
					    const char* comment) {
  AssemblyLine* asml = createAsmLine(address, ASM_LINE_TYPE_SYMBOL_ASSIGNMENT,
				     line, column, comment);
  if (asml) {
    asml->value.symassign.symbolName = strdup(symbolName);
    initAsmValueFromOther(&asml->value.symassign.value, value); 
  }
  return asml;
}

void destroyAssemblyLine(AssemblyLine* asmLine) {
  if (!asmLine) {
    return;
  }

  switch(asmLine->type) {
    case ASM_LINE_TYPE_EMPTY:
      break;

    case ASM_LINE_TYPE_INSTRUCTION:
      cleanAsmValue(&asmLine->value.instruction.operand);
      break;

    case ASM_LINE_TYPE_DIRECTIVE:
      cleanAsmValue(&asmLine->value.directive.operand);
      break;

    case ASM_LINE_TYPE_LABEL:
      free((void*)asmLine->value.label.labelName);
      break;

    case ASM_LINE_TYPE_SYMBOL_ASSIGNMENT:
      free((void*)asmLine->value.symassign.symbolName);
      cleanAsmValue(&asmLine->value.symassign.value);
      break;

    default:
      break;
  }

  free((void*)asmLine->comment);
  free((void*)asmLine);
}

int printAssemblyLine(AssemblyLine* asml, FILE* out) {
  if (!asml) {
    return 0;
  }
  int prefix = fprintf(out, "type=%" PRIu16 ", address=%" PRIu64 ", line=%"
		       PRIu32 ", column=%" PRIu16, asml->type, asml->address,
		       asml->line, asml->column);
  if (prefix < 0) {
    return prefix;
  }

  int middle = 0;

  switch(asml->type) {
    case ASM_LINE_TYPE_EMPTY:
      break;

    case ASM_LINE_TYPE_INSTRUCTION:
      middle = fprintf(out, ", opcode=%" PRIu16 ", operand=",
		       (uint16_t)asml->value.instruction.opcode);
      if (middle >= 0) {
	int n = printAsmValue(&asml->value.instruction.operand, out);
	middle = (n >= 0) ? middle + n : n;
      }
      break;

    case ASM_LINE_TYPE_DIRECTIVE:
      middle = fprintf(out, ", code=%" PRIu16 ", operand=",
		       (uint16_t)asml->value.directive.code);
      if (middle >= 0) {
	int n = printAsmValue(&asml->value.directive.operand, out);
	middle = (n >= 0) ? middle + n : n;
      }
      break;

    case ASM_LINE_TYPE_LABEL:
      middle = fprintf(out, ", label=\"%s\"", asml->value.label.labelName);
      break;
      
    case ASM_LINE_TYPE_SYMBOL_ASSIGNMENT:
      middle = fprintf(out, ", symbol=\"%s\", value=",
		       asml->value.symassign.symbolName);
      if (middle >= 0) {
	int n = printAsmValue(&asml->value.symassign.value, out);
	middle = (n >= 0) ? middle + n : n;
      }
      break;

    default:
      middle = fprintf(out, " **BAD TYPE**");
      break;
  }

  if (middle < 0) {
    return middle;
  }

  int suffix;
  if (asml->comment) {
    suffix = fprintf(out, ", comment=\"%s\"", asml->comment);
  } else {
    suffix = 0;
  }
  return (suffix < 0) ? suffix : prefix + middle + suffix;
}

int sprintAssemblyLine(char* buf, size_t size, AssemblyLine* asml) {
  /** This code is slow, but gets the job done and good enough for its
   *  expected use cases
   */
  char* text = NULL;
  size_t textLen = 0;
  FILE* stream = open_memstream(&text, &textLen);

  if (!stream) {
    return 0;
  }

  printAssemblyLine(asml, stream);
  fclose(stream);

  if (!text) {
    return 0;
  }

  int n = snprintf(buf, size, "%s", text);
  free((void*)text);
  return n;
}

void destroyAsmParseError(AsmParseError* parseError) {
  if (parseError) {
    free((void*)parseError->message);
    free((void*)parseError);
  }
}
  
/** Assembly grammar
 *
 *  asm_line := stmt? commennt?
 *  stmt := instruction | directive | label | sym-assign
 *  comment := /#.*$/
 *  instruction:= operator address?
 *  directive := start-directive
 *  start-directive := ".START" address
 *  sym-assign := symbol "=" address
 *  address := u64 | symbol | symbol "+" u64 | symbol "-" u64
 *  u64 := /[0-9]+/
 *  symbol := /[A-Za-z][A-Za-z0-9_]* /
 *  operator := "PANIC" | "PUSH" | "POP" | "SWAP" | "DUP" | "PCALL" | "RET" |
 *              "MKK" | "MKS0" | "MKS1" | "MKS2" | "MKD" | "MKC" | "SAVE" |
 *              "RESTORE" | "PRINT" | "HALT"
 */


static const char* skipWhitespace(const char* p);
static AsmParseError* createParseError(const char* message, uint32_t column);
static const char* readSymbol(const char* p, const char** symbolName);
static int findOpcode(const char* text, uint8_t* opcode);
static AssemblyLine* parseInstruction(const char* text, uint64_t address,
				      uint32_t lineNum, uint16_t column,
				      const char* start,
				      const char* instructionName,
				      uint8_t opcode,
				      AsmParseError** parseError);
static AssemblyLine* parseDirective(const char* text, uint64_t address,
				    uint32_t lineNum, uint16_t column,
				    const char* start,
				    AsmParseError** parseError);
static AssemblyLine* parseLabel(const char* text, uint64_t address,
				uint32_t lineNum, uint16_t column,
				const char* start,
				const char* labelName,
				AsmParseError** parseError);
static AssemblyLine* parseSymbolAssignment(const char* text, uint64_t address,
					   uint32_t lineNum, uint16_t column,
					   const char* start,
					   const char* symbolName,
					   AsmParseError** parseError);
static const char* parseAddress(const char* text, const char* start,
				TypedAsmValue* address,
				AsmParseError** parseError);
static const char* parseUInt64Operand(const char* text, const char* start,
				      uint64_t* operand,
				      AsmParseError** parseError);
static const char* parseCharacterOperand(const char* text, const char* start,
					 uint8_t* operand,
					 AsmParseError** parseError);
static const char* checkForComment(const char* text, const char* start,
				   AsmParseError** parseError);

AssemblyLine* parseAssemblyLine(const char* text, uint64_t address,
				uint32_t lineNum, AsmParseError** parseError) {
  const char* p = skipWhitespace(text);

  *parseError = NULL;
  
  if (!*p) {
    return createEmptyAsmLine(address, lineNum, 0, NULL);
  } else if (*p == '#') {
    return createEmptyAsmLine(address, lineNum, p - text, p + 1);
  } else if (*p == '.') {
    return parseDirective(text, address, lineNum, p - text, p + 1, parseError);
  } else if (!isalpha(*p)) {
    *parseError = createParseError("Syntax error", p - text);
    return NULL;
  } else {
    const char* symStart = p;
    const char* symbolName = NULL;
    uint8_t opcode = 255;

    p = skipWhitespace(readSymbol(p, &symbolName));
    if (!findOpcode(symbolName, &opcode)) {
      return parseInstruction(text, address, lineNum, symStart - text, p,
			      symbolName, opcode, parseError);
    } else if (*p == ':') {
      /** It's a label */
      return parseLabel(text, address, lineNum, symStart - text, p + 1,
			symbolName, parseError);
/**
    } else if (*p == '=') {
      p = skipWhitespace(p + 1)
      return parseSymbolAssignment(text, address, lineNum, symStart - text, p, 
                                   symbolName, parseError);
**/
    } else {
      /** It's an invalid instruction */
      *parseError = createParseError("Invalid instruction", symStart - text);
      free((void*)symbolName);
      return NULL;
    }
  }
}

static const char* skipWhitespace(const char* p) {
  while (*p && isspace(*p)) {
    ++p;
  }
  return p;
}

static AsmParseError* createParseError(const char* message, uint32_t column) {
  AsmParseError* parseError = (AsmParseError*)malloc(sizeof(AsmParseError));

  if (parseError) {
    parseError->column = column;
    parseError->message = strdup(message);
  }
  
  return parseError;
}

static const char* readSymbol(const char* p, const char** symbolName) {
  const char* start = p;
  while (isalnum(*p) || (*p == '_')) {
    ++p;
  }
  assert(p > start);

  char* sym = malloc(p - start + 1);
  memcpy(sym, start, p - start);
  sym[p - start] = 0;
  *symbolName = sym;
  return p;
}

static int findOpcode(const char* text, uint8_t* opcode) {
  for (uint8_t opc = 0; opc < NUM_VM_INSTRUCTIONS; ++opc) {
    const char* name = instructionName(opc);
    if (name && !strcasecmp(text, name)) {
      *opcode = opc;
      return 0;
    }
  }
  *opcode = 255;
  return -1;
}

static AssemblyLine* parseInstruction(const char* text, uint64_t address,
				      uint32_t lineNum, uint16_t column,
				      const char* start,
				      const char* instructionName,
				      uint8_t opcode,
				      AsmParseError** parseError) {
  TypedAsmValue operand;
  const char* p = NULL;
  
  if (opcode == PUSH_INSTRUCTION) {
    p = parseAddress(text, start, &operand, parseError);
    if (*parseError) {
      free((void*)instructionName);
      return NULL;
    }
  } else if ((opcode == SAVE_INSTRUCTION) || (opcode == RESTORE_INSTRUCTION)) {
    uint64_t v = 0;
    p = parseUInt64Operand(text, start, &v, parseError);
    if (*parseError) {
      free((void*)instructionName);
      return NULL;
    }
    if (v > 255) {
      *parseError = createParseError("Operand must be in the range 0-255",
				     start - text);
      free((void*)instructionName);
      return NULL;
    }
    initUInt64AsmValue(&operand, v);
  } else if (opcode == PRINT_INSTRUCTION) {
    uint8_t v = 0;
    p = parseCharacterOperand(text, start, &v, parseError);
    if (*parseError) {
      free((void*)instructionName);
      return NULL;
    }
    initUInt64AsmValue(&operand, v);
  } else {
    initEmptyAsmValue(&operand);
    p = start;
  }

  const char* comment = checkForComment(text, p, parseError);
  free((void*)instructionName);
  if (*parseError) {
    cleanAsmValue(&operand);
    return NULL;
  }
  AssemblyLine* result = createInstructionAsmLine(
    address, lineNum, column, opcode, &operand, comment
  );
  cleanAsmValue(&operand);
  return result;
}

static AssemblyLine* parseDirective(const char* text, uint64_t address,
				    uint32_t lineNum, uint16_t column,
				    const char* start,
				    AsmParseError** parseError) {
  if (isspace(*start) || (*start == '#')) {
    *parseError = createParseError("Directive name missing", start - text);
    return NULL;
  } else if (!isalpha(*start)) {
    *parseError = createParseError("Syntax error", start - text);
    return NULL;
  }

  const char* directiveName = NULL;
  const char* p = readSymbol(start, &directiveName);

  if (!strcasecmp(directiveName, "start")) {
    TypedAsmValue operand;
    p = parseAddress(text, skipWhitespace(p), &operand, parseError);
    if (*parseError) {
      free((void*)directiveName);
      return NULL;
    }

    const char* comment = checkForComment(text, p, parseError);
    free((void*)directiveName);
    if (*parseError) {
      cleanAsmValue(&operand);
      return NULL;
    }
    AssemblyLine* result = createDirectiveAsmLine(
      address, lineNum, start - text - 1, START_ADDRESS_DIRECTIVE, &operand,
      comment
    );
    cleanAsmValue(&operand);
    return result;
  } else {
    *parseError = createParseError("Unknown directive", start - text);
    free((void*)directiveName);
    return NULL;
  }
}

static AssemblyLine* parseLabel(const char* text, uint64_t address,
				uint32_t lineNum, uint16_t column,
				const char* start, const char* labelName,
				AsmParseError** parseError) {
  const char* comment = checkForComment(text, start, parseError);
  if (*parseError) {
    free((void*)labelName);
    return NULL;
  }

  AssemblyLine* asml = createLabelAsmLine(address, lineNum, column, labelName,
					  comment);
  free((void*)labelName);
  return asml;
}

static AssemblyLine* parseSymbolAssignment(const char* text, uint64_t address,
					   uint32_t lineNum, uint16_t column,
					   const char* start,
					   const char* symbolName,
					   AsmParseError** parseError) {
  TypedAsmValue operand;
  const char* p = parseAddress(text, start, &operand, parseError);
  if (*parseError) {
    free((void*)symbolName);
    return NULL;
  }

  const char* comment = checkForComment(text, p, parseError);
  if (*parseError) {
    free((void*)symbolName);
    cleanAsmValue(&operand);
    return NULL;
  }

  AssemblyLine* asml = createSymbolAssignmentAsmLine(
    address, lineNum, column, symbolName, &operand, comment
  );
  free((void*)symbolName);
  cleanAsmValue(&operand);
  return asml;
}
  
static const char* parseAddress(const char* text, const char* start,
				TypedAsmValue* address,
				AsmParseError** parseError) {
  const char* p = skipWhitespace(start);
  if (!*p || (*p == '#')) {
    *parseError = createParseError("Operand missing", p - text);
    return NULL;
  } else if (isdigit(*p)) {
    /** Numeric address */
    uint64_t v = 0;
    p = parseUInt64Operand(text, p, &v, parseError);
    if (*parseError) {
      return NULL;
    }
    initUInt64AsmValue(address, v);
    return p;
  } else if (!isalpha(*p)) {
    *parseError = createParseError("Syntax error", p - text);
  } else {
    /** Symbol possibly followed by offset */
    const char* symbolName = NULL;
    uint64_t offset = 0;
    
    p = skipWhitespace(readSymbol(p, &symbolName));

    if (!*p || (*p == '#')) {
      /** Just a symbol */
      offset = 0;
    } else if ((*p == '+') || (*p == '-')) {
      /** Symbol followed by offset */
      int isNegative = *p == '-';
      p = parseUInt64Operand(text, skipWhitespace(p + 1), &offset, parseError);
      if (*parseError) {
	free((void*)symbolName);
	return NULL;
      }
      if (isNegative) {
	offset = -offset;
      }
    } else {
      *parseError = createParseError("Syntax error", p - text);
      free((void*)symbolName);
      return NULL;
    }

    initSymbolAndOffsetAsmValue(address, symbolName, (int64_t)offset);
    free((void*)symbolName);
    return p;
  }
}

/** TODO: Create a single UInt64 parsing routine */
static const uint64_t MAX_UINT64_DIV_10 = (uint64_t)0xFFFFFFFFFFFFFFFF / 10;
static const uint64_t MAX_UINT64_MOD_10 = (uint64_t)0xFFFFFFFFFFFFFFFF % 10;

static const char* parseUInt64Operand(const char* text, const char* start,
				      uint64_t* value,
				      AsmParseError** parseError) {
  uint64_t v = 0;
  const char* p = skipWhitespace(start);

  if (!*p || (*p == '\n')) {
    *parseError = createParseError("Operand missing", p - text);
    return NULL;
  } else if (!isdigit(*p)) {
    *parseError = createParseError("Operand must be an integer", p - text);
    return NULL;
  }

  const char* numStart = p;
  
  while (*p && isdigit(*p)) {
    int d = *p - '0';
    if ((v > MAX_UINT64_DIV_10)
	  || ((v == MAX_UINT64_DIV_10) && (d > MAX_UINT64_MOD_10))) {
      *parseError = createParseError("Value is too large", numStart - text);
      return NULL; 
    }
    v = v * 10 + d;
    ++p;
  }

  *value = v;
  return p;
}

static uint8_t hexDigitToDec(char c) {
  c &= ~48;
  return (c < 10) ? c : c - 55;
}

static const char* parseHexCharEscape(size_t literalStart,
				      const char* start,
				      uint8_t* value,
				      AsmParseError** parseError) {
  if (!isxdigit(start[0]) || !isxdigit(start[1])) {
    *parseError = createParseError(
      "Invalid character literal (Invalid hex escape sequence)",
      literalStart
    );
    return NULL;
  }
  *value = hexDigitToDec(start[0]) << 4 | hexDigitToDec(start[1]);
  return start + 2;
}

static const char* parseEscapeSequence(size_t literalStart,
				       const char* start,
				       uint8_t* value,
				       AsmParseError** parseError) {
  /** Parse escape sequence */
  if (*start == 'n') {
    *value = '\n';
  } else if (*start == 't') {
    *value = '\t';
  } else if (*start == 'r') {
    *value = '\r';
  } else if (*start == '\'') {
    *value = '\'';
  } else if (*start == '\\') {
    *value = '\\';
  } else if (*start != 'x') {
    *parseError = createParseError("Invalid character literal (Unknown "
				   "escape sequence)", literalStart);
    return NULL;
  } else {
    return parseHexCharEscape(literalStart, start + 1, value, parseError);
  }
  return start + 1;
}

static const char* parseCharacterOperand(const char* text, const char* start,
					 uint8_t* value,
					 AsmParseError** parseError) {
  const char *p = skipWhitespace(start);

  if (!*p || (*p == '\n')) {
    *parseError = createParseError("Operand missing", p - text);
    return NULL;
  } else if (*p != '\'') {
    *parseError = createParseError("Invalid character literal "
				   "(must start with \"'\")", p - text);
    return NULL;
  }

  const char* literalStart = p++;
  
  if (!*p || (*p == '\n') || (*p == '\'')) {
    *parseError = createParseError("Invalid character literal "
				   "(literal is empty)", literalStart - text);
    return NULL;
  } else if (*p != '\\') {
    *value = *p++;
  } else {
    p = parseEscapeSequence(literalStart - text, p + 1, value, parseError);
    if (*parseError) {
      return NULL;
    }
  }

  /** Check for closing "'" */
  if (*p != '\'') {
    *parseError = createParseError(
      "Invalid character literal (Closing \"'\" missing)",
      literalStart - text
    );
    return NULL;
  }
  return p + 1;
}

static const char* checkForComment(const char* text, const char* start,
				   AsmParseError** parseError) {
  const char* p = skipWhitespace(start);
  if (*p == '#') {
    return p + 1;
  } else if (*p) {
    *parseError = createParseError("Syntax error", p - text);
    return NULL;
  } else {
    return NULL;
  }
}
