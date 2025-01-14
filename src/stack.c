#include "stack.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct StackImpl_ {
  uint8_t* data;    /** Data allocated for the stack */
  uint8_t* end;     /** End of data allocated for the stack */
  uint8_t* top;     /** Current stack top */
  size_t maxSize;   /** Maximum size of the stack */
  int statusCode;   /** Last operation result code.  0 == success */
  const char* statusMsg;  /** Last operation status message */
} StackImpl;

static const char OK_MSG[] = "OK";
static const char DEFAULT_ERR_MSG[] = "ERROR";

#ifndef __cplusplus
const int StackOverflowError = -1;
const int StackMemoryAllocationFailedError = -2;
const int StackUnderflowError = -3;
const int StackInvalidArgumentError = -4;
#endif

static size_t doubleStackSize(size_t currentSize, size_t maxSize);
static int increaseStackSize(Stack s, size_t n);
static void setStackStatus(Stack s, int statusCode, const char* statusMsg);
static void setStackOverflowError(Stack s, size_t size);
static int shouldDeallocateStatusMsg(Stack s);

Stack createStack(size_t initialSize, size_t maxSize) {
  if ((!maxSize) || (initialSize > maxSize)) {
    return NULL;
  }
  
  Stack s = (Stack)malloc(sizeof(StackImpl));
  if (!s) {
    return NULL;
  }

  s->data = (uint8_t*)malloc(initialSize);
  if (!s->data) {
    free((void*)s);
    return NULL;
  }

  s->end = s->data + initialSize;
  s->top = s->data;
  s->maxSize = maxSize;
  s->statusCode = 0;
  s->statusMsg = OK_MSG;

  return s;
}

void destroyStack(Stack s) {
  clearStackStatus(s);
  free((void*)s->data);
  free((void*)s);
}

size_t stackSize(Stack s) {
  return s->top - s->data;
}

size_t stackMaxSize(Stack s) {
  return s->maxSize;
}

size_t stackAllocated(Stack s) {
  return s->end - s->data;
}

uint8_t* bottomOfStack(Stack s) {
  return s->data;
}

uint8_t* topOfStack(Stack s) {
  return s->top;
}

int pushStack(Stack s, const void* item, size_t size) {
  clearStackStatus(s);
  
  if (!size) {
    return 0;
  }

  if (!item) {
    setStackStatus(s, StackInvalidArgumentError, "\"item\" is NULL");
    return -1;
  }

  uint8_t* const newTop = s->top + size;
  if (newTop < s->top) {
    /** Pointer overflow.  Definitely exceeds maxSize */
    setStackOverflowError(s, size);
    return -1;
  }
  
  if ((newTop > s->end) && increaseStackSize(s, size)) {
    /** Tried to increase stack size, but couldn't */
    setStackOverflowError(s, size);
    return -1;
  }

  /** Note that newTop may not be valid if increaseStackSize moved s->data
   *  to a new location.
   */
  memcpy((void*)s->top, (const void*)item, size);
  s->top += size;
  return 0;
}

int popStack(Stack s, void* item, size_t size) {
  clearStackStatus(s);
  if (!size) {
    return 0;
  }
  
  if ((s->top - s->data) < size) {
    char msg[100];
    snprintf(msg, sizeof(msg),
	     "Cannot pop %zu bytes from a stack with only %zu bytes on it",
	     size, (size_t)(s->top - s->data));
    setStackStatus(s, StackUnderflowError, msg);
    return -1;
  }

  if (item) {
    memcpy(item, (const void*)(s->top - size), size);
  }

  s-> top -= size;
  return 0;
}

int readStackTop(Stack s, void* p, size_t size) {
  clearStackStatus(s);
  if (!size) {
    return 0;
  }
  
  if (!p) {
    setStackStatus(s, StackInvalidArgumentError, "\"p\" is NULL");
    return -1;
  }
  
  if ((s->top - s->data) < size) {
    char msg[100];
    snprintf(msg, sizeof(msg),
	     "Cannot read %zu bytes from a stack with only %zu bytes on it",
	     size, (size_t)(s->top - s->data));
    setStackStatus(s, StackUnderflowError, msg);
    return -1;
  }

  memcpy(p, (const void*)(s->top - size), size);
  return 0;
}

int swapStackTop(Stack s, size_t size) {
  clearStackStatus(s);
  if (!size) {
    return 0;
  }

  if (((2 * size) < size) || ((s->top - s->data) < (2 * size))) {
    char msg[100];
    snprintf(msg, sizeof(msg),
	     "Cannot swap the top %zu bytes on a stack that only has %zu "
	     "bytes", size, (size_t)(s->top - s->data));
    setStackStatus(s, StackUnderflowError, msg);
    return -1;
  }

  void* tmp = malloc(size);
  if (!tmp) {
    setStackStatus(s, StackMemoryAllocationFailedError,
		   "Unable to allocate temporary buffer for swap");
    return -1;
  }

  memmove(tmp, (const void*)(s->top - size), size);
  memmove((void*)(s->top - size), (const void*)(s->top - 2 * size), size);
  memmove((void*)(s->top - 2 * size), tmp, size);
  free(tmp);
  return 0;
}

int dupStackTop(Stack s, size_t size) {
  clearStackStatus(s);

  if (!size) {
    return 0;
  }

  if (size > (s->top - s->data)) {
    char msg[100];
    snprintf(msg, sizeof(msg),
	     "Cannot duplicate %zd bytes on a stack that has only %zd bytes",
	     size, (s->top - s->data));
    setStackStatus(s, StackUnderflowError, msg);
    return -1;
  }
  uint8_t* newTop = s->top + size;
  if (newTop < s->top) {
    /** Pointer overflow.  Stack would exceed maxSize */
    setStackOverflowError(s, size);
    return -1;
  }

  if ((newTop > s->end) && increaseStackSize(s, size)) {
    /** Couldn't allocate enough memory for the dup */
    return -1;
  }

  /** newTop might be invalid if increaseStackSize() moved s->data */
  memcpy((void*)s->top, (const void*)(s->top - size), size);
  s->top += size;
  return 0;
}

void clearStack(Stack s) {
  clearStackStatus(s);
  s->top = s->data;
}

int setStack(Stack s, const uint8_t* data, uint64_t size) {
  clearStackStatus(s);

  if (size > s->maxSize) {
    setStackOverflowError(s, size);
    return -1;
  } else if (size > stackAllocated(s)) {
    if (increaseStackSize(s, size)) {
      return -1;
    }
    assert(size <= stackAllocated(s));
  }
  
  memcpy((void*)s->data, (const void*)data, size);
  s->top = s->data + size;

  return 0;
}

int getStackStatus(Stack s) {
  return s->statusCode;
}

const char* getStackStatusMsg(Stack s) {
  return s->statusMsg;
}

void clearStackStatus(Stack s) {
  if (shouldDeallocateStatusMsg(s)) {
    free((void*)s->statusMsg);
  }
  s->statusCode = 0;
  s->statusMsg = OK_MSG;
}

static size_t doubleStackSize(size_t currentSize, size_t maxSize) {
  const size_t newSize = currentSize ? 2 * currentSize : 16;
  return ((newSize < currentSize) || (newSize > maxSize)) ? maxSize : newSize;
}

static int increaseStackSize(Stack s, size_t n) {
  const size_t currentSize = s->top - s->data;
  const size_t desiredSize = currentSize + n;

  if ((desiredSize < currentSize) || (desiredSize > s->maxSize)) {
    /** Desired stack size exceeds maximum allowed */
    setStackOverflowError(s, n);
    return -1;
  }
  
  size_t newSize = doubleStackSize(currentSize, s->maxSize);
  /** Because desiredSize <= maxSize, and doubleStackSize(current, maxSize)
   *  is always > current unless current == maxSize, in which case the
   *  result is equal to maxSize, this loop must exit.
   */
  while (newSize < desiredSize) {
    newSize = doubleStackSize(newSize, s->maxSize);
  }

  uint8_t* newStack = (uint8_t*)realloc(s->data, newSize);
  if (!newStack) {
    char msg[100];
    snprintf(msg, sizeof(msg),
	     "Resizing stack to %zd bytes failed to allocate memory",
	     newSize);
    setStackStatus(s, StackMemoryAllocationFailedError, msg);
    return -1;
  }

  /** realloc() would free old s->data if needed */
  s->data = newStack;
  s->end = s->data + newSize;
  s->top = s->data + currentSize;

  return 0;
}

static void setStackStatus(Stack s, int statusCode, const char* statusMsg) {
  if (shouldDeallocateStatusMsg(s)) {
    free((void*)s->statusMsg);
  }

  s->statusMsg = (const char*)malloc(strlen(statusMsg) + 1);
  if (!s->statusMsg) {
    s->statusMsg = DEFAULT_ERR_MSG;
  } else {
    strcpy((char*)s->statusMsg, statusMsg);
  }
  s->statusCode = statusCode;
}

static void setStackOverflowError(Stack s, size_t size) {
  char msg[200];
  snprintf(msg, sizeof(msg),
	   "Stack overflow - increasing the size of the stack by %zu bytes "
	   "would exceed the maximum size of %zu bytes",
	   size, s->maxSize);
  setStackStatus(s, StackOverflowError, msg);
}

static int shouldDeallocateStatusMsg(Stack s) {
  return s->statusMsg && (s->statusMsg != OK_MSG)
             && (s->statusMsg != DEFAULT_ERR_MSG);
}
