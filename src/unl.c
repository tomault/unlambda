/** The Unlambda virtual machine */
#include <argparse.h>
#include <array.h>
#include <dbgcmd.h>
#include <debug.h>
#include <logging.h>
#include <vm.h>
#include <vm_instructions.h>

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct VmCmdLineArgs_ {
  /** Name of executable to load */
  const char* executableFilePath;

  /** Name of log file.  NULL disables logging */
  const char* logFilePath;

  /** Logging modules to enable */
  uint32_t loggingModules;
  
  /** Initial size of the virtual machine's memory, in bytes */
  uint64_t initialVmSize;

  /** Maximum size of the virtual machine's memory, in bytes */
  uint64_t maxVmSize;

  /** Maximum size of the address stack, in addresses */
  uint32_t maxAddressStackSize;

  /** Maximum size of the call stack, in addresses */
  uint32_t maxCallStackSize;

  /** Whether to load symbols from the executable or not */
  int loadSymbols;
  
  /** Any preset breakpoints */
  Array breakpoints;

  /** Whether to start in the debugger */
  int startInDebugger;

  /** Whether a PANIC instruction enters the debugger (0) or causes the VM to
   *  quit (1)
   */
  int quitOnPanic;

  /** Whether a HALT instruction causes the VM to quit (0) or enter the
   *  debugger (1)
   */
  int debugOnHalt;

  /** Whether to enter the debugger on a fatal error or fault (0) or quit
   *  the VM (1)
   */
  int quitOnFatalError;

  /** Whether or not to print the address on the top of the stack when the
   *  VM exits as the result of the computation
   */
  int printResultOnExit;

  /** Whether to show the usage message (1) or execute the program (0) */
  int showHelp;
} VmCmdLineArgs;

static const char* DEBUG_CMD_TOO_LONG = (const char*)-1;

static int isBlank(int c) {
  return (c == ' ') || (c == '\t') || (c == '\r');
}

static const char* readDebugCommand() {
  static const size_t MAX_CMD_LEN = 1023;
  char* cmdBuf = (char*)malloc(MAX_CMD_LEN + 1);
  if (!cmdBuf) {
    return NULL;
  }
  
  size_t i = 0;
  int c = fgetc(stdin);

  while ((c != EOF) && (c != '\n')) {
    if (i == MAX_CMD_LEN) {
      free((void*)cmdBuf);
      return DEBUG_CMD_TOO_LONG;
    }
    cmdBuf[i++] = c;
    c = fgetc(stdin);
  }

  assert(i <= MAX_CMD_LEN);

  /** Strip off any trailing whitespace */
  while ((i > 0) && isBlank(cmdBuf[i - 1])) {
    --i;
  }

  cmdBuf[i] = 0;
  return cmdBuf;
}

static const uint32_t MAX_BREAKPOINTS = 65536;

/** TODO: Break this up */
static int mainLoop(VmCmdLineArgs* args) {

  /** Open the log file, create the logger and set the logger in the VM */
  FILE* logFile = NULL;
  Logger logger = NULL;
  if (args->logFilePath) {
    /** TODO: Use open() and fdopen() to set logFile, so permissions
     *        can be set appripriately without relying on umask
     */
    logFile = fopen(args->logFilePath, "w");
    if (!logFile) {
      fprintf(stderr, "WARNING: Could not open log file %s.  Logging will "
	      "be disabled (%s)\n", args->logFilePath, strerror(errno));
    } else {
      logger = createLogger(logFile, args->loggingModules);
      if (!logger) {
	fprintf(stderr, "WARNING: Could not create logger.  Logging will "
		"be disabled\n");
	fclose(logFile);
	logFile = NULL;
      }
    }
  }

  assert((logFile && logger) || (!logFile && !logger));

  /** Create the VM and its debugger */
  UnlambdaVM vm = createUnlambdaVM(args->maxCallStackSize,
				   args->maxAddressStackSize,
				   args->initialVmSize,
				   args->maxVmSize);
  if (!vm) {
    fprintf(stderr, "Failed to create the VM.  Exiting.");
    if (logger) {
      destroyLogger(logger);
      fclose(logFile);
    }
    return -1;
  }
  if (logger) {
    setVmLogger(vm, logger);
  }

  Debugger dbg = createDebugger(vm, MAX_BREAKPOINTS);
  if (!dbg) {
    fprintf(stderr, "Failed to create the VM debugger.  Exiting.");
    destroyUnlambdaVM(vm);
    if (logger) {
      destroyLogger(logger);
      fclose(logFile);
    }
    return -1;
  }
  
  /** Load the program */
  if (loadProgramIntoVm(vm, args->executableFilePath, args->loadSymbols)) {
    fprintf(stderr, "%s\n", getVmStatusMsg(vm));
    destroyDebugger(dbg);
    destroyUnlambdaVM(vm);
    if (logger) {
      destroyLogger(logger);
      fclose(logFile);
    }
    return -1;
  }
  
  int shouldRun = 1;
  int enterDebugger = args->startInDebugger;
  VmMemory memory = getVmMemory(vm);
  int resultCode = 0;

  while (shouldRun) {
    if (enterDebugger || shouldBreakExecution(dbg)) {
      int shouldDebug = 1;

      while (shouldDebug) {
	if (isValidVmmAddress(memory, getVmPC(vm))) {
	  disassembleVmCode(ptrToVmPC(vm), ptrToVmMemory(memory),
			    getVmmHeapStart(memory), ptrToVmMemoryEnd(memory),
			    getVmSymbolTable(vm), stdout);
	} else {
	  fprintf(stdout, "PC is at invalid address %" PRIu64 "\n",
		  getVmPC(vm));
	}
	fprintf(stdout, "(debug) ");

	const char* cmdText = readDebugCommand();
	if (!cmdText) {
	  fprintf(stderr, "Cannot allocate memory for debug command buffer -- "
		  "Aborting VM\n");
	  shouldDebug = 0;
	  shouldRun = 0;
	  resultCode = -3;
	  break;
	} else if (cmdText == DEBUG_CMD_TOO_LONG) {
	  fprintf(stdout, "Command is too long\n");
	  continue;
	} else if (!cmdText[0]) {
	  /** Command is blank -- ignore */
	  continue;
	}
	
	DebugCommand cmd = parseDebugCommand(vm, cmdText);
	if (cmd->cmd == DEBUG_CMD_PARSE_ERROR) {
	  fprintf(stdout, "%s\n", cmd->args.errorDetails.details);
	} else if (executeDebugCommand(dbg, cmd)) {
	  fprintf(stdout, "%s\n\n", getDebuggerStatusMsg(dbg));
	} else {
	  fprintf(stdout, "\n");
	}

	if (getDebuggerStatus(dbg) == DebuggerResumeExecution) {
	  shouldDebug = 0;
	} else if (getDebuggerStatus(dbg) == DebuggerQuitVm) {
	  shouldDebug = 0;
	  shouldRun = 0;
	  resultCode = 0;
	}
	destroyDebugCommand(cmd);
	free((void*)cmdText);
      }
    }
    
    if (shouldRun) {
      if (stepVm(vm)) {
	int status = getVmStatus(vm);
	if (status == VmHalted) {
	  fprintf(stdout, "VM halted.");
	  if (args->printResultOnExit) {
	    Stack s = getVmAddressStack(vm);
	    if (stackSize(s) >= 8) {
	      const uint64_t* p = (const uint64_t*)topOfStack(s);
	      fprintf(stdout, "  Result is ");
	      writeAddressWithSymbol(
		p[-1], vmmAddressForPtr(memory, getVmmHeapStart(memory)),
		getVmSymbolTable(vm), stdout
	      );
	    } else {
	      fprintf(stdout, "  No result (address stack empty)\n");
	    }
	  }
	  fprintf(stdout, "\n");
	  shouldRun = args->debugOnHalt;
	  resultCode = 0;
	} else if (status == VmPanicError) {
	  fprintf(stdout, "VM executed PANIC instruction.\n");
	  shouldRun = !args->quitOnPanic;
	  resultCode = -1;
	} else {
	  fprintf(stdout, "%s\n", getVmStatusMsg(vm));
	  fprintf(stdout, "\n");
	  shouldRun = !args->quitOnFatalError;
	  resultCode = -2;
	}
	enterDebugger = shouldRun;
      }
    }
  }

  destroyDebugger(dbg);
  destroyUnlambdaVM(vm);
  if (logger) {
    destroyLogger(logger);
    fclose(logFile);
  }

  return resultCode;
}

static uint32_t parseLoggingModuleList(const char* moduleNameList,
				       char** errorMessage) {
  const char* p = moduleNameList;
  uint32_t modules = 0;

  while (*p) {
    const char* start = p;
    while (*p && (*p != '+')) {
      ++p;
    }
    if (p == start) {
      *errorMessage = strdup("Logging module name missing");
      return 0;
    }
    if (!strncmp(start, "info", p - start)) {
      modules |= LogGeneralInfo;
    } else if (!strncmp(start, "instructions", p - start)) {
      modules |= LogInstructions;
    } else if (!strncmp(start, "stacks", p - start)) {
      modules |= LogStacks;
    } else if (!strncmp(start, "allocations", p - start)) {
      modules |= LogMemoryAllocations;
    } else if (!strncmp(start, "codeblks", p - start)) {
      modules |= LogCodeBlocks;
    } else if (!strncmp(start, "stateblks", p - start)) {
      modules |= LogStateBlocks;
    } else if (!strncmp(start, "gc1", p - start)) {
      modules |= LogGC1;
    } else if (!strncmp(start, "gc2", p - start)) {
      modules |= LogGC2;
    } else {
      char* moduleName = malloc(p - start + 1);
      memcpy(moduleName, start, p - start);
      moduleName[p - start] = 0;

      char* msg = malloc(p - start + 31);
      snprintf(msg, p - start + 31, "Invalid logging module name \"%s\"",
	       moduleName);
      free((void*)moduleName);
      *errorMessage = msg;
      return 0;
    }

    if (*p == '+') {
      ++p;
      if (!*p) {
	*errorMessage = strdup("Logging module name missing");
	return 0;
      }
    }
  }

  return modules;
}

#define CHECK_FOR_MISSING_ARG(ARG_NAME)					\
  if (getCmdLineArgParserStatus(parser) == NoMoreCmdLineArgsError) {    \
    fprintf(stderr, "ERROR: Argument missing for %s\n", ARG_NAME);      \
    destroyCmdLineArgParser(parser);                                    \
    return -1;                                                          \
  }

#define CHECK_FOR_INVALID_ARG(ARG_NAME)                                 \
  if (getCmdLineArgParserStatus(parser) == InvalidCmdLineArgError) {	\
    fprintf(stderr, "ERROR: Value for %s is invalid (%s)",              \
	    ARG_NAME, getCmdLineArgParserStatusMsg(parser));		\
    destroyCmdLineArgParser(parser);                                    \
    return -1;                                                          \
  }

/** TODO: This function is very long.  Break it up */
static int parseCmdLineArgs(int argc, char** argv, VmCmdLineArgs* args) {
  static const uint64_t DEFAULT_INITIAL_VM_SIZE = 16 * 1024 * 1024;
  static const uint64_t DEFAULT_MAX_VM_SIZE = DEFAULT_INITIAL_VM_SIZE;
  static const uint32_t DEFAULT_MAX_CALL_STACK_SIZE = 1024 * 1024;
  static const uint32_t DEFAULT_MAX_ADDRESS_STACK_SIZE = 1024 * 1024;


  /** Initialize command-line arguments */
  args->executableFilePath = NULL;
  args->logFilePath = NULL;
  args->loggingModules = 0;
  args->initialVmSize = 0;
  args->maxVmSize = 0;
  args->maxAddressStackSize = DEFAULT_MAX_ADDRESS_STACK_SIZE;
  args->maxCallStackSize = DEFAULT_MAX_CALL_STACK_SIZE;
  args->loadSymbols = 1;
  args->breakpoints = createArray(0, MAX_BREAKPOINTS * sizeof(uint64_t));
  args->startInDebugger = 0;
  args->quitOnPanic = 0;
  args->debugOnHalt = 0;
  args->quitOnFatalError = 0;
  args->printResultOnExit = 0;
  args->showHelp = 0;

  /** Parse the command line arguments and update args */
  CmdLineArgParser parser = createCmdLineArgParser(argc, argv);
  if (!parser) {
    fprintf(stderr, "Could not allocate memory for command-line parser - "
	    "Aborting VM\n");
    return -1;
  }

  while (hasMoreCmdLineArgs(parser)) {
    const char* argName = nextCmdLineArg(parser);
    if (!strcmp(argName, "--log-file")) {
      args->logFilePath = nextCmdLineArg(parser);
      CHECK_FOR_MISSING_ARG(argName);
    } else if (!strcmp(argName, "--log-modules")) {
      const char* logModuleList = nextCmdLineArg(parser);
      char* errorMessage = NULL;
      
      CHECK_FOR_MISSING_ARG(argName);
      args->loggingModules = parseLoggingModuleList(logModuleList,
						    &errorMessage);
      if (errorMessage) {
	fprintf(stderr, "Value for %s is invalid (%s)", argName, errorMessage);
	free((void*)errorMessage);
	destroyCmdLineArgParser(parser);
	return -1;
      }
    } else if (!strcmp(argName, "--initial-memory")) {
      args->initialVmSize = nextCmdLineArgAsMemorySize(parser);
      CHECK_FOR_MISSING_ARG(argName);
      CHECK_FOR_INVALID_ARG(argName);
    } else if (!strcmp(argName, "--max-memory")) {
      args->maxVmSize = nextCmdLineArgAsMemorySize(parser);
      CHECK_FOR_MISSING_ARG(argName);
      CHECK_FOR_INVALID_ARG(argName);
    } else if (!strcmp(argName, "--max-call-stack")) {
      uint64_t maxStackSize = nextCmdLineArgAsMemorySize(parser);
      CHECK_FOR_MISSING_ARG(argName);
      CHECK_FOR_INVALID_ARG(argName);
      if (maxStackSize > 0xFFFFFFFF) {
	fprintf(stderr, "ERROR: Invalid value for %s (Maximum stack size "
		"is 4g)\n", argName);
	destroyCmdLineArgParser(parser);
	return -1;
      }
      args->maxCallStackSize = maxStackSize;
    } else if (!strcmp(argName, "--max-address-stack")) {
      uint64_t maxStackSize = nextCmdLineArgAsMemorySize(parser);
      CHECK_FOR_MISSING_ARG(argName);
      CHECK_FOR_INVALID_ARG(argName);
      if (maxStackSize > 0xFFFFFFFF) {
	fprintf(stderr, "ERROR: Invalid value for %s (Maximum stack size "
		"is 4g)\n", argName);
	destroyCmdLineArgParser(parser);
	return -1;
      }
      args->maxAddressStackSize = maxStackSize;
    } else if (!strcmp(argName, "--no-symbols")) {
      args->loadSymbols = 0;
    } else if (!strcmp(argName, "--breakpoint")) {
      uint64_t address = nextCmdLineArgAsUInt64(parser);
      CHECK_FOR_MISSING_ARG(argName);
      CHECK_FOR_INVALID_ARG(argName);
      if (appendToArray(args->breakpoints, (const uint8_t*)&address,
			sizeof(address))) {
	fprintf(stderr, "ERROR: Could not add %" PRIu64
		" to breakpoint list (%s)\n", address,
		getArrayStatusMsg(args->breakpoints));
	destroyCmdLineArgParser(parser);
	return -1;
      }
    } else if (!strcmp(argName, "--start-in-debug")) {
      args->startInDebugger = 1;
    } else if (!strcmp(argName, "--quit-on-panic")) {
      args->quitOnPanic = 1;
    } else if (!strcmp(argName, "--debug-on-halt")) {
      args->debugOnHalt = 1;
    } else if (!strcmp(argName, "--quit-on-fatal")) {
      args->quitOnFatalError = 1;
    } else if (!strcmp(argName, "--print-result")) {
      args->printResultOnExit = 1;
    } else if (!strcmp(argName, "-h") || !strcmp(argName, "--help")) {
      args->showHelp = 1;
    } else if (!args->executableFilePath) {
      args->executableFilePath = argName;
    } else {
      fprintf(stderr, "ERROR: Too many command-line arguments.  "
	      "Use -h for help\n");
      destroyCmdLineArgParser(parser);
      return -1;
    }
  }

  destroyCmdLineArgParser(parser);
  
  /** Check for required arguments and propagate defaults */
  if (!args->showHelp && !args->executableFilePath) {
    fprintf(stderr, "ERROR: Program executable filename missing.  "
	    "Use -h for help\n");
    return -1;
  }

  if (!args->initialVmSize) {
    args->initialVmSize = args->maxVmSize ? args->maxVmSize
                                          : DEFAULT_INITIAL_VM_SIZE;
  }

  if (!args->maxVmSize) {
    args->maxVmSize = args->initialVmSize;
  }

  if (args->maxVmSize < args->initialVmSize) {
    fprintf(stderr, "ERROR: Max VM size (%" PRIu64 ") is less than initial "
	    "VM size (%" PRIu64 ")\n", args->maxVmSize, args->initialVmSize);
    return -1;
  }

  if (args->logFilePath && !args->loggingModules) {
    args->loggingModules = LogGeneralInfo;
  }

  return 0;
}

static void showUsage() {
  fprintf(stdout, "Usage message goes here\n");
}

int main(int argc, char* argv[]) {
  VmCmdLineArgs args;
  int result = parseCmdLineArgs(argc, argv, &args);

  if (!result) {
    if (args.showHelp) {
      showUsage();
      result = 0;
    } else {
      result = mainLoop(&args);
    }
  }
  
  if (args.breakpoints) {
    destroyArray(args.breakpoints);
  }
  
  return result;
}
