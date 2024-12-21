#ifndef __BRKPT_H__
#define __BRKPT_H__

#include <stddef.h>
#include <stdint.h>

/** A list of breakpoints.  */
typedef struct BreakpointListImpl_* BreakpointList;

BreakpointList createBreakpointList(uint32_t maxBreakpoints);
void destroyBreakpointList(BreakpointList brkList);

int getBreakpointListStatus(BreakpointList brkList);
const char* getBreakpointListStatusMsg(BreakpointList brkList);
void clearBreakpointListStatus(BreakpointList brkList);

uint32_t breakpointListSize(BreakpointList brkList);
uint32_t breakpointListMaxSize(BreakpointList brkList);
const uint64_t* startOfBreakpointList(BreakpointList brkList);

const uint64_t* endOfBreakpointList(BreakpointList brkList);
const uint64_t* findBreakpointInList(BreakpointList brkList, uint64_t address);
const uint64_t* findBreakpointAtOrAfter(BreakpointList brkList,
					uint64_t address);
const uint64_t* findBreakpointAfter(BreakpointList brkList, uint64_t address);

int isAtBreakpoint(BreakpointList brkList, uint64_t pc);
int addBreakpointToList(BreakpointList brkList, uint64_t address);
int removeBreakpointFromList(BreakpointList brkList, uint64_t address);
int clearBreakpointList(BreakpointList brkList);

#ifdef __cplusplus

const int BreakpointListInvalidArgumentError = -1;
const int BreakpointListFullError = -2;
const int BreakpointListOutOfMemoryError = -3;
const int BreakpointListOtherError = -4;

#else

const int BreakpointListInvalidArgumentError;
const int BreakpointListFullError;
const int BreakpointListOutOfMemoryError;
const int BreakpointListOtherError;

#endif

#endif
