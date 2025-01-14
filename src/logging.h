#ifndef __UNLAMBDA__LOGGING_H__
#define __UNLAMBDA__LOGGING_H__

#include <stack.h>
#include <symtab.h>

#include <stdint.h>
#include <stdio.h>

typedef struct LoggerImpl_* Logger;

Logger createLogger(FILE* output, uint32_t modulesEnabled);
void destroyLogger(Logger logger);

uint32_t loggingModulesEnabled(Logger logger);
int loggingModuleIsEnabled(Logger logger, uint32_t module);
void enableLoggingModules(Logger logger, uint32_t modules);
void disableLoggingModules(Logger logger, uint32_t modules);

void logMessage(Logger logger, uint32_t module, const char* fmt, ...);
void logAddressStack(Logger logger, Stack addressStack,
		     uint64_t heapStart, SymbolTable symtab);
void logCallStack(Logger logger, Stack callStack,
		  uint64_t heapStart, SymbolTable symtab);


/** Constants that identify modules */

#ifdef __cplusplus
/** General information and errors */
const uint32_t LogGeneralInfo  =      0x00000001;
  
/** Log instructions and debug commands executed */
const uint32_t LogInstructions =      0x00000002;

/** Log changes to the state of the call or address stacks */
const uint32_t LogStacks =            0x00000004;

/** Log allocations from memory and results of garbage collection */
const uint32_t LogMemoryAllocations = 0x00000008;

/** Log contents of code blocks constructed */
const uint32_t LogCodeBlocks =        0x00000010;

/** Log contents of state blocks constructed */
const uint32_t LogStateBlocks =       0x00000020;

/** Log garbage collection, level 1 */
const uint32_t LogGC1 =               0x00000040;

/** Log garbage collection, level 2 (includes level 1) */
const uint32_t LogGC2 =               0x000000C0;

#else

/** General information and errors */
const uint32_t LogGeneralInfo;

/** Log instructions and debug commands executed */
const uint32_t LogInstructions;

/** Log changes to the state of the call or address stacks */
const uint32_t LogStacks;

/** Log allocations from memory and results of garbage collection */
const uint32_t LogMemoryAllocations;

/** Log contents of code blocks constructed */
const uint32_t LogCodeBlocks;

/** Log contents of state blocks constructed */
const uint32_t LogStateBlocks;

/** Log garbage collection, level 1 */
const uint32_t LogGC1;

/** Log garbage collection, level 2 (includes level 1) */
const uint32_t LogGC2;

#endif

#endif
