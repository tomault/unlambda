#include <logging.h>
#include <vm_instructions.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

typedef struct LoggerImpl_ {
  FILE* out;
  uint32_t enabled;
} LoggerImpl;

const uint32_t LogGeneralInfo  =      0x00000001;
const uint32_t LogInstructions =      0x00000002;
const uint32_t LogStacks =            0x00000004;
const uint32_t LogMemoryAllocations = 0x00000008;
const uint32_t LogCodeBlocks =        0x00000010;
const uint32_t LogStateBlocks =       0x00000020;
const uint32_t LogGC1 =               0x00000040;
const uint32_t LogGC2 =               0x000000C0;
const uint32_t LogAllModules =
  LogGeneralInfo | LogInstructions | LogStacks | LogMemoryAllocations
    | LogCodeBlocks | LogStateBlocks | LogGC1 | LogGC2;

Logger createLogger(FILE* output, uint32_t modulesEnabled) {
  Logger logger = malloc(sizeof(LoggerImpl));
  if (logger) {
    logger->out = output;
    logger->enabled = modulesEnabled;
  }
  return logger;
}

void destroyLogger(Logger logger) {
  free((void*)logger);
}

uint32_t loggingModulesEnabled(Logger logger) {
  return logger ? logger->enabled : 0;
}

int loggingModuleIsEnabled(Logger logger, uint32_t module) {
  return logger && ((logger->enabled & module) == module);
}

void enableLoggingModules(Logger logger, uint32_t modules) {
  if (logger) {
    logger->enabled |= modules;
  }
}

void disableLoggingModules(Logger logger, uint32_t modules) {
  if (logger) {
    logger->enabled &= ~modules;
  }
}

static const char* getModuleName(uint32_t module) {
  /** TODO: Switch module codes to #defines (sigh) and use a switch statement */
  if (module == LogGeneralInfo) {
    return "INFO";
  } else if (module == LogInstructions) {
    return "INST";
  } else if (module == LogStacks) {
    return "STAC";
  } else if (module == LogMemoryAllocations) {
    return "MEMO";
  } else if (module == LogCodeBlocks) {
    return "CBLK";
  } else if (module == LogStateBlocks) {
    return "SBLK";
  } else if (module == LogGC1) {
    return "GC1 ";
  } else if (module == LogGC2) {
    return "GC2 ";
  } else {
    return "OTHR";
  }
}

static void logTimestamp(Logger logger, uint32_t module) {
  struct timeval now;
  struct tm timeInfo;
  char stamp[30];

  gettimeofday(&now, NULL);
  localtime_r(&now.tv_sec, &timeInfo);
  strftime(stamp, sizeof(stamp), "%Y/%m/%d %H:%M:%S", &timeInfo);

  fprintf(logger->out, "%s.%03d %s ", stamp, (int)(now.tv_usec / 1000),
	  getModuleName(module));
}

void logMessage(Logger logger, uint32_t module, const char* fmt, ...) {
  if (logger && loggingModuleIsEnabled(logger, module)) {
    va_list args;

    va_start(args, fmt);

    logTimestamp(logger, module);
    
    vfprintf(logger->out, fmt, args);
    fprintf(logger->out, "\n");
    fflush(logger->out);
    va_end(args);
  }
}

void logAddressStack(Logger logger, Stack addressStack, uint64_t heapStart,
		     SymbolTable symtab) {
  static const size_t NUM_FRAMES = 4;
  
  if (loggingModuleIsEnabled(logger, LogStacks)) {
    char* text;
    size_t textLength;
    FILE* memstream = open_memstream(&text, &textLength);
    if (!memstream) {
      logMessage(logger, LogStacks, "Could not log address stack: "
		 "open_memstream returned NULL");
      return;
    }

    const uint64_t* start = (const uint64_t*)topOfStack(addressStack);
    const uint64_t* end = (stackSize(addressStack) > NUM_FRAMES)
                            ? start - NUM_FRAMES
                            : (const uint64_t*)bottomOfStack(addressStack);
    
    fprintf(memstream, "Address stack is [");
    for (const uint64_t* p = start; p > end; --p) {
      if (p < start) {
	fprintf(memstream, ", ");
      }
      writeAddressWithSymbol(p[-1], 0, heapStart, symtab, memstream);
    }
    fprintf(memstream, "]");

    fclose(memstream);
    if (!text) {
      logMessage(logger, LogStacks, "Could not log address stack: "
		 "text is NULL");
    } else {
      logMessage(logger, LogStacks, text);
      free((void*)text);
    }
  }
}

void logCallStack(Logger logger, Stack callStack, uint64_t heapStart,
		  SymbolTable symtab) {
  static const size_t NUM_FRAMES = 4;
  
  if (loggingModuleIsEnabled(logger, LogStacks)) {
    char* text;
    size_t textLength;
    FILE* memstream = open_memstream(&text, &textLength);
    if (!memstream) {
      logMessage(logger, LogStacks,
		 "Could not log call stack: open_memstream returned NULL");
      return;
    }

    const uint64_t* start = (const uint64_t*)topOfStack(callStack);
    const uint64_t* end = (stackSize(callStack) > (2 * NUM_FRAMES))
                            ? start - 2 * NUM_FRAMES
                            : (const uint64_t*)bottomOfStack(callStack);
    
    fprintf(memstream, "Call stack is [");
    for (const uint64_t* p = start; p > end; p -= 2) {
      if (p < start) {
	fprintf(memstream, ", ");
      }
      /** Always write the block called into as a plain number */
      fprintf(memstream, "(%" PRIu64 ", ", p[-2]);
      writeAddressWithSymbol(p[-1], 0, heapStart, symtab, memstream);
      fprintf(memstream, ")");
    }
    fprintf(memstream, "]");

    fclose(memstream);
    if (!text) {
      logMessage(logger, LogStacks,
		 "Could not log call stack: text is NULL");
    } else {
      logMessage(logger, LogStacks, text);
      free((void*)text);
    }
  }
}
