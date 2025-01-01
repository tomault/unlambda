#ifndef __UNLAMBDA__DBGCMD_H__
#define __UNLAMBDA__DBGCMD_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct DissassembleArgs_ {
  uint64_t address;
  uint32_t numLines;
} DissasembleArgs;

typedef struct DumpBytesArgs_ {
  uint64_t address;
  uint32_t length;
} DumpBytesArgs;

typedef struct WriteBytesArgs_ {
  uint64_t address;
  uint32_t length;
  uint8_t* data;
};

typedef struct DumpStackArgs_ {
  uint64_t depth;
  uint64_t count;
} DumpStackArgs;

typedef struct ModifyAddrStackArgs_ {
  uint64_t depth;
  uint64_t address;
} ModifyAddrStackArgs;

typedef struct PushAddrStackArgs_ {
  uint64_t address;
} PushAddrStackArgs;

typedef struct ModifyCallStackArgs_ {
  uint64_t depth;
  uint64_t blockAddress;
  uint64_t returnAddress;
} ModifyCallStackArgs;

typedef struct PushCallStackArgs_ {
  uint64_t blockAddress;
  uint64_t returnAddress;
} PushCallStackArgs;

typedef struct AddBreakpointArgs_ {
  uint64_t address;
} AddBreakpointArgs;

typedef struct RemoveBreakpointArgs_ {
  uint64_t address;
} RemoveBreakpointArgs;

typedef struct RunArgs_ {
  uint64_t address;
} RunArgs;

typedef struct HeapDumpArgs_ {
  const char* filename;
} HeapDumpArgs;

typedef struct LookupSymbolArgs_ {
  const char* name;
} LookupSymbolArgs;

typedef struct ErrorDetails_ {
  int32_t code;
  const char* details;
} ErrorDetails;

typedef union DebugCommandArgs_ {
  DisassembleArgs disassemble;
  DumpBytesArgs dumpBytes;
  WriteBytesArgs writeBytes;
  DumpStackArgs dumpStack;
  ModifyAddrStackArgs modifyAddrStack;
  PushAddrStackArgs pushAddrStack;
  ModifyCallStackArgs modifyCallStack;
  PushCallStackArgs pushCallStack;
  AddBreakpointArgs addBreakpoint;
  RemoveBreakpointArgs removeBreakpoint;
  RunArgs run;
  HeapDumpArgs heapDump;
  LookupSymbolArgs lookupSymbol;
  ErrorDetails errorDetails;
} CommandArgs;

typedef struct DebugCommandImpl_ {
  int32_t cmd;
  DebugCommandArgs args;
} DebugCommandImpl;

typedef DebugCommandImpl* DebugCommand;

DebugCommand createDisssassembleCommand(uint64_t address, uint32_t numLines);
DebugCommand createDumpBytesCommand(uint64_t address, uint32_t length);
DebugCommand createWriteBytesCommand(uint64_t address, uint32_t length,
				     uint8_t* bytes);
DebugCommand createDumpAddressStackCommand(uint64_t depth, uint64_t count);
DebugCommand createModifyAddressStackCommand(uint64_t depth, uint64_t address);
DebugCommand createPushAddressStackCommand(uint64_t address);
DebugCommand createPopAddressStackCommand();
DebugCommand createDumpCallStackCommand(uint64_t depth, uint64_t count);
DebugCommand createModifyCallStackCommand(uint64_t depth, uint64_t blockAddress,
					  uint64_t returnAddress);
DebugCommand createPushCallStackCommand(uint64_t blockAddress,
					uint64_t returnAddress);
DebugCommand createPopCallStackCommand();
DebugCommand createListBreakpointsCommand();
DebugCommand createAddBreakpointCommand(uint64_t address);
DebugCommand createRemoveBreakpointCommand(uint64_t address);
DebugCommand createRunCommand(uint64_t address);
DebugCommand createRunUntilReturnCommand();
DebugCommand createSingleStepIntoCommand();
DebugCommand createSingleStepOverCommand();
DebugCommand createHeapDumpCommand(const char* filename);
DebugCommand createQuitVMCommand();
DebugCommand createShowHelpCommand();
DebugCommand createLookupSymbolCommand(const char* name);
DebugCommand createCommandParseError(int32_t code, const char* details);

void destroyDebugCommand(DebugCommand cmd);

DebugCommand parseDebugCommand(UnlambdaVM context, const char* text);

void printDebugCommand(DebugCommand cmd, FILE* out);
int sprintDebugCommand(DebugCommand cmd, char* buffer, size_t bufferSize);

/** Disassemble code starting at a given address */
#define DISASSEMBLE_CMD            1

/** Dump bytes in the VM's memory */
#define DUMP_BYTES_CMD             2

/** Change bytes in the VM's memory */
#define WRITE_BYTES_CMD            3

/** Dump frames on the address stack */
#define DUMP_ADDRESS_STACK_CMD     4

/** Change the value of a frame on the address stack */
#define MODIFY_ADDRESS_STACK_CMD   5

/** Push an address onto the address stack */
#define PUSH_ADDRESS_STACK_CMD     6

/** Pop an address from the address stack */
#define POP_ADDRESS_STACK_CMD      7

/** Dump frames on the call stack */
#define DUMP_CALL_STACK_CMD        8

/** Modify a frame on the call stack */
#define MODIFY_CALL_STACK_CMD      9

/** Push a frame onto the call stack */
#define PUSH_CALL_STACK_CMD       10

/** Pop a frame from the call stack */
#define POP_CALL_STACK_CMD        11

/** List persistent breakpoints */
#define LIST_BREAKPOINTS_CMD      12

/** Add a new breakpoint */
#define ADD_BREAKPOINT_CMD        13

/** Remove a breakpoint */
#define REMOVE_BREAKPOINT_CMD     14

/** Resume execution at a given address */
#define RUN_PROGRAM_CMD           15

/** Resume execution until after the next return statement */
#define RUN_UNTIL_RETURN_CMD      16

/** Execute one instruction, stepping into procedure calls */
#define SINGLE_STEP_INTO_CMD      17

/** Execute one instruction, stepping over procedure calls */
#define SINGLE_STEP_OVER_CMD      18

/** Dump the heap to standard output or a file */
#define HEAP_DUMP_CMD             19

/** Quit the VM */
#define QUIT_VM_CMD               20

/** Show help message */
#define SHOW_HELP_CMD             21

/** Lookup the address of a symbol */
#define LOOKUP_SYMBOL_CMD         22

/** An error occurred while parsing the command.  See args.errorDetails
 *    for more info
 */
#define DEBUG_CMD_PARSE_ERROR     -1

/** Values for ErrorDetails.code */

#define DEBUG_CMD_PARSE_SYNTAX_ERROR      -1
#define DEBUG_CMD_PARSE_INVALID_ARG_ERROR -2
#define DEBUG_CMD_PARSE_MISSING_ARG_ERROR -3
#define DEBUG_CMD_UNKNOWN_CMD_ERROR       -4
#define DEBUG_CMD_OUT_OF_MEMORY_ERROR     -5

#endif

