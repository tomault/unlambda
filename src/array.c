#include "array.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ArrayImpl_ {
  /** Start of the array */
  uint8_t* data;

  /** End of valid data */
  uint8_t* end;

  /** End of storage allocated for the array */
  uint8_t* eos;

  /** Maximum size of the array */
  size_t maxSize;

  /** Status of the last operation */
  int statusCode;

  /** Message describing the outcome of the last operation */
  const char* statusMsg;
} ArrayImpl;

static const char OK_MSG[] = "OK";
static const char DEFAULT_ERR_MSG[] = "ERROR";

#ifndef __cplusplus
const int ArrayInvalidArgumentError = -1;
const int ArraySequenceTooLongError = -2;
const int ArrayOutOfMemoryError = -3;
#endif

static int increaseArrayStorage(Array array, size_t desiredSize);
static void setArrayStatus(Array array, int code, const char* msg);

static int shouldDeallocateStatusMsg(const char* msg) {
  return msg && (msg != OK_MSG) && (msg != DEFAULT_ERR_MSG);
}

Array createArray(size_t initialSize, size_t maxSize) {
  if (!maxSize || (initialSize > maxSize)) {
    return NULL;
  }

  Array array = malloc(sizeof(ArrayImpl));
  if (!array) {
    return NULL;
  }

  array->data = NULL;
  array->end = NULL;
  array->eos = NULL;
  array->maxSize = maxSize;
  array->statusCode = 0;
  array->statusMsg = OK_MSG;

  if (initialSize) {
    if (increaseArrayStorage(array, initialSize)) {
      free(array);
      return NULL;
    }
    array->end = array->data + initialSize;
  }

  return array;
}

void destroyArray(Array array) {
  if (array) {
    if (array->data) {
      free(array->data);
    }
    if (shouldDeallocateStatusMsg(array->statusMsg)) {
      free((void*)array->statusMsg);
    }
    free(array);
  }
}

int getArrayStatus(Array array) {
  return array->statusCode;
}

const char* getArrayStatusMsg(Array array) {
  return array->statusMsg;
}

static void setArrayStatus(Array array, int code, const char* msg) {
  if (shouldDeallocateStatusMsg(array->statusMsg)) {
    free((void*)array->statusMsg);
  }

  array->statusMsg = malloc(strlen(msg) + 1);
  if (!array->statusMsg) {
    array->statusMsg = DEFAULT_ERR_MSG;
  } else {
    strcpy((char*)array->statusMsg, msg);
  }
  array->statusCode = code;
}

void clearArrayStatus(Array array) {
  if (shouldDeallocateStatusMsg(array->statusMsg)) {
    free((void*)array->statusMsg);
  }
  array->statusCode = 0;
  array->statusMsg = OK_MSG;
}

size_t arraySize(Array array) {
  return array->end - array->data;
}

size_t arrayMaxSize(Array array) {
  return array->maxSize;
}

size_t arrayCapacity(Array array) {
  return array->eos - array->data;
}

uint8_t* startOfArray(Array array) {
  return array->data;
}

uint8_t* endOfArray(Array array) {
  return array->end;
}

uint8_t* ptrToArrayIndex(Array array, size_t index) {
  uint8_t* p = array->data + index;
  return ((p >= array->data) && (p < array->end)) ? p : NULL;
}

uint8_t valueAtArrayIndex(Array array, size_t index) {
  uint8_t* p = ptrToArrayIndex(array, index);
  if (!p) {
    setArrayStatus(array, ArrayInvalidArgumentError, "Index out of range");
    return 0;
  }
  clearArrayStatus(array);
  return *p;
}

uint8_t* findValueInArray(Array array, size_t start, size_t end,
			  uint8_t value) {
  size_t sz = arraySize(array);
  if ((end <= start) || (start >= sz)) {
    return NULL;
  }
  if (end > arraySize(array)) {
    end = sz;
  }
  return (uint8_t*)memchr(array->data + start, value, end - start);
}

uint8_t* findSeqInArray(Array array, size_t start, size_t end,
			const uint8_t* seq, size_t size) {
  size_t sz = arraySize(array);
  if (end > sz) {
    end = sz;
  }
  if (!size || (end <= start) || (start >= sz) || (size > (end - start))) {
    return NULL;
  }

  uint8_t* const endOfRange = array->data + end - size + 1;
  for (uint8_t* p = array->data + start; p != endOfRange; ++p) {
    if (!memcmp(p, seq, size)) {
      return p;
    }
  }
  return NULL;
}

int appendToArray(Array array, const uint8_t* seq, size_t size) {
  if (!size) {
    return 0;
  }

  if (!seq) {
    setArrayStatus(array, ArrayInvalidArgumentError, "\"seq\" is NULL");
    return -1;
  }

  size_t sz = arraySize(array);
  size_t newSize = sz + size;
  if ((newSize < sz) || ((sz + size) > array->maxSize)) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Appending %" PRIu64 " bytes to an array of %" PRIu64
	     " bytes would exceed the array's maximum size of %" PRIu64
	     " bytes", (uint64_t)size, (uint64_t)sz, (uint64_t)array->maxSize);
    setArrayStatus(array, ArraySequenceTooLongError, msg);
    return -1;
  }

  if (newSize > (array->eos - array->data)) {
    if (increaseArrayStorage(array, newSize)) {
      return -1;
    }
  }

  uint8_t* p = array->end;
  array->end += size;
  assert(array->end <= array->eos);
  memcpy(p, seq, size);

  clearArrayStatus(array);
  return 0;
}

int insertIntoArray(Array array, size_t location, const uint8_t* seq,
		    size_t size) {
  size_t sz = arraySize(array);
  if (location == sz) {
    return appendToArray(array, seq, size);
  } else if (location > sz) {
    setArrayStatus(array, ArrayInvalidArgumentError,
		   "\"location\" is outside the array");
    return -1;
  }

  if (!size) {
    return 0;
  }

  if (!seq) {
    setArrayStatus(array, ArrayInvalidArgumentError, "\"data\" is NULL");
    return -1;
  }

  size_t newSize = sz + size;
  if ((newSize < sz) || (newSize > array->maxSize)) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Inserting %" PRIu64 " bytes into an array of %" PRIu64
	     " bytes will exceed the array's maximum size of %" PRIu64
	     " bytes", (uint64_t)size, (uint64_t)sz, (uint64_t)array->maxSize); 
    setArrayStatus(array, ArraySequenceTooLongError, msg);
    return -1;
  }

  if (newSize > (array->eos - array->data)) {
    if (increaseArrayStorage(array, newSize)) {
      return -1;
    }
  }

  uint8_t* p = array->data + location;
  assert((array->end + size) <= array->eos);
  
  memmove(p + size, p, sz - location);
  memcpy(p, seq, size);
  array->end += size;

  clearArrayStatus(array);
  return 0;
}

int removeFromArray(Array array, size_t location, size_t size) {
  size_t sz = arraySize(array);
  
  if (location > sz) {
    setArrayStatus(array, ArrayInvalidArgumentError,
		   "\"location\" is outside the array");
    return -1;
  }

  if ((location + size) > sz) {
    size = sz - location;
  }

  if (!size) {
    return 0;
  }

  if ((location + size) < sz) {
    memmove(array->data + location, array->data + location + size,
	    sz - location - size);
  }
  array->end -= size;
  assert(array->end >= array->data);

  clearArrayStatus(array);
  return 0;
}

int clearArray(Array array) {
  array->end = array->data;
  clearArrayStatus(array);
  return 0;
}

int fillArray(Array array, size_t start, size_t end, uint8_t value) {
  size_t sz = arraySize(array);
  
  if (end < start) {
    setArrayStatus(array, ArrayInvalidArgumentError, "\"end\" < \"start\"");
    return -1;
  }

  if (start > sz) {
    setArrayStatus(array, ArrayInvalidArgumentError,
		   "\"start\" is outside the array");
    return -1;
  }

  if (end > sz) {
    setArrayStatus(array, ArrayInvalidArgumentError,
		   "\"end\" is outside the array");
    return -1;
  }

  assert((start < sz) || (end == start));
  
  if (end == start) {
    return 0;
  }

  memset(array->data + start, value, end - start);

  clearArrayStatus(array);
  return 0;
}

static size_t nextSizeIncrement(size_t currentSize, size_t maxSize) {
  size_t nextSize = (currentSize < 16) ? 16 : currentSize * 2;
  return ((nextSize > currentSize) && (nextSize < maxSize)) ? nextSize
                                                            : maxSize;
}

static int increaseArrayStorage(Array array, size_t desiredSize) {
  size_t currentSize = array->eos - array->data;
  size_t nextSize = nextSizeIncrement(currentSize, array->maxSize);

  /** Caller should make sure this is true */
  assert(desiredSize <= array->maxSize);

  while (nextSize < desiredSize) {
    nextSize = nextSizeIncrement(nextSize, array->maxSize);
  }

  assert(nextSize <= array->maxSize);
  assert(nextSize >= desiredSize);

  uint8_t* newData = (uint8_t*)realloc(array->data, nextSize);
  if (!newData) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Could not allocate %" PRIu64
	     " bytes to increase the array's size", (uint64_t)nextSize);
    setArrayStatus(array, ArrayOutOfMemoryError, msg);
    return -1;
  }

  size_t sz = arraySize(array);

  array->data = newData;
  array->end = newData + sz;
  array->eos = newData + nextSize;

  return 0;
}
