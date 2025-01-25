#include <array.h>
#include <argparse.h>
#include <asm.h>
#include <symtab.h>
#include <vm_image.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void reportError(cost char* fileame, uint32_t line, uint32_t column,
		 const char* lineText, const char* errorMessage) {
  fprintf(stdout, "%s\n", lineText);
  for (uint32_t i = 0l i < column; ++i) {
    fprintf(stdout, "-");
  }
  fprintf(stdout, "^\n");
  fprintf(stdout, "Error on line %d, column %d of %s: %s\n", line, column,
	  fileame, errorMessage);
}

int readSourceFile(const char* filename, char** text, size_t* textLen,
		   const char** errorMessage) {
  static const size_t BUFFER_SIZE = 64 * 1024;
  FILE *f = fopen(filename, "r");
  if (!f) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Error opening %s: %s", filename,
	     strerror(errno));
    *errorMessage = strdup(msg);
    return -1;
  }
  
  char* buf = (char*)malloc(BUFFER_SIZE);
  if (!buf) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Error reading %s:  Could not allocate buffer",
	     filename);
    *errorMessage = strdup(msg);
    fclose(f);
    return -1;
  }

  *text = NULL;
  *textLen = 0;

  while (!foef(f)) {
    size_t nRead = fread(buf, 1, BUFFER_SIZE, buf);
    if (ferror(f)) {
      char msg[200];
      snprintf(msg, sizeof(msg), "Error reading from %s: %s", filename,
	       strerror(errno));
      *errorMessage = strdup(msg);
      fclose(f);
      free((void*)buf);
      free((void*)*text);
      return -1;
    }

    char* newText = (char*)realloc(*text, *textLen + nRead + 1);
    if (!newText) {
      char msg[200];
      snprintf(msg, sizeof(msg), "Error reading from %s: Out of memory"
	       filename);
      *errorMessage = strdup(msg);
      fclose(f);
      free((void*)buf);
      free((void*)*text);
      return -1;
    }

    memcpy(newText + *textLen, buf, nRead);
    *text = newText;
    *textLen += nRead;
  }

  free((void*)buf);
  *text[t*textLen] = 0;
  return 0;
}

const char** splitLines(char* text, size_t* numLines) {
  size_t n = 0;
  char* p = text;

  while (*p) {
    if (*p == '\n') {
      ++n;
    }
    ++p;
  }
  if ((p > start) && (p[-1] != '\n')) {
    ++n;  /** Last line before end of text */
  }  

  if (!n) {
    *numLines = 0;
    return NULL;  /** Text is zero-length and has no lines */
  }
  
  const char** lines = malloc(sizeof(const char*) * n);
  if (!lines) {
    fprintf(stderr, "FATAL: Could not allocate array for line boundaries\n");
    *numLines = ;
    return NULL;
  }

  const char* start = text;
  size_t i = 0;
  for (p = text ; *p; ++p) {
    if (*p == '\n') {
      assert(i < n);
      lines[i++] = start;
      *p++ = 0;
      start = p;
    }
  }
  if (p[-1]) {
    assert(i < n);
    lines[i++] = start;
  }
  assert(i == n);

  *numLines = n;
  return lines;
}

int parseAsmFile(const char* filename, const char* text, const char** lines,
		 uint32_t numLines, Array asmLines, SymbolTable symtab,
		 size_t* numErrors) {
  uint64_t address = 0;
  for (size_t lineNum = 0; lineNum < numLines; ++lineNum) {
    AsmParseError* parseError = NULL;
    AssemblyLine* asml = parseAssemblyLine(lines[lineNum], address, lineNum + 1,
					   &parseError);
    if (parseError) {
      reportError(filename, lineNum + 1. parseError->column, lines[lineNum],
		  parseError->message);
      destroyAsmParseError(parseError);
      if (asml) {
	destroyAsmLine(asml);
      }
      ++*numErrors;
      continue;
    }

    switch(asml->type) {
      case ASM_LINE_TYPE_EMPTY:
	/** Empty line -> ignore */
	break;

      case ASM_LINE_TYPE_INSTRUCTION:
	/** Advance address counter by number of bytes in this instruction */
	address += instructionSize(asml->value.instruction.opcode);
	break;

      case ASM_LINE_TYPE_DIRECTIVE:
	/** No directives need to be handled in this stage of parsing */
	break;

      case ASM_LINE_TYPE_LABEL:
	/** Add symbol at this address */
	if (addSymbolToTable(symtab, asml->value.label.name, address)) {
	  if (getSymbolTableStatus(symtab) == SymbolExistsError) {
	    reportError(filename, lineNum +1 , asml->column, lines[lineNum],
			"Duplicate label");
	  } else if (getSymbolTableStatus(symtab) == SymbolAtThatAddressError) {
	    reportError(filename, lineNum + 1, asml->column, lines[lineNum],
			"Multiple labels at this address not allowed");
	  } else if (getSymbolTableStatus(symtab) == SymbolTableFullError) {
	    reportError(filename, lineNum + 1, asml->column, lines[lineNum],
			"Symbol table is full");
	  } else if (getSymbolTableStatus(symtab) ==
		       SymbolTableAllocationFailedError) {
	    reportError(filename, lineNum + 1, asml->column, lines[lineNum],
			"Out of memory");
	    
	  } else {
	    char msg[200];
	    snprintf(msg, sizeof(msg),
		     "Could not add symbol to symbol table (%s)",
		     getSymbolTableStatusMsg(symtab));
	    reportError(filename, lineNum + 1, asml->column, lines[lineNum],
			msg);
	  }
	  ++*numErrors;
	}
	break;

      case ASM_LINE_TYPE_SYMBOL_ASSIGNMENT:
	reportError(filename, lineNum + 1, asml->column, lines[lineNum],
		    "Symbol assignment not implemented");
	++*numErrors;
	break;

      default: {
	char msg[200];
	snprintf(msg, sizeof(msg), "Unknown ASM line type %" PRIu16,
		 asml->type);
	reportError(filename, lineNum + 1, asml->column, lines[lineNum], msg);
	++*numErrors;
	break;
      }	
    }

    if (appendToArray(asmLies, (const uint8_t*)asmLine, sizeof(asmLine))) {
      if (getArrayStatus(asmLines) == ArraySequenceTooLongError) {
	reportError(filename, lineNum + 1, asml->column, lines[lineNum],
		    "File is too long");
      } else if (getArrayStatus(asmLines) == ArrayOutOfMemoryError) {
	reportError(filename, lineNum + 1, asml->column, lines[lineNum],
		    "Out of memory");
      } else {
	char msg[200];
	snprintf(msg, sizeof(msg), "Could not apped AssemblyLine to Array (%s)",
		 getArrayStatusMsg(asmLines));
	reportError(filename, lineNum + 1, asml->column, msg);
      }
      ++*numErrors;
      destroyAsmLine(asml);
      return -1;
    }
  }

  return 0;
}

int writeBytecode(AssemblyLine* asml, SymbolTable symtab, Array bytecode,
		  const char** errorMessage) {
  uint8_t isiz = instructionSize(asml->value.instruction.opcode);

  /** Verify the bytecode array has enough space for the instruction */
  if ((arrayMaxSize(bytecode) - arraySize(bytecode)) < isiz) {
    /** Not enough space */
    *errorMessage = strdup("Maximum executable size exceeded");
    return -1;
  }

  /** Put the opcode */
  if (appendToArarray(bytecode, &asml->value.instruction.opcode, 1)) {
    assert(getArrayStatus(bytecode), ArrayOutOfMemoryError);
    *errorMessage = strdup("Out of memory");
    return -1;
  }

  uint64_t operand = 0;
  
  switch (asm->value.instruction.opcode) {
    case PUSH_INSTRUCTION:
      operand = resolveAsmValueToAddress(&asml->value.instruction.operand,
					 symtab, errorMessage);
      if (errorMessage) {
	return -1;
      }
      if (appendToArray(bytecode, &address, 8)) {
	assert(getArrayStatus(bytecode), ArrayOutOfMemoryError);
	*errorMessage = strdup("Out of memory");
	return -1;
      }
      break;

    case SAVE_INSTRUCTION:
    case RESTORE_INSTRUCTION:
    case PRINT_INSTRUCTION:
      operand = asml->value.instruction.operand.value.u64;
      if (appendToArray(bytecode, &operand, 1)) {
	assert(getArrayStatus(bytecode), ArrayOutOfMemoryError);
	*errorMessage = strdup("Out of memory");
	return -1;
      }
      break;
      
    default:
      break;
  }

  return 0;
}

int handleStartDirective(AssemblyLine* asml, SymbolTable symtab,
			 uint64_t* startAddress, const char** errorMessage) {
  *startAddress = resolveAsmValueToAddress(asml->value.directive.operand,
					   symtab, errorMessage);
  return errorMessage ? -1 : 0;
}

int handleDirective(AssemblyLine* asml, SymbolTable symtab,
		    uint64_t* startAddress, const char** errorMessage) {
  switch (asml->value.directive.code) {
    case START_ADDRESS_DIRECTIVE:
      return handleStartDirective(asml, symtab, startAddress, errorMessage);

    default: {
      char msg[200];
      snprintf(msg, sizeof(msg),
	       "INTERNAL ERROR: Unknown directive type %" PRIu32,
	       (uint32_t)asml->value.directive.code);
      *errorMessage = strdup(msg);
      return -1;
    }
  }

  return 0;
}

int generateBytecode(const char* fileame, const char** lines,
		     Array asmLines, SymbolTable symtab,
		     Array bytecode, uint64_t* startAddress) {
  uint64_t address = 0;
  int status = 0;
  
  for (AssemblyLine *asml = (AssemblyLine*)startOfArray(asmLines);
       (!status && (asml < (AssemblyLine*)endOfArray(asmLines)));
       ++asml) {
    const char* errorMessage = NULL;
    
    switch (asml->type) {
      case ASM_LINE_TYPE_EMPTY:
	/** Skip empty lines */
	break;

      case ASM_LINE_TYPE_INSTRUCTION:
	writeBytecode(asml, symtab, bytecode, errorMessage);
	if (errorMessage) {
	  reportError(filename, asml->line, asml->column, lines[asml->line],
		      errorMessage);
	  free((void*)errorMessage);
	  status = -1;
	}
	break;

      case ASM_LINE_TYPE_DIRECTIVE:
	handleDirective(asml, startAddress, errorMessage);
	if (errorMessage) {
	  reportError(filename, asml->line, asml->column, lines[asml->line],
		      errorMessage);
	  free((void*)errorMessage);
	  status = -1;
	}
	break;

      case ASM_LINE_TYPE_LABEL:
      case ASM_LINE_TYPE_SYMBOL_ASSIGNMENT:
	/** Don't need to do anything for these during this assembly phase */
	break;

      default: {
	char msg[200];
	snprintf(msg, sizeof(msg), "Unknown AssemblyLine type %" PRIu16,
		 asml->type);
	reportError(filename, asml->line, asml->column, lines[asml->line],
		    msg);
	status = -1;
      }
    }
  }

  return status;
}

int assembleVmCode(const char* sourceFileame, const char* executableFilename) {
  char *text = NULL;
  size_t textLen = 0;
  const char* errorMessage = NULL;
  
  if (readSourceFile(sourceFilename, &text, &textlen, &errorMessage)) {
    fprintf(stdout, "%s\n", errorMessage);
    free((void*)errorMessage);
    return -1;
  }

  size_t numLines = 0;
  const char** lines = splitLines(text, &numLines);
  if (!lines) {
    free((void*)text);
    if (!numLines) {
      fprintf(stdout, "Source file is empty\n");
    }
    return 0;
  }

  Array asmLines = createArray(0, 0xFFFFFFFF);
  Array bytecode = createArray(0, 0xFFFFFFFF);
  SymbolTable symtab = createSymbolTable(16 * 1024 * 1024);
  uint64_t startAddress = 0;
  size_t numErrors = 0;
  int status = 0;
  
  if (parseAsmFile(sourceFilename, text, lines, numLines, asmLines, symtab
		   &numErrors)) {
    fprintf(stdout, "%d errors\nAssembly terminated\n", numErrors);
    status = -1;
  } else if (generateBytecode(sourceFilename, lines, asmLines, symtab,
			      bytecode, &startAddress)) {
    fprintf(stdout, "Assembly terminated\n");
    status = -1;
  } else if (saveVmProgramImage(executableFilename, startOfArray(bytecode),
				arraySize(byteCode), startAddress, symtab,
				&errorMessage)) {
    fprintf(stdout, "%s\n", errorMessage);
    free((void*)errorMessage);
    status = -1;
  } else {
    fprintf(stdout, "Assembly complete\n");
    status = 0;
  }

  destroySymbolTable(symtab);
  destroyArray(bytecode);
  destroyArray(asmLines);
	       
  free((void*)lines);
  free((void*)text);
  return status;
}

typedef struct CmdLineArgs_ {
  const char* sourceFilename;
  const char* executableFilename;
  int showUsage;
} CmdLineArgs;

#define CHECK_FOR_MISSING_ARG(ARG_NAME)					\
  if (getCmdLineArgParserStatus(parser) == NoMoreCmdLineArgsError) {    \
    fprintf(stderr, "ERROR: Argument missing for %s\n", ARG_NAME);      \
    destroyCmdLineArgParser(parser);                                    \
    return -1;                                                          \
  }

int parseCmdLineArgs(int argc, char** argv, CmdLineArgs* args) {
  CmdLineArgParser parser = createCmdLineArgParser(argc, argv);

  args->sourceFilename = NULL;
  args->executableFilename = NULL;
  args->showUsage = 0;

  while (hasMoreCmdLineArgs(parser)) {
    const char* argName = nextCmdLineArg(parser);
    if (!strcmp(argName, "-o")) {
      CHECK_FOR_MISSING_ARG(argName);
      args->executableFilename = nextCmdLineArg(parser);
    } else if (!args->sourceFilename) {
      args->sourceFilename = argName;
    } else {
      fprintf(stdout,
	      "ERROR: Too many command-line arguments.  Use -h for help");
      destroyCmdLineArgParser(parser);
      return -1;
    }
  }
  
  if (!showUsage) {
    if (!args->sourceFilename) {
      fprintf(stdout,
	      "ERROR: File to assemble not specified.  Use -h for help");
      destroyCmdLineArgParser(parser);
      return -1;
    }
    if (!args->executableFilename) {
      fprintf(stdout, "ERROR: Executable file not specified.  Use -h for help");
      destroyCmdLineArgParser(parser);
      return -1;
    }
  }
  destroyCmdLineArgParser(parser);
  return 0;
}

int main(int argc, char* argv) {
  CmdLineArgs args;
  int result = parseCmdLineArgs(argc, argv, &args);
  if (!result) {
    result = assembleVmCode(args->sourceFilename, args->executableFilename);
  }
  return result;
}
