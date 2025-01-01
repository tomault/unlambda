#include "brkpt.h"
#include "debug.h"
#include "stack.h"
#include "vm.h"
#include "vm_instructions.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DebuggerImpl_ {
  UnlambdaVM vm;
  BreakpointList persistentBreakpoints;
  BreakpointList temporaryBreakpoints;
  int breakOnNext;

  int statusCode;
  const char* statusMsg;
} DebuggerImpl;

const char OK_MSG[] = "OK";
const char DEFAULT_ERR_MSG[] = "ERROR";

const uint32_t MAX_TEMP_BREAKPOINTS = 32;

Debugger createDebugger(UnlambdaVM vm, uint32_t maxBreakpoints) {
  Debugger dbg = (Debugger)malloc(sizeof(DebuggerImpl));
  if (!dbg) {
    return NULL;
  }

  dbg->vm = vm;
  dbg->persistentBreakpoints = createBreakpointList(maxBreakpoints);
  if (!dbg->persistentBreakpoints) {
    free((void*)dbg);
    return NULL;
  }
  dbg->temporaryBreakpoints = createBreakpointList(MAX_TEMP_BREAKPOINTS);
  if (!dbg->temporaryBreakpoints) {
    destroyBreakpointList(dbg->persistentBreakpoints);
    free((void*)dbg);
    return NULL;
  }
  dbg->breakOnNext = 0;
  dbg->statusCode = 0;
  dbg->statusMsg = OK_MSG;

  return dbg;
}

void destroyDebugger(Debugger dbg) {
  clearDebuggerStatus(dbg);
  destroyBreakpointList(dbg->temporaryBreakpoints);
  destroyBreakpointList(dbg->persistentBreakpoints);
  free((void*)dbg);
}

int getDebuggerStatus(Debugger dbg) {
  return dbg->statusCode;
}

const char* getDebuggerStatusMsg(Debugger dbg) {
  return dbg->statusMsg;
}

static int shouldDeallocateStatusMsg(Debugger dbg) {
  return dbg->statusMsg && (dbg-statusMsg != OK_MSG)
           && (dbg->statusMsg != DEFAULT_ERR_MSG);
}

void clearDebuggerStatus(Debugger dbg) {
  if (shouldDeallocateStatusMsg(dbg)) {
    free((void*)dbg->statusMsg);
  }
  dbg->statusCode = 0;
  dbg->statusMsg = OK_MSG;
}

static void setDebuggerStatus(Debugger dbg, int code, const char* msg) {
  if (shouldDeallocateStatusMsg(dbg)) {
    free((void*)dbg->statusMsg);
  }

  dbg->statusCode = code;
  if ((msg == OK_MSG) || (msg == DEFAULT_ERR_MSG)) {
    dbg->statusMsg = msg;
  } else {
    dbg->statusMsg = malloc(strlen(msg) + 1);
    if (dbg->statusMsg) {
      strcpy(dbg->statusMsg, msg);
    } else {
      dbg->statusMsg = DEFAULT_ERR_MSG;
    }
  }
}

int shouldBreakExecution(Debugger dbg) {
  return dbg->breakOnNext
           || isAtBreakpoint(dbg->persistentBreakpoints, getVmPC(dbg->vm))
           || isAtBreakpoint(dbg->temporaryBreakpoints, getVmPC(dbg->vm));
}

typedef int (*)(Debugger, DebugCommand) DebugCommandHandler;

static int executeDisassembleCmd(Debugger dbg, DebugCommand cmd);
static int executeDumpBytesCmd(Debugger dbg, DebugCommand cmd);
static int executeWriteBytesCmd(Debugger dbg, DebugCommand cmd);
static int executeDumpAddressStackCmd(Debugger dbg, DebugCommand cmd);
static int executeModifyAddressStackCmd(Debugger dbg, DebugCommand cmd);
static int executePushAddressStackCmd(Debugger dbg, DebugCommand cmd);
static int executePopAddressStackCmd(Debugger dbg, DebugCommand cmd);
static int executeDumpCallStackCmd(Debugger dbg, DebugCommand cmd);
static int executeModifyCallStackCmd(Debugger dbg, DebugCommand cmd);
static int executePushCallStackCmd(Debugger dbg, DebugCommand cmd);
static int executePopCallStackCmd(Debugger dbg, DebugCommand cmd);
static int executeListBreakpointsCmd(Debugger dbg, DebugCommand cmd);
static int executeAddBreakpointCmd(Debugger dbg, DebugCommand cmd);
static int executeRemoveBreakpointCmd(Debugger dbg, DebugCommand cmd);
static int executeRunProgramCmd(Debugger dbg, DebugCommand cmd);
static int executeRunUntilReturnCmd(Debugger dbg, DebugCommand cmd);
static int executeSingleStepIntoCmd(Debugger dbg, DebugCommand cmd);
static int executeSingleStepOverCmd(Debugger dbg, DebugCommand cmd);
static int executeHeapDumpCmd(Debugger dbg, DebugCommand cmd);
static int executeQuitVmCmd(Debugger dbg, DebugCommand cmd);
static int executeShowHelpCmd(Debugger dbg, DebugCommand cmd);
static int executeLookupSymbolCmd(Debugger dbg, DebugCommand cmd);

DebugCommandHandler debugCommandHandlers[] = {
  NULL,
  executeDisassembleCmd,
  executeDumpBytesCmd,
  executeWriteBytesCmd,
  executeDumpAddressStackCmd,
  executeModifyAddressStackCmd,
  executePushAddressStackCmd,
  executePopAddressStackCmd,
  executeDumpCallStackCmd,
  executeModifyCallStackCmd,
  executePushCallStackCmd,
  executePopCallStackCmd,
  executeListBreakpointsCmd,
  executeAddBreakpointCmd,
  executeRemoveBreakpointCmd,
  executeRunProgramCmd,
  executeRunUntilReturnCmd,
  executeSingleStepIntoCmd,
  executeSingleStepOverCmd,
  executeHeapDumpCmd,
  executeQuitVmCmd,
  executeShowHelpCmd,
  executeLookupSymbolCmd,
};

/** This is actually the number of commands plus 1 */
static const size_t NUM_COMMANDS =
  sizeof(debugCommandHandlers) / sizeof(DebugCommandHandler);

int executeDebugCommand(Debugger dbg, DebugCommand cmd) {
  clearDebuggerStatus(dbg);
  if (cmd->cmd == DEBUG_CMD_PARSE_ERROR) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Attempt to execute a malformed command (%" PRId32 "/%s)",
	     cmd->args.errorDetails.code, cmd->args.errorDetails.details);
    setDebuggerStatus(dbg, DebuggerIllegalArgumentError, msg);
    return -1;
  } else if ((cmd->cmd < 1) || (cmd->cmd >= NUM_COMMANDS)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Unknown debugger command with code %" PRId32,
	     cmd->cmd);
    setDebuggerStatus(dbg, DebuggerIllegalArgumentError, ms);
    return -1;
  } else {
    return debugCommandHandlers[cmd->cmd](dbg, cmd);
  }
}

static int executeDisassembleCmd(Debugger dbg, DebugCommand cmd) {
  const uint8_t* p = ptrToVmAddress(dbg->vm, cmd->args.disassemble.address);
  if (!p) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Invalid address %" PRIu64,
	     cmd->args.disassemble.address);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
    return -1;
  }

  VmMemory memory = getVmMemory(dbg->vm);
  SymbolTable symtab = getVmSymbolTable(dbg->vm);
  int inCodeRegion = (p >= getProgramStartInVmm(memory))
                       && (p < getVmmHeapStart(memory));
  for (uint32_t cnt = 0; cnt < cmd->args.disassemble.numLines; ++cnt) {
    const uint8_t* next = disassembleVmCode(p, ptrToVmMemory(memory),
					    getVmmHeapStart(memory),
					    ptrToVmMemoryEnd(memory),
					    symtab, stdout);
    if (!next || (next >= ptrToVmMemoryEnd(memory))
	  || (inCodeRegion && (next >= getVmmHeapStart(memory)))) {
      break;
    }
    p = next;
  }

  return 0;
}

static int executeDumpBytesCmd(Debugger dbg, DebugCommand cmd) {
  const uint8_t* p = ptrToVmAddress(dbg->vm, cmd->args.dumpBytes.address);
  if (!p) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Invalid address %" PRIu64,
	     cmd->args.dumpBytes.address);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
    return -1;
  }

  const uint8_t* startOfMemory = ptrToVmMemory(getVmMemory(dbg->vm));
  const uint8_t* endOfMemory = ptrToVmMemoryEnd(getVmMemory(dbg->vm));
  const uint8_t* end = p + cmd->args.dumpBytes.length;
  if (p > endOfMemory) {
    p = endOfMemory;
  }

  int cnt = 0;
  while (p < end) {
    if (!cnt) {
      fprintf(stdout, "%21" PRIu64, (p - startOfMemory));
    }
    fprintf(stdout, " %3" PRIu8, *p);
    if (++cnt == 16) {
      fprintf(stdout, "\n");
      cnt = 0;
    }
    ++p;
  }

  if (cnt) {
    fprintf(stdout, "\n");
  }

  return 0;
}

static int executeWriteBytesCmd(Debugger dbg, DebugCommand cmd) {
  const uint8_t* p = ptrToVmAddress(dbg->vm, cmd->args.writeBytes.address);
  if (!p) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Invalid address %" PRIu64,
	     cmd->args.writeBytes.address);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
    return -1;
  }

  const uint8_t* startOfMemory = ptrToVmMemory(getVmMemory(dbg->vm));
  const uint8_t* endOfMemory = ptrToVmMemoryEnd(getVmMemory(dbg->vm));
  const uint8_t* end = p + cmd->args.writeBytes.length;

  if (end > endOfMemory) {
    setDebuggerStatus(dbg, DebuggerInvalidCommandError,
		      "Write extends outside VM memory");
    return -1;
  }

  memcpy(p, cmd->args.writeBytes.data, cmd->args.writeBytes.length);
  return 0;
}

static int executeDumpAddressStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmAddressStack(dbg->vm);
  const uint64_t numAddresses = stackSize(s) / 8;

  if (cmd->args.dumpStack.depth >= numAddresses) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Address stack only has %" PRIu64 " addresses",
	     numAddresses);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
    return -1;
  }

  uint64_t end = cmd->args.dumpStack.depth + cmd->args.dumpStack.count;
  if (end > numAddresses) {
    end = numAddresses;
  }

  const uint64_t* top = (const uint64_t*)(topOfStack(s) - 8);
  for (uint64_t i = cmd->args.dumpStack.depth; i < end; ++i) {
    fprintf(stdout, "%21" PRIu64 " %" PRIu64 "\n", i, top[-i]);
  }

  return 0;
}

static int executeModifyAddressStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmAddressStack(dbg->vm);
  const uint64_t numAddresses = stackSize(s) / 8;

  if (cmd->args.modifyAddrStack.depth >= numAddresses) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Address stack only has %" PRIu64 " addresses",
	     numAddresses);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
    return -1;
  }

  uint64_t* top = (const uint64_t*)(topOfStack(s) - 8);
  top[-cmd->args.modifyAddrStack.depth] = cmd->args.modifyAddrStack.address;

  return 0;
}

static int executePushAddressStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmAddressStack(dbg->vm);

  if (pushStack(s, &cmd->args.pushAddrStack.address,
		sizeof(cmd->args.pushAddrStack.address))) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Push to address stack failed (%s)",
	     getStackStatusMsg(s));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  return 0;
}

static int executePopAddressStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmAddressStack(dbg->vm);

  if (popStack(s, NULL, sizeof(uint64_t))) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Pop from address stack failed (%s)",
	     getStackStatusMsg(s));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  return 0;
}

static int executeDumpCallStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmCallStack(dbg->vm);
  const uint64_t numFrames = stackSize(s) / 16;

  if (cmd->args.dumpStack.depth >= numFrames) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Call stack only has %" PRIu64 " frames",
	     numFrames);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
  }

  uint64_t end = cmd->args.dumpStack.depth + cmd->args.dumpStack.count;
  if (end > numFrames) {
    end = numFrames;
  }

  const uint64_t* top = (const uint64_t*)(topOfStack(s) - 16);
  for (uint64_t i = cmd->args.dumpStack.depth; i < end; ++i) {
    fprintf(stdout, "%21" PRIu64, " %21" PRIu64 " %21" PRIu64 "\n",
	    i, top[-2 * i], top[1 - 2 * i]);
  }

  return 0;
}

static int executeModifyCallStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmCallStack(dbg->vm);
  const uint64_t numFrames = stackSize(s) / 16;

  if (cmd->args.modifyCallStack.depth >= numFrames) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Call stack only has %" PRIu64 " frames",
	     numFrames);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
  }

  const uint64_t* top = (const uint64_t*)(topOfStack(s) - 16);
  top[-2 * cmd->args.modifyCallStack.depth] =
    cmd->args.modifyCallStack.blockAddress;
  top[1 - 2 * cmd->args.modifyCallStack.depth] =
    cmd->args.modifyCallStack.returnAddress;

  return 0;
}

static int executePushCallStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmCallStack(dbg->vm);

  if (pushStack(s, &cmd->args.pushCallStack.blockAddress,
		sizeof(cmd->args.pushCallStack.blockAddress))
        || pushStack(s, &cmd->args.pushCallStack.returnAddress,
		     sizeof(cmd->args.pushCallStack.returnAddress))) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Push to call stack failed (%s)",
	     getStackStatusMsg(s));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  return 0;
}

static int executePopCallStackCmd(Debugger dbg, DebugCommand cmd) {
  Stack s = getVmCallStack(dbg->vm);
  uint64_t returnAddress = 0;

  if (popStack(s, &returnAddress, sizeof(returnAddress))) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Pop from call stack failed (%s)",
	     getStackStatusMsg(s));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  if (popStack(s, NULL, sizeof(uint64_t))) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Pop from call stack failed (%s)",
	     getStackStatusMsg(s));

    assert(!pushStack(s, &blockAddress, sizeof(returnAddress)));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  return 0;
}

static int executeListBreakpointsCmd(Debugger dbg, DebugCommand cmd) {
  for (const uint64_t* p = startOfBreakpointList(dbg->persistentBreakpoints);
       p != dbg->endOfBreakpointList(dbg->persistentBreakpoints);
       ++p) {
    fprintf(stdout, "%11" PRIu32 " %21\n" PRIu64,
	    (p - startOfBreakpoints(dbg->persistentBreakpoints), *p));
  }
  fprintf(stdout, "----------- ---------------------\n");
  fprintf(stdout, "%11" PRIu32 " breakpoints\n",
	  breakpointListSize(dbg->persistentBreakpoints));
  
  return 0;
}

static int executeAddBreakpointCmd(Debugger dbg, DebugCommand cmd) {
  if (addBreakpointToList(dbg->persistentBreakpoints,
			  cmd->args.addBreakpoint.address)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Failed to add breakpoint (%s)",
	     getBreakpointListStatusMsg(dbg->persistentBreakpoints));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  return 0;
}

static int executeRemoveBreakpointCmd(Debugger dbg, DebugCommand cmd) {
  if (removeBreakpointFromList(dbg->persistentBreakpoints,
			       cmd->args.removeBreakpoint.address)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Failed to remove breakpoint (%s)",
	     getBreakpointListStatusMsg(dbg->persistentBreakpoints));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }
  return 0;
}

static int executeRunProgramCmd(Debugger dbg, DebugCommand cmd) {
  if (!isValidVmmAddress(getVmMemory(dbg->vm), cmd->args.run.address)) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Cannot resume execution at invalid address %" PRIu64,
	     cmd->args.run.address);
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, msg);
    return -1;
  }
  
  setVmPC(dbg->vm, cmd->args.run.address);
  setDebuggerStatus(dbg, DebuggerResumeExecution, "Resume execution");
  dbg->breakOnNext = 0;
  return 0;
}

static int executeRunUntilReturnCmd(Debugger dbg, DebugCommand cmd) {
  Stack callStack = getVmCallStack(dbg->vm);

  if (!stackSize(callStack)) {
    setDebuggerStatus(dbg, DebuggerCommandExecutionError,
		      "Call stack is empty");
    return -1;
  }

  const uint64_t* pReturn = (const uint64_t*)(topOfStack(callStack) - 8);
  if (!addBreakpointToList(dbg->temporaryBreakpoints, *pReturn)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Failed to set temporary breakpoint (%s)",
	     getBreakpointListStatusMsg(dbg->temporaryBreakpoints));
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  setDebuggerStatus(dbg, DebuggerResumeExecution, "Resume execution");
  dbg->breakOnNext = 0;
  return 0;
}

static int executeSingleStepIntoCmd(Debugger dbg, DebugCommand cmd) {
  setDebuggerStatus(dbg, DebuggerResumeExecution, "Resume execution");
  dbg->breakOnNext = 1;
  return 0;  
}

static int executeSingleStepOverCmd(Debugger dbg, DebugCommand cmd) {
  uint64_t pc = getVmPC(dbg->vm);
  uint8_t* pcp = ptrToVmPC(dbg->vm);
  
  if (!isValidVmmAddress(getVmMemory(dbg->vm), pc)) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Cannot resume execution at invalid address %" PRIu64, pc);
    setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
    return -1;
  }

  pc += instructionSize(*pcp);
  if (!isValidVmmAddress(getVmMemory(dbg->vm), pc)) {
    setDebuggerStatus(dbg, DebuggerCommandExecutionError,
		      "No next instruction.  PC is at the last instruction "
		      "in memory");
    return -1;
  }

  addBreakpointToList(dbg->temporaryBreakpoints, pc);
  setDebuggerStatus(dbg, DebuggerResumeExecution, "Resume execution");
  dbg->breakOnNext = 0;
  return 0;
}

static void performHeapDump(FILE* out, VmMemory memory) {
  HeapBlock* p = firstHeapBlockInVmm(memory);
  uint64_t blockCount = 0;

  while (p) {
    fprintf(out, "%21" PRIu64 " %21" PRIu64 " %1s ",
	    vmmAddressForPtr(memory, p), getVmmBlockSize(p),
	    vmmBlockIsMarked(p) ? "X" : " ");
    
    switch (getVmmBlockType(p)) {
      case VmmFreeBlockType:
	fprintf(out, "FREE next=%" PRIu64 "\n",
		((FreeBlock*)p)->next);
	break;

      case VmmCodeBlockType:
	fprintf(out, "CODE\n");
	break;

      case VmmStateBlockType:
	fprintf(out, "STATE (as=%" PRIu64 ", cs=%" PRIu64 ")\n",
		((VmStateBlock*)p)->addressStackSize,
		((VmStateBlock*)p)->callStackSize);
	break;

      default:
	fprintf(out, "**UNKNOWN (type=%" PRIu32 ")\n",
		(uint32_t)getVmmBlockType(p));
	break;
    }
    p = nextHeapBlockInVmm(memory);
  }

  fprintf(out, "--------------------- --------------------- --- "
	  "-------------\n");
  fprintf(out, "%" PRIu64 " heap blocks\n", blockCount);
}
  
static int executeHeapDumpCmd(Debugger dbg, DebugCommand cmd) {
  if (cmd.args.heapDump.filename) {
    FILE* out = fopen(cmd.args.heapDump.filename, "w");
    if (!out) {
      char msg[200];
      snprintf("Failed to open file %s (%s)",
	       cmd.args.heapDump.filename, strerror(errno));
      setDebuggerStatus(dbg, DebuggerCommandExecutionError, msg);
      return -1;
    }
    performHeapDump(out, getVmMemory(dbg->vm));
    fclose(out);
    return 0;
  } else {
    performHeapDump(stdout, getVmMemory(dbg->vm));
    return 0;
  }
}

static int executeQuitVmCmd(Debugger dbg, DebugCommand cmd) {
  setDebuggerStatus(dbg, DebuggerQuitVm, "Quit VM");
  return 0;
}

static int executeShowHelpCmd(Debugger dbg, DebugCommand cmd) {
  fprintf(stdout,
    "l [address] [# lines]\n"
    "  Disassemble \"# lines\" of code starting at \"addr\" (default is\n"
      "current PC)\n"
    "d <address> [# bytes]\n"
    "  Dump bytes starting at <address>\n"
    "w <addr> <byte> [byte...]\n"
    "  Write bytes starting at address.  Can write up to 65,536 bytes with\n"
      "per \"w\" command\n"
    "as [depth] [count]\n"
    "  Dump \"count\" frames from the address stack starting at \"depth\" \n"
      "with the top of the stack being depth 0\n"
    "was <depth> <address>\n"
    "  Replace the address at the given depth on the address stack\n"
    "pas <address>\n"
    "  Push \"address\" onto the address stack\n"
    "ppas\n"
    "  Pop the top of the address stack\n"
    "cs [depth] [count]\n"
    "  Dump \"count\" frames from the call stack starting at \"depth\" \n"
      "with the top of the stack being depth 0\n"
    "wcs <depth> <block-addr> <ret-addr>\n"
    "  Replace the call stack frame at the given depth with the given \n"
      "address of the block called into and return address\n"
    "pcs <block-addr> <ret-addr>\n"
    "  Push a new frame onto the call stack\n"
    "ppcs\n"
    "  Pop the frame at the top of the call stack\n"
    "b\n"
    "  List all breakpoints\n"
    "ba <address>\n"
    "  Add a new breakpoint\n"
    "bd <address>\n"
    "  Remove a breakpoint\n"
    "r [address]\n"
    "  Resume execution at the given address (default is current PC)\n"
    "rr\n"
    "  Run until return from the current procedure call\n"
    "s\n"
    "  Execute one instruction, stepping into procedure calls\n"
    "ss\n"
    "  Execute one instruction, stepping over procedure calls\n"
    "hd [filename]\n"
    "  Dump the current block structure of the heap\n"
    "q\n"
    "  Exit the VM\n"
    "h\n"
    "  Print this help message\n"
    "sym <name>\n"
    "  Lookup a symbol by name\n"
  );

  return 0;
}

static int executeLookupSymbolCmd(Debugger dbg, DebugCommand cmd) {
  if (!cmd->args.lookupSymbol.name || !*cmd->args.lookupSymbol.name) {
    setDebuggerStatus(dbg, DebuggerInvalidCommandError, "Symbol name missing");
    return -1;
  }

  Symbol s = findSymbol(getVmSymbolTable(dbg->vm), cmd->args.lookupSymbol.name);
  if (s) {
    fprintf(stdout, "Symbol [%s] is at %" PRIu64 "\n", s->name, s->address);
  } else {
    fprintf(stdout, "Symbol [%s] not found\n", cmd->args.lookupSymbol.name);
  }

  return 0;
}

