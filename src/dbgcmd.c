#include "dbgcmd.h"

#include "array.h"
#include "symtab.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static const char* COMMAND_NAMES[] = {
  "", "l", "d", "w", "as", "was", "pas", "ppas", "cs", "wcs", "pcs", "ppcs",
  "b", "ba", "bd", "r", "rr", "s", "ss", "hd", "q", "h", "sym"
};
static const size_t NUM_COMMANDS = sizeof(COMMAND_NAMES) / sizeof(const char*);

static DebugCommand createDebugCommand(int32_t code) {
  DebugCommand cmd = (DebugCommand)malloc(sizeof(DebugCommandImpl));
  if (cmd) {
    cmd->cmd = code;
  }
  return cmd;
}

DebugCommand createDisassembleCommand(uint64_t address, uint32_t numLines) {
  DebugCommand cmd = createDebugCommand(DISASSEMBLE_CMD);
  if (cmd) {
    cmd->args.disassemble.address = address;
    cmd->args.disassemble.numLines = numLines;
  }
  return cmd;
}

DebugCommand createDumpBytesCommand(uint64_t address, uint32_t length) {
  DebugCommand cmd = createDebugCommand(DUMP_BYTES_CMD);
  if (cmd) {
    cmd->args.dumpBytes.address = address;
    cmd->args.dumpBytes.length = length;
  }
  return cmd;
}

DebugCommand createWriteBytesCommand(uint64_t address, uint32_t length,
				     const uint8_t* bytes) {
  // Allocate and initialize the copy of the data to write the command
  // will use.  If this fails, quit before allocating the command itself
  uint8_t* data = (uint8_t*)malloc(length);
  if (!data) {
    return NULL;  // Could not allocate a new buffer for data to write
  }
  memcpy(data, bytes, length);
  
  DebugCommand cmd = createDebugCommand(WRITE_BYTES_CMD);
  if (!cmd) {
    free((void*)data);
  } else {
    cmd->args.writeBytes.address = address;
    cmd->args.writeBytes.length = length;
    cmd->args.writeBytes.data = data;
  }
  return cmd;
}

DebugCommand createDumpAddressStackCommand(uint64_t depth, uint64_t count) {
  DebugCommand cmd = createDebugCommand(DUMP_ADDRESS_STACK_CMD);
  if (cmd) {
    cmd->args.dumpStack.depth = depth;
    cmd->args.dumpStack.count = count;
  }
  return cmd;
}

DebugCommand createModifyAddressStackCommand(uint64_t depth, uint64_t address) {
  DebugCommand cmd = createDebugCommand(MODIFY_ADDRESS_STACK_CMD);
  if (cmd) {
    cmd->args.modifyAddrStack.depth = depth;
    cmd->args.modifyAddrStack.address = address;
  }
  return cmd;
}

DebugCommand createPushAddressStackCommand(uint64_t address) {
  DebugCommand cmd = createDebugCommand(PUSH_ADDRESS_STACK_CMD);
  if (cmd) {
    cmd->args.pushAddrStack.address = address;
  }
  return cmd;
}

DebugCommand createPopAddressStackCommand() {
  return createDebugCommand(POP_ADDRESS_STACK_CMD);
}

DebugCommand createDumpCallStackCommand(uint64_t depth, uint64_t count) {
  DebugCommand cmd = createDebugCommand(DUMP_CALL_STACK_CMD);
  if (cmd) {
    cmd->args.dumpStack.depth = depth;
    cmd->args.dumpStack.count = count;
  }
  return cmd;
}

DebugCommand createModifyCallStackCommand(uint64_t depth, uint64_t blockAddress,
					  uint64_t returnAddress) {
  DebugCommand cmd = createDebugCommand(MODIFY_CALL_STACK_CMD);
  if (cmd) {
    cmd->args.modifyCallStack.depth = depth;
    cmd->args.modifyCallStack.blockAddress = blockAddress;
    cmd->args.modifyCallStack.returnAddress = returnAddress;
  }
  return cmd;
}

DebugCommand createPushCallStackCommand(uint64_t blockAddress,
					uint64_t returnAddress) {
  DebugCommand cmd = createDebugCommand(PUSH_CALL_STACK_CMD);
  if (cmd) {
    cmd->args.pushCallStack.blockAddress = blockAddress;
    cmd->args.pushCallStack.returnAddress = returnAddress;
  }
  return cmd;
}

DebugCommand createPopCallStackCommand() {
  return createDebugCommand(POP_CALL_STACK_CMD);
}

DebugCommand createListBreakpointsCommand() {
  return createDebugCommand(LIST_BREAKPOINTS_CMD);
}

DebugCommand createAddBreakpointCommand(uint64_t address) {
  DebugCommand cmd = createDebugCommand(ADD_BREAKPOINT_CMD);
  if (cmd) {
    cmd->args.addBreakpoint.address = address;
  }
  return cmd;
}

DebugCommand createRemoveBreakpointCommand(uint64_t address) {
  DebugCommand cmd = createDebugCommand(REMOVE_BREAKPOINT_CMD);
  if (cmd) {
    cmd->args.removeBreakpoint.address = address;
  }
  return cmd;
}

DebugCommand createRunCommand(uint64_t address) {
  DebugCommand cmd = createDebugCommand(RUN_PROGRAM_CMD);
  if (cmd) {
    cmd->args.run.address = address;
  }
  return cmd;
}

DebugCommand createRunUntilReturnCommand() {
  return createDebugCommand(RUN_UNTIL_RETURN_CMD);
}

DebugCommand createSingleStepIntoCommand() {
  return createDebugCommand(SINGLE_STEP_INTO_CMD);
}

DebugCommand createSingleStepOverCommand() {
  return createDebugCommand(SINGLE_STEP_OVER_CMD);
}

DebugCommand createHeapDumpCommand(const char* filename) {
  const char* dumpFilename = NULL;
  
  if (filename) {
    dumpFilename = strdup(filename);
    if (!dumpFilename) {
      return NULL;
    }
  }
  
  DebugCommand cmd = createDebugCommand(HEAP_DUMP_CMD);
  if (!cmd) {
    free((void*)dumpFilename);  // OK to free NULL
  } else {
    cmd->args.heapDump.filename = dumpFilename;
  }
  return cmd;
}

DebugCommand createQuitVMCommand() {
  return createDebugCommand(QUIT_VM_CMD);
}

DebugCommand createShowHelpCommand() {
  return createDebugCommand(SHOW_HELP_CMD);
}

DebugCommand createLookupSymbolCommand(const char* name) {
  DebugCommand cmd = createDebugCommand(LOOKUP_SYMBOL_CMD);
  if (cmd) {
    cmd->args.lookupSymbol.name = strdup(name);
  }
  return cmd;
}

DebugCommand createCommandParseError(int32_t code, const char* details) {
  const char* errDetails = strdup(details);
  if (!errDetails) {
    return NULL;
  }
  
  DebugCommand cmd = createDebugCommand(DEBUG_CMD_PARSE_ERROR);
  if (!cmd) {
    free((void*)errDetails);
    return NULL;
  } else {
    cmd->args.errorDetails.code = code;
    cmd->args.errorDetails.details = errDetails;
    return cmd;
  }
}

void destroyDebugCommand(DebugCommand cmd) {
  if (cmd) {
    switch (cmd->cmd) {
      case WRITE_BYTES_CMD:
	free((void*)cmd->args.writeBytes.data);
	break;
      
      case HEAP_DUMP_CMD:
	free((void*)cmd->args.heapDump.filename);
	break;

      case LOOKUP_SYMBOL_CMD:
	free((void*)cmd->args.lookupSymbol.name);
	break;
      
      case DEBUG_CMD_PARSE_ERROR:
	free((void*)cmd->args.errorDetails.details);
	break;
    }

    free((void*)cmd);
  }
}

void printDebugCommand(DebugCommand cmd, FILE* out) {
  size_t msgLength = 0;

  // Allocate a buffer big enough to hold the result without truncation
  if (cmd->cmd == WRITE_BYTES_CMD) {
    msgLength = 35 + 4 * cmd->args.writeBytes.length;
  } else if (cmd->cmd == HEAP_DUMP_CMD) {
    msgLength =
      2 + (cmd->args.heapDump.filename
             ? strlen(cmd->args.heapDump.filename) + 1
	     : 0);
  } else if (cmd->cmd == DEBUG_CMD_PARSE_ERROR) {
    msgLength = 25 + strlen(cmd->args.errorDetails.details);
  } else if (cmd->cmd == LOOKUP_SYMBOL_CMD) {
    if (cmd->args.lookupSymbol.name) {
      msgLength = 4 + strlen(cmd->args.lookupSymbol.name);
    } else {
      msgLength = 16;
    }
  } else {
    msgLength = 71;
  }

  char* msg = malloc(msgLength + 1);
  if (!msg) {
    fprintf(out, "FAILED TO ALLOCATE BUFFER of size %zu IN printDebugCommand()",
	    msgLength + 1);
  } else {
    sprintDebugCommand(cmd, msg, msgLength);
    fprintf(out, "%s", msg);
    free((void*)msg);
  }
}

int sprintDebugCommand(DebugCommand cmd, char* buffer, size_t bufferSize) {
  switch(cmd->cmd) {
    case DISASSEMBLE_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64 " %" PRIu32,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.disassemble.address,
		      cmd->args.disassemble.numLines);
      
    case DUMP_BYTES_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64 " %" PRIu32,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.dumpBytes.address,
		      cmd->args.dumpBytes.length);

    case DUMP_ADDRESS_STACK_CMD:
    case DUMP_CALL_STACK_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64 " %" PRIu64,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.dumpStack.depth,
		      cmd->args.dumpStack.count);

    case MODIFY_ADDRESS_STACK_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64 " %" PRIu64,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.modifyAddrStack.depth,
		      cmd->args.modifyAddrStack.address);

    case PUSH_ADDRESS_STACK_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.pushAddrStack.address);

    case POP_ADDRESS_STACK_CMD:
    case POP_CALL_STACK_CMD:
    case LIST_BREAKPOINTS_CMD:
    case RUN_UNTIL_RETURN_CMD:
    case SINGLE_STEP_INTO_CMD:
    case SINGLE_STEP_OVER_CMD:
    case QUIT_VM_CMD:
    case SHOW_HELP_CMD:
      return snprintf(buffer, bufferSize, "%s", COMMAND_NAMES[cmd->cmd]);
      
    case MODIFY_CALL_STACK_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64 " %" PRIu64 " %" PRIu64,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.modifyCallStack.depth,
		      cmd->args.modifyCallStack.blockAddress,
		      cmd->args.modifyCallStack.returnAddress);

    case PUSH_CALL_STACK_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64 " %" PRIu64,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.pushCallStack.blockAddress,
		      cmd->args.pushCallStack.returnAddress);


    case ADD_BREAKPOINT_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64,
		      COMMAND_NAMES[cmd->cmd], cmd->args.addBreakpoint.address);
      
    case REMOVE_BREAKPOINT_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64,
		      COMMAND_NAMES[cmd->cmd],
		      cmd->args.removeBreakpoint.address);
      
    case RUN_PROGRAM_CMD:
      return snprintf(buffer, bufferSize, "%s %" PRIu64,
		      COMMAND_NAMES[cmd->cmd], cmd->args.run.address);
      
    case HEAP_DUMP_CMD:
      if (cmd->args.heapDump.filename) {
	return snprintf(buffer, bufferSize, "%s %s",
			COMMAND_NAMES[cmd->cmd], cmd->args.heapDump.filename);
      } else {
	return snprintf(buffer, bufferSize, "%s", COMMAND_NAMES[cmd->cmd]);
      }

    case LOOKUP_SYMBOL_CMD:
      if (cmd->args.lookupSymbol.name) {
	return snprintf(buffer, bufferSize, "%s %s",
			COMMAND_NAMES[cmd->cmd], cmd->args.lookupSymbol.name);
      } else {
	return snprintf(buffer, bufferSize, "%s **MISSING**",
			COMMAND_NAMES[cmd->cmd]);
      }
      
    case DEBUG_CMD_PARSE_ERROR:
      return snprintf(buffer, bufferSize, "PARSE_ERROR %" PRId32 "/%s",
		      cmd->args.errorDetails.code,
		      cmd->args.errorDetails.details);

    default:
      return snprintf(buffer, bufferSize,
		      "COMMAND WITH UNKNOWN CODE %" PRId32, cmd->cmd);
  }
}

typedef struct ParserState_ {
  UnlambdaVM vm;               /** VM state for defaults */
  const char* text;            /** Text being parsed */
  const char* p;               /** Current position in text */
  int32_t cmdCode;             /** Code for command (0 = not set) */
  const char* cmdText;         /** Text of command */
  int32_t errorCode;           /** Error code (0 = no error) */
  const char* errorDetails;    /** Error details */
} ParserState;


static void initParserState(ParserState* state, UnlambdaVM vm,
			    const char* text);
static void cleanParserState(ParserState* state);
static int skipBlanks(ParserState* state);
static int32_t  parseCommandText(ParserState* state);
static uint8_t  parseNextByte(ParserState* state);
static uint32_t parseNextUInt32(ParserState* state);
static uint32_t parseNextUInt32WithDefault(ParserState* state, uint32_t dv);
static uint64_t parseNextUInt64(ParserState* state);
static uint64_t parseNextUInt64WithDefault(ParserState* state, uint64_t dv);
static int checkNoMoreArguments(ParserState* state);
static DebugCommand makeParseErrorFromState(ParserState* state);
static void setParseError(ParserState* state, int32_t code, const char* msg);
static void clearParseError(ParserState* state);

typedef DebugCommand (*CommandParserFn)(ParserState*);

static DebugCommand parseDisassembleCmd(ParserState* state);
static DebugCommand parseDumpBytesCmd(ParserState* state);
static DebugCommand parseWriteBytesCmd(ParserState* state);
static DebugCommand parseDumpAddrStackCmd(ParserState* state);
static DebugCommand parseModifyAddrStackCmd(ParserState* state);
static DebugCommand parsePushAddreessCmd(ParserState* state);
static DebugCommand parsePopAddressCmd(ParserState* state);
static DebugCommand parseDumpCallStackCmd(ParserState* state);
static DebugCommand parseModifyCallStackCmd(ParserState* state);
static DebugCommand parsePushCallFrameCmd(ParserState* state);
static DebugCommand parsePopCallFrameCmd(ParserState* state);
static DebugCommand parseListBreakpointsCmd(ParserState* state);
static DebugCommand parseAddBreakpointCmd(ParserState* state);
static DebugCommand parseRemoveBreakpointCmd(ParserState* state);
static DebugCommand parseRunProgramCmd(ParserState* state);
static DebugCommand parseRunUntilReturnCmd(ParserState* state);
static DebugCommand parseSingleStepIntoCmd(ParserState* state);
static DebugCommand parseSingleStepOverCmd(ParserState* state);
static DebugCommand parseHeapDumpCmd(ParserState* state);
static DebugCommand parseQuitVmCmd(ParserState* state);
static DebugCommand parseShowHelpCmd(ParserState* state);
static DebugCommand parseLookupSymbolCmd(ParserState* state);

static CommandParserFn CommandParsers[] = {
  NULL,
  parseDisassembleCmd,
  parseDumpBytesCmd,
  parseWriteBytesCmd,
  parseDumpAddrStackCmd,
  parseModifyAddrStackCmd,
  parsePushAddreessCmd,
  parsePopAddressCmd,
  parseDumpCallStackCmd,
  parseModifyCallStackCmd,
  parsePushCallFrameCmd,
  parsePopCallFrameCmd,
  parseListBreakpointsCmd,
  parseAddBreakpointCmd,
  parseRemoveBreakpointCmd,
  parseRunProgramCmd,
  parseRunUntilReturnCmd,
  parseSingleStepIntoCmd,
  parseSingleStepOverCmd,
  parseHeapDumpCmd,
  parseQuitVmCmd,
  parseShowHelpCmd,
  parseLookupSymbolCmd,
};

DebugCommand parseDebugCommand(UnlambdaVM context, const char* text) {
  ParserState state;
  initParserState(&state, context, text);

  int32_t cmdCode = parseCommandText(&state);
  if (!cmdCode) {
    cleanParserState(&state);
    return NULL; // Line is blank
  } else if (cmdCode < 0) {
    // makeParseErrorFromState() cleans state before returning
    return makeParseErrorFromState(&state);
  } else {
    assert(cmdCode > 0);
    assert(cmdCode < (sizeof(CommandParsers) / sizeof(CommandParserFn)));

    DebugCommand cmd = CommandParsers[cmdCode](&state);
    assert(cmd);

    cleanParserState(&state);
    return cmd;
  }
}

static void initParserState(ParserState* state, UnlambdaVM vm,
			    const char* text) {
  state->vm = vm;
  state->text = text;
  state->p = text;
  state->cmdCode = 0;
  state->cmdText = NULL;
  state->errorCode = 0;
  state->errorDetails = NULL;
}

static void cleanParserState(ParserState* state) {
  if (state->cmdText) {
    free((void*)state->cmdText);
    state->cmdText = NULL;
  }
  if (state->errorDetails) {
    free((void*)state->errorDetails);
    state->errorDetails = NULL;
  }
}

static int skipBlanks(ParserState* state) {
  while (*(state->p) && ((*(state->p) == ' ') || (*(state->p) == '\t'))) {
    ++(state->p);
  }
  return *(state->p);
}

static int32_t parseCommandText(ParserState* state) {
  // Should only be called if a command has not been recognized
  assert(!state->cmdCode);
  assert(!state->cmdText);
  
  if (!skipBlanks(state)) {
    return 0;  // No command
  }

  // Find end of the command
  const char* start = state->p;
  while (*(state->p) && isalnum(*(state->p))) {
    ++(state->p);
  }

  if (state->p == start) {
    // state->p points to some non-alnum char, which is a syntax error
    setParseError(state, DEBUG_CMD_PARSE_SYNTAX_ERROR, "Syntax error");
    return -1;
  }

  state->cmdText = malloc((state->p - start) + 1);
  if (!state->cmdText) {
    setParseError(state, DEBUG_CMD_OUT_OF_MEMORY_ERROR,
		  "Could not allocate memory for command text");
    return -1;
  }
  memcpy((char*)state->cmdText, start, state->p - start);
  ((char*)(state->cmdText))[state->p - start] = 0;
  
  // Linear search for command is good enough
  for (int i = 1; i < NUM_COMMANDS; ++i) {
    if (!strcmp(state->cmdText, COMMAND_NAMES[i])) {
      // Found it
      state->cmdCode = i;

      // Make sure text after command is blank or end of string
      if (*(state->p) && (*(state->p) != ' ') && (*(state->p) != '\t')
	    && (*(state->p) != '\n')) {
	setParseError(state, DEBUG_CMD_PARSE_SYNTAX_ERROR, "Syntax error");
	return -1;
      }
      return state->cmdCode;
    }
  }

  // Command not found
  char msg[200];
  snprintf(msg, sizeof(msg), "Unknown command \"%s\".  Use h to print a "
	   "list of commands", state->cmdText);
  setParseError(state, DEBUG_CMD_UNKNOWN_CMD_ERROR, msg);
  state->cmdCode = -1;
  return -1;
}

static uint8_t parseNextByte(ParserState* state) {
  uint64_t result = parseNextUInt64(state);
  if (result > 255) {
    setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		  "Value is too large");
    return 0;
  }
  return result;
}

static uint32_t parseNextUInt32(ParserState* state) {
  uint64_t result = parseNextUInt64(state);
  if (result > (uint64_t)0xFFFFFFFF) {
    setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		  "Value is too large");
    return 0;
  }
  return result;
}

static uint32_t parseNextUInt32WithDefault(ParserState* state, uint32_t dv) {
  uint64_t result = parseNextUInt64WithDefault(state, dv);
  if (result > (uint64_t)0xFFFFFFFF) {
    setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		  "Value is too large");
    return 0;
  }
  return result;
}

static uint64_t MAXU64_DIV_10 = (uint64_t)0xFFFFFFFFFFFFFFFF / 10;
static uint64_t MAXU64_MOD_10 = (uint64_t)0xFFFFFFFFFFFFFFFF % 10;

static uint64_t parseNextUInt64(ParserState* state) {
  uint64_t result = 0;
  int c = skipBlanks(state);

  if (!c) {
    setParseError(state, DEBUG_CMD_PARSE_MISSING_ARG_ERROR, "Argument missing");
    return 0;
  }

  while (c && isdigit(c)) {
    if ((result > MAXU64_DIV_10)
	  || ((result == MAXU64_DIV_10) && (c > MAXU64_MOD_10))) {
      // Overflow
      setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		    "Value is too large");
      return 0;
    }

    result = result * 10 + (c - '0');
    c = *(++(state->p));
  }

  if (c && (c != ' ') && (c != '\t') && (c != '\n')) {
    // Value is not a number
    setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		  "Value is not a number");
    return 0;
  }

  return result;
}

static uint64_t parseNextUInt64WithDefault(ParserState* state, uint64_t dv) {
  int c = skipBlanks(state);
  return c ? parseNextUInt64(state) : dv;
}

static uint64_t parseNextAddress(ParserState* state) {
  int c = skipBlanks(state);
  if (!c) {
    // Missing value
    setParseError(state, DEBUG_CMD_PARSE_MISSING_ARG_ERROR, "Argument missing");
    return 0;
  } else if (isdigit(c)) {
    // Numeric address
    return parseNextUInt64(state);
  } else if (!isalpha(c) && (c != '_')) {
    // Syntax error
    setParseError(state, DEBUG_CMD_PARSE_SYNTAX_ERROR, "Syntax error");
    return 0;
  } else {
    // Symbol with optional displacement
    const char* start = state->p;
    while (*(state->p) && (isalnum(*(state->p)) || (*(state->p) == '_'))) {
      ++(state->p);
    }
    assert(state->p > start);

    // TODO: Handle allocation failure for symbol name
    char* name = (char*)malloc(state->p - start + 1);
    memcpy(name, start, state->p - start);
    name[state->p - start] = 0;

    const Symbol* sym = findSymbol(getVmSymbolTable(state->vm), name);
    if (!sym) {
      char msg[200];
      snprintf(msg, sizeof(msg), "Unknown symbol \"%s\"", name);
      setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR, msg);
      free((void*)name);
      return 0;
    }

    free((void*)name);
    uint64_t address = sym->address;

    if (*(state->p) == '+') {
      ++(state->p);

      uint64_t disp = parseNextUInt64(state);
      if (state->errorCode == DEBUG_CMD_PARSE_MISSING_ARG_ERROR) {
	setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		      "Missing displacement value");
	return 0;
      } else if (state->errorCode) {
	char msg[200];
	snprintf(msg, sizeof(msg), "Invalid displacement (%s)",
		 state->errorDetails);
	setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR, msg);
	return 0;
      }

      address += disp;
    } else if (*(state->p) == '-') {
      ++(state->p);

      uint64_t disp = parseNextUInt64(state);
      if (state->errorCode == DEBUG_CMD_PARSE_MISSING_ARG_ERROR) {
	setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		      "Missing displacement value");
	return 0;
      } else if (state->errorCode) {
	char msg[200];
	snprintf(msg, sizeof(msg), "Invalid displacement (%s)",
		 state->errorDetails);
	setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR, msg);
	return 0;
      }

      address -= disp;
    } else if (*(state->p) && (*(state->p) != ' ') && (*(state->p) != '\t')
	       && (*(state->p) != '\n')) {
      setParseError(state, DEBUG_CMD_PARSE_SYNTAX_ERROR, "Syntax error");
      return 0;
    }

    return address;
  }
}

static uint64_t parseNextAddressWithDefault(ParserState* state, uint64_t dv) {
  int c = skipBlanks(state);
  return c ? parseNextAddress(state) : dv;
}

static int checkNoMoreArguments(ParserState* state) {
  int next = skipBlanks(state);

  while (next == '\n') {
    next = *(++(state->p));
  }
  
  if (next) {
    setParseError(state, DEBUG_CMD_PARSE_SYNTAX_ERROR, "Too many arguments");
    return -1;
  }
  return 0;
}

static DebugCommand makeParseErrorFromState(ParserState* state) {
  assert(state->errorCode < 0);
  assert(state->errorDetails);

  DebugCommand cmd = createCommandParseError(state->errorCode,
					     state->errorDetails);
  cleanParserState(state);
  return cmd;
}

static void setParseError(ParserState* state, int32_t code, const char* msg) {
  if (state->errorDetails) {
    free((void*)state->errorDetails);
  }

  assert(msg);
  state->errorCode = code;

  // TODO: Handle the case where strdup() fails
  state->errorDetails = strdup(msg);
}

static void clearParseError(ParserState* state) {
  if (state->errorDetails) {
    free((void*)state->errorDetails);
    state->errorDetails = NULL;
  }
  state->errorCode = 0;
}

static int checkArgumentParsed(ParserState* state, const char* argName) {
  if (!state->errorCode) {
    return 0;
  } else {
    char msg[200];
    int32_t errorCode = state->errorCode;

    switch(state->errorCode) {
      case DEBUG_CMD_PARSE_SYNTAX_ERROR:
      case DEBUG_CMD_PARSE_INVALID_ARG_ERROR:
	state->errorCode = DEBUG_CMD_PARSE_INVALID_ARG_ERROR;
	snprintf(msg, sizeof(msg), "Invalid %s (%s)", argName,
		 state->errorDetails);
	break;

      case DEBUG_CMD_PARSE_MISSING_ARG_ERROR:
	snprintf(msg, sizeof(msg), "Required argument \"%s\" is missing",
		 argName);
	break;

      case DEBUG_CMD_UNKNOWN_CMD_ERROR:
	snprintf(msg, sizeof(msg), "Received an \"Unknown command\" error"
		 "parsing the \"%s\", argument.  How did this happen???",
		 argName);
	break;

      case DEBUG_CMD_OUT_OF_MEMORY_ERROR:
	snprintf(msg, sizeof(msg), "Ran out of memory parsing \"%s\" (%s)",
		 argName, state->errorDetails);
	break;

      default:
	snprintf(msg, sizeof(msg), "Unknown error condition %d while parsing "
		 "the \"%s\" argument (%s)", state->errorCode, argName,
		 state->errorDetails ? state->errorDetails : "");
	break;
    }

    setParseError(state, errorCode, msg);
    return -1;
  }
}

static DebugCommand parseDisassembleCmd(ParserState* state) {
  static const uint32_t DEFAULT_NUM_LINES = 10;

  uint64_t address = parseNextAddressWithDefault(state, getVmPC(state->vm));
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  uint32_t numLines = parseNextUInt32WithDefault(state, DEFAULT_NUM_LINES);
  if (checkArgumentParsed(state, "number of lines")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createDisassembleCommand(address, numLines);
}

static DebugCommand parseDumpBytesCmd(ParserState* state) {
  static const uint32_t DEFAULT_NUM_BYTES = 256;
  uint64_t address = parseNextAddress(state);
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  uint32_t numBytes = parseNextUInt32WithDefault(state, DEFAULT_NUM_BYTES);
  if (checkArgumentParsed(state, "number of bytes")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createDumpBytesCommand(address, numBytes);
}

static DebugCommand parseWriteBytesCmd(ParserState* state) {
  static const uint32_t MAX_DATA_LENGTH = 65536;
  uint64_t address = parseNextAddress(state);
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  Array data = createArray(0, MAX_DATA_LENGTH);
  uint8_t v = parseNextByte(state);
  while (!state->errorCode) {
    if (appendToArray(data, &v, 1)) {
      if (getArrayStatus(data) == ArraySequenceTooLongError) {
	setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		      "Too many values to write");
      } else {
	setParseError(state, DEBUG_CMD_OUT_OF_MEMORY_ERROR, "Out of memory");
      }
      destroyArray(data);
      return makeParseErrorFromState(state);
    }
    v = parseNextByte(state);
  }

  if (state->errorCode != DEBUG_CMD_PARSE_MISSING_ARG_ERROR) {
    checkArgumentParsed(state, "bytes to write");
    return makeParseErrorFromState(state);
  }

  if (!arraySize(data)) {
    setParseError(state, DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
		  "Bytes to write are missing");
    return makeParseErrorFromState(state);
  }
  
  clearParseError(state);
  DebugCommand cmd = createWriteBytesCommand(address, arraySize(data),
					     startOfArray(data));
  destroyArray(data);
  return cmd;
}

static DebugCommand parseDumpAddrStackCmd(ParserState* state) {
  static uint64_t DEFAULT_NUM_FRAMES = 16;
  uint64_t depth = parseNextUInt64WithDefault(state, 0);
  if (checkArgumentParsed(state, "depth")) {
    return makeParseErrorFromState(state);
  }
  
  uint64_t count = parseNextUInt64WithDefault(state, DEFAULT_NUM_FRAMES);
  if (checkArgumentParsed(state, "frame count")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createDumpAddressStackCommand(depth, count);
}

static DebugCommand parseModifyAddrStackCmd(ParserState* state) {
  uint64_t depth = parseNextUInt64(state);
  if (checkArgumentParsed(state, "depth")) {
    return makeParseErrorFromState(state);
  }

  uint64_t value = parseNextAddress(state);
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createModifyAddressStackCommand(depth, value);
}

static DebugCommand parsePushAddreessCmd(ParserState* state) {
  uint64_t address = parseNextAddress(state);
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createPushAddressStackCommand(address);
}

static DebugCommand parsePopAddressCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createPopAddressStackCommand();
}

static DebugCommand parseDumpCallStackCmd(ParserState* state) {
  static uint64_t DEFAULT_NUM_FRAMES = 16;
  uint64_t depth = parseNextUInt64WithDefault(state, 0);
  if (checkArgumentParsed(state, "depth")) {
    return makeParseErrorFromState(state);
  }
  
  uint64_t count = parseNextUInt64WithDefault(state, DEFAULT_NUM_FRAMES);
  if (checkArgumentParsed(state, "frame count")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createDumpCallStackCommand(depth, count);
}

static DebugCommand parseModifyCallStackCmd(ParserState* state) {
  uint64_t depth = parseNextUInt64(state);
  if (checkArgumentParsed(state, "depth")) {
    return makeParseErrorFromState(state);
  }

  uint64_t blockAddress = parseNextAddress(state);
  if (checkArgumentParsed(state, "block address")) {
    return makeParseErrorFromState(state);
  }

  uint64_t returnAddress = parseNextAddress(state);
  if (checkArgumentParsed(state, "return address")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createModifyCallStackCommand(depth, blockAddress, returnAddress);
}

static DebugCommand parsePushCallFrameCmd(ParserState* state) {
  uint64_t blockAddress = parseNextAddress(state);
  if (checkArgumentParsed(state, "block address")) {
    return makeParseErrorFromState(state);
  }

  uint64_t returnAddress = parseNextAddress(state);
  if (checkArgumentParsed(state, "return address")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createPushCallStackCommand(blockAddress, returnAddress);
}

static DebugCommand parsePopCallFrameCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createPopCallStackCommand();
}

static DebugCommand parseListBreakpointsCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createListBreakpointsCommand();
}

static DebugCommand parseAddBreakpointCmd(ParserState* state) {
  uint64_t address = parseNextAddressWithDefault(state, getVmPC(state->vm));
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createAddBreakpointCommand(address);
}

static DebugCommand parseRemoveBreakpointCmd(ParserState* state) {
  uint64_t address = parseNextAddressWithDefault(state, getVmPC(state->vm));
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createRemoveBreakpointCommand(address);
}

static DebugCommand parseRunProgramCmd(ParserState* state) {
  uint64_t address = parseNextAddressWithDefault(state, getVmPC(state->vm));
  if (checkArgumentParsed(state, "address")) {
    return makeParseErrorFromState(state);
  }

  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createRunCommand(address);
}

static DebugCommand parseRunUntilReturnCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createRunUntilReturnCommand();
}

static DebugCommand parseSingleStepIntoCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createSingleStepIntoCommand();
}

static DebugCommand parseSingleStepOverCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createSingleStepOverCommand();
}

static DebugCommand parseHeapDumpCmd(ParserState* state) {
  int c = skipBlanks(state);
  if (!c) {
    return createHeapDumpCommand(NULL);
  } else {
    const char* start = state->p;
    while (*(state->p) && (*(state->p) != ' ') && (*(state->p) != '\t')) {
      ++(state->p);
    }

    const char* end = state->p;
    
    if (checkNoMoreArguments(state)) {
      return makeParseErrorFromState(state);
    }
	
    assert(end > start);
    char* filename = (char*)malloc(end - start + 1);
    strncpy(filename, start, end - start);
    filename[end - start] = 0;

    DebugCommand cmd = createHeapDumpCommand(filename);
    free((void*)filename);
    return cmd;
  }
}

static DebugCommand parseQuitVmCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createQuitVMCommand();
}

static DebugCommand parseShowHelpCmd(ParserState* state) {
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }

  return createShowHelpCommand();
}

static DebugCommand parseLookupSymbolCmd(ParserState* state) {
  int c = skipBlanks(state);

  if (!c) {
    setParseError(state, DEBUG_CMD_PARSE_MISSING_ARG_ERROR,
		  "Symbol name missing");
    return makeParseErrorFromState(state);
  } else if (!isalpha(c) && (c != '_')) {
    setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		  "Invalid symbol name");
    return makeParseErrorFromState(state);
  }

  const char* start = state->p;
  while (*(state->p) && (isalnum(*(state->p)) || (*(state->p) == '_'))) {
    ++(state->p);
  }

  assert(state->p > start);

  if (*(state->p) && (*(state->p) != ' ') && (*(state->p) != '\t')
        && (*(state->p) != '\n')) {
    setParseError(state, DEBUG_CMD_PARSE_INVALID_ARG_ERROR,
		  "Invalid symbol name");
    return makeParseErrorFromState(state);
  }
  
  if (checkNoMoreArguments(state)) {
    return makeParseErrorFromState(state);
  }
  
  char* name = (char*)malloc(state->p - start + 1);
  memcpy(name, start, state->p - start);
  name[state->p - start] = 0;

  DebugCommand cmd = createLookupSymbolCommand(name);
  free((void*)name);
  return cmd;
}
