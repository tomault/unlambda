#include <brkpt.h>
#include <array.h>

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct BreakpointListImpl_ {
  Array breakpoints;
  uint32_t current;
  uint32_t maxBreakpoints;
  uint64_t lastPC;
  int statusCode;
  const char* statusMsg;
} BreakpointListImpl;

static const char OK_MSG[] = "OK";
static const char DEFAULT_ERR_MSG[] = "ERROR";

const int BreakpointListInvalidArgumentError = -1;
const int BreakpointListFullError = -2;
const int BreakpointListOutOfMemoryError = -3;
const int BreakpointListOtherError = -4;

BreakpointList createBreakpointList(uint32_t maxBreakpoints) {
  if (!maxBreakpoints) {
    return NULL;
  }

  BreakpointList bl = (BreakpointList)malloc(sizeof(BreakpointListImpl));

  bl->breakpoints = createArray(0, maxBreakpoints * sizeof(uint64_t));
  if (!bl->breakpoints) {
    free(bl);
    return NULL;
  }
  bl->current = 0;
  bl->maxBreakpoints = maxBreakpoints;
  bl->lastPC = 0;
  bl->statusCode = 0;
  bl->statusMsg = OK_MSG;

  return bl;
}

void destroyBreakpointList(BreakpointList brkList) {
  if (brkList) {
    clearBreakpointListStatus(brkList);
    destroyArray(brkList->breakpoints);
    free(brkList);
  }
}

int getBreakpointListStatus(BreakpointList brkList) {
  return brkList->statusCode;
}

const char* getBreakpointListStatusMsg(BreakpointList brkList) {
  return brkList->statusMsg;
}

static int shouldDeallocateStatusMsg(BreakpointList brkList) {
  return brkList->statusMsg && (brkList->statusMsg != OK_MSG)
           && (brkList->statusMsg != DEFAULT_ERR_MSG);
}

void clearBreakpointListStatus(BreakpointList brkList) {
  if (shouldDeallocateStatusMsg(brkList)) {
    free((void*)brkList->statusMsg);
  }
  brkList->statusCode = 0;
  brkList->statusMsg = OK_MSG;
}

static void setBreakpointListStatus(BreakpointList brkList, int code,
				    const char* msg) {
  if (shouldDeallocateStatusMsg(brkList)) {
    free((void*)brkList->statusMsg);
  }

  if ((msg == OK_MSG) && (msg == DEFAULT_ERR_MSG)) {
    brkList->statusMsg = msg;
  } else {
    brkList->statusMsg = malloc(strlen(msg) + 1);
    if (!brkList->statusMsg) {
      brkList->statusMsg = DEFAULT_ERR_MSG;
    } else {
      strcpy((char*)brkList->statusMsg, msg);
    }
  }
  brkList->statusCode = code;
}

uint32_t breakpointListSize(BreakpointList brkList) {
  return arraySize(brkList->breakpoints) / 8;
}

uint32_t breakpointListMaxSize(BreakpointList brkList) {
  return brkList->maxBreakpoints;
}

const uint64_t* startOfBreakpointList(BreakpointList brkList) {
  return (const uint64_t*)startOfArray(brkList->breakpoints);
}

const uint64_t* endOfBreakpointList(BreakpointList brkList) {
  return (const uint64_t*)endOfArray(brkList->breakpoints);
}

/** Not part of the public API.  Used only for testing */
uint32_t breakpointListCurrentCandidate(BreakpointList brkList) {
  return brkList->current;
}

/** Not part of the public API.  Used only for testing */
uint64_t breakpointListLastPC(BreakpointList brkList) {
  return brkList->lastPC;
}

/** Not part of the public API.  Used only for testing */
void setBreakpointListLastPC(BreakpointList brkList, uint64_t address) {
  brkList->lastPC = address;
}

/** Not part of the public API.  Used only for testing */
void setBreakpointListCurrentCandidate(BreakpointList brkList, uint32_t c) {
  brkList->current = c;
}

const uint64_t* findBreakpointInList(BreakpointList brkList, uint64_t address) {
  /** Breakpoint list is expected to be small, so O(N) is ok */
  for (const uint64_t* p = startOfBreakpointList(brkList);
       p != endOfBreakpointList(brkList);
       ++p) {
    if (*p == address) {
      return p;
    }
  }

  return NULL;
}

const uint64_t* findBreakpointAtOrAfter(BreakpointList brkList,
					uint64_t address) {
  /** Breakpoint is list is expected to be small, so O(N) is ok */
  const uint64_t* p;
  for (p = startOfBreakpointList(brkList);
       (p != endOfBreakpointList(brkList)) && (*p < address);
       ++p);
  return p;
}

const uint64_t* findBreakpointAfter(BreakpointList brkList, uint64_t address) {
  /** Breakpoint is list is expected to be small, so O(N) is ok */
  const uint64_t* p;
  for (p = startOfBreakpointList(brkList);
       (p != endOfBreakpointList(brkList)) && (*p <= address);
       ++p);
  return p;
}

int isAtBreakpoint(BreakpointList brkList, uint64_t pc) {
  if (pc < brkList->lastPC) {
    /** PC went backwards - start search from beginning of list */
    const uint64_t* next = findBreakpointAtOrAfter(brkList, pc);
    brkList->current = next - startOfBreakpointList(brkList);
    brkList->lastPC = pc;
    return (next < endOfBreakpointList(brkList)) && (*next == pc);
  } else {
    /** PC went forwards.  Start search at brkList[current] */
    const uint64_t* breakpoints = startOfBreakpointList(brkList);
    const uint32_t numBreakpoints = breakpointListSize(brkList);
    while ((brkList->current < numBreakpoints)
	     && (breakpoints[brkList->current] < pc)) {
      ++(brkList->current);
    }
    brkList->lastPC = pc;
    return (brkList->current < numBreakpoints)
             && (breakpoints[brkList->current] == pc);
  }
}

int addBreakpointToList(BreakpointList brkList, uint64_t address) {
  const uint64_t* p = findBreakpointAtOrAfter(brkList, address);
  assert((p == endOfBreakpointList(brkList)) || (*p >= address));
  
  if ((p == endOfBreakpointList(brkList)) || (*p > address)) {
    if (insertIntoArray(brkList->breakpoints,
			(p - startOfBreakpointList(brkList)) * 8,
			(const uint8_t*)&address, sizeof(address))) {
      if (getArrayStatus(brkList->breakpoints) == ArraySequenceTooLongError) {
	clearArrayStatus(brkList->breakpoints);
	setBreakpointListStatus(brkList, BreakpointListFullError,
				"Breakpoint list is full");
	return -1;
      } else {
	char msg[200];
	assert(getArrayStatus(brkList->breakpoints) == ArrayOutOfMemoryError);

	snprintf(msg, sizeof(msg),
		 "Cannot allocate memory for new breakpoint (%s)",
		 getArrayStatusMsg(brkList->breakpoints));
	
	clearArrayStatus(brkList->breakpoints);
	setBreakpointListStatus(brkList, BreakpointListOutOfMemoryError, msg);
	return -1;
      }
    }

    /** TODO: Make this more efficient.  It will do for now */
    const uint64_t* q = findBreakpointAfter(brkList, brkList->lastPC);
    brkList->current = q - startOfBreakpointList(brkList);
  }

  clearBreakpointListStatus(brkList);
  return 0;
}

int removeBreakpointFromList(BreakpointList brkList, uint64_t address) {
  const uint64_t* p = findBreakpointInList(brkList, address);
  if (p) {
    if (removeFromArray(brkList->breakpoints,
			(p - startOfBreakpointList(brkList)) * 8, 8)) {
      char msg[200];
      snprintf(msg, sizeof(msg),
	       "Cannot remove breakpoint 0x%" PRIx64 " from breakpoint "
	       "list (%s)", address, getArrayStatusMsg(brkList->breakpoints));

      clearArrayStatus(brkList->breakpoints);
      setBreakpointListStatus(brkList, BreakpointListOtherError, msg);
      return -1;
    }

    /** TODO: Make this more efficient.  It will do for now */
    const uint64_t* q = findBreakpointAfter(brkList, brkList->lastPC);
    brkList->current = q - startOfBreakpointList(brkList);
  }
  clearBreakpointListStatus(brkList);
  return 0;
}

int clearBreakpointList(BreakpointList brkList) {
  clearArray(brkList->breakpoints);
  brkList->current = 0;
  clearBreakpointListStatus(brkList);
  return 0;
}

