#include "symtab.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SymbolTableLink_ {
  Symbol symbol;
  struct SymbolTableLink_* next;
} SymbolTableLink;

typedef SymbolTableLink* SymbolTableBucket;

typedef struct SymbolTableImpl_ {
  /** Maximum number of symbols in the table */
  uint32_t maxSize;

  /** Number of symbols currently in the table */
  uint32_t numSymbols;

  /** Number of buckets in the hash table */
  uint32_t numBuckets;

  /** Index into HASH_TABLE_NUM_BUCKETS corresponding to numBuckets */
  uint32_t numBucketsIndex;

  /** Hash table from name to symbol */
  SymbolTableBucket* buckets;

  /** Array of symbols sorted by address */
  Symbol** symbols;

  /** End of storage allocated for the symbol list */
  Symbol** endOfSymbols;

  /** Result code for last operation.  0 == no error */
  int statusCode;

  /** Message describing outcome of last operator.  "OK" == no error */
  const char* statusMsg;
} SymbolTableImpl;

static const char OK_MSG[] = "OK";
static const char DEFAULT_ERR_MSG[] = "ERROR";
static const uint32_t INITIAL_LIST_SIZE = 16;
static const float MAX_LOAD = 1.0;

static const uint32_t HASH_TABLE_NUM_BUCKETS[] = {
    17, 31, 61, 127, 257, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65537,
    131071, 262147, 524287, 1048573, 2097143, 4194301, 8388617, 16777213,
    33554467, 67108859, 134217757, 268435459, 536870909, 1073741827,
    2147483647, 4294967291
};

#ifndef __cplusplus
const int SymbolTableInvalidArgumentError = -1;
const int SymbolTableAllocationFailedError = -2;
const int SymbolExistsError = -3;
const int SymbolAtThatAddressError = -4;
const int SymbolTableFullError = -5;
#endif

static const uint32_t NUM_HASH_TABLE_BUCKET_SIZES =
  sizeof(HASH_TABLE_NUM_BUCKETS) / sizeof(uint32_t);

static void setSymbolTableStatus(SymbolTable symtab, int statusCode,
				 const char* statusMsg);
static void increaseSymbolTableBuckets(SymbolTable symtab);
static void putLinkIntoBucket(SymbolTableBucket* buckets, uint32_t numBuckets,
			      SymbolTableLink* link);
static int64_t findSymbolByAddress(SymbolTable symtab, uint64_t target);
static int findSymbolByName(SymbolTable symtab, const char* name,
			    const uint32_t hashCode,
			    SymbolTableLink** symbol);
static uint32_t hashName(const char* name);
static int insertIntoAddressList(SymbolTable symtab, Symbol* symbol,
				 int64_t index);
static int increaseAddressListSize(SymbolTable symtab);
  
SymbolTable createSymbolTable(uint32_t maxSize) {
  SymbolTable symtab = (SymbolTable)malloc(sizeof(SymbolTableImpl));

  if (!symtab) {
    return NULL;
  }

  symtab->maxSize = maxSize;
  symtab->numSymbols = 0;
  symtab->numBuckets = HASH_TABLE_NUM_BUCKETS[0];
  symtab->numBucketsIndex = 0;
  symtab->buckets = (SymbolTableBucket*)malloc(
      symtab->numBuckets * sizeof(SymbolTableBucket)
  );
  if (!symtab->buckets) {
    free((void*)symtab);
    return NULL;
  }
  memset(symtab->buckets, 0, symtab->numBuckets * sizeof(SymbolTableBucket));
  
  symtab->symbols = (Symbol**)malloc(INITIAL_LIST_SIZE * sizeof(Symbol*));
  if (!symtab->symbols) {
    free((void*)symtab->buckets);
    free((void*)symtab);
    return NULL;
  }
  symtab->endOfSymbols = symtab->symbols + INITIAL_LIST_SIZE;
  symtab->statusCode = 0;
  symtab->statusMsg = OK_MSG;
  return symtab;
}

void destroySymbolTable(SymbolTable symtab) {
  clearSymbolTable(symtab);
  clearSymbolTableStatus(symtab);
  free((void*)symtab->buckets);
  free((void*)symtab->symbols);
  free((void*)symtab);
}

int getSymbolTableStatus(SymbolTable symtab) {
  return symtab->statusCode;
}

const char* getSymbolTableStatusMsg(SymbolTable symtab) {
  return symtab->statusMsg;
}

void clearSymbolTableStatus(SymbolTable symtab) {
  if (symtab->statusMsg && (symtab->statusMsg != OK_MSG)
         && (symtab->statusMsg != DEFAULT_ERR_MSG)) {
    free((void*)symtab->statusMsg);
  }
  symtab->statusCode = 0;
  symtab->statusMsg = OK_MSG;
}

static void setSymbolTableStatus(SymbolTable symtab, int statusCode,
				 const char* statusMsg) {
  if (symtab->statusMsg && (symtab->statusMsg != OK_MSG)
         && (symtab->statusMsg != DEFAULT_ERR_MSG)) {
    free((void*)symtab->statusMsg);
  }
  symtab->statusCode = statusCode;
  if ((statusMsg == OK_MSG) || (statusMsg == DEFAULT_ERR_MSG)) {
    symtab->statusMsg = statusMsg;
  } else {
    symtab->statusMsg = (const char*)malloc(strlen(statusMsg) + 1);
    if (symtab->statusMsg) {
      strcpy((char*)symtab->statusMsg, statusMsg);
    } else {
      symtab->statusMsg = DEFAULT_ERR_MSG;
    }
  }
}

uint32_t symbolTableSize(SymbolTable symtab) {
  return symtab->numSymbols;
}

uint32_t numSymbolTableBuckets(SymbolTable symtab) {
  return symtab->numBuckets;
}

const Symbol* findSymbol(SymbolTable symtab, const char* name) {
  SymbolTableLink* link = NULL;
  return findSymbolByName(symtab, name, hashName(name), &link)
             ? &(link->symbol) : NULL;
}

const Symbol* getSymbolAtAddress(SymbolTable symtab, uint64_t address) {
  int64_t i = findSymbolByAddress(symtab, address);
  return i >= 0 ? symtab->symbols[i] : NULL;
}

const Symbol* getSymbolBeforeAddress(SymbolTable symtab, uint64_t address) {
  int64_t i = findSymbolByAddress(symtab, address);
  if (i < 0) {
    i = -i - 1;
  }
  return i > 0 ? symtab->symbols[i - 1] : NULL;
}

const Symbol* getSymbolAtOrBeforeAddress(SymbolTable symtab, uint64_t address) {
  int64_t i = findSymbolByAddress(symtab, address);
  if (i >= 0) {
    return symtab->symbols[i];
  } else if (-i > 1) {
    return symtab->symbols[-i - 2];
  } else {
    return NULL;
  }
}

const Symbol* getSymbolAfterAddress(SymbolTable symtab, uint64_t address) {
  int64_t i = findSymbolByAddress(symtab, address);
  if (i < 0) {
    i = -i - 1;
  } else {
    ++i;
  }
  return (i < symtab->numSymbols) ? symtab->symbols[i] : NULL;
}

const Symbol* getSymbolAtOrAfterAddress(SymbolTable symtab, uint64_t address) {
  int64_t i = findSymbolByAddress(symtab, address);
  if (i >= 0) {
    return symtab->symbols[i];
  } else {
    i = -i - 1;
    return (i < symtab->numSymbols) ? symtab->symbols[i] : NULL;
  }
}

int addSymbolToTable(SymbolTable symtab, const char* name, uint64_t address) {
  const uint32_t hashCode = hashName(name);
  SymbolTableLink* p = NULL;

  clearSymbolTableStatus(symtab);

  if (findSymbolByName(symtab, name, hashCode, &p)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Symbol with name \"%s\" already exists", name);
    setSymbolTableStatus(symtab, SymbolExistsError, msg);
    return -1;
  }

  int64_t symIndex = findSymbolByAddress(symtab, address);
  if (symIndex >= 0) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Symbol with name \"%s\" already maps to address 0x%lx",
	     symtab->symbols[symIndex]->name, address);
    setSymbolTableStatus(symtab, SymbolAtThatAddressError, msg);
    return -1;
  }

  symIndex = - symIndex - 1;

  if (symtab->numSymbols >= symtab->maxSize) {
    setSymbolTableStatus(symtab, SymbolTableFullError, "Symbol table is full");
    return -1;
  }

  const float load = (float)(symtab->numSymbols + 1)/(float)symtab->numBuckets;
  if (((float)(symtab->numSymbols + 1)/(float)symtab->numBuckets) > MAX_LOAD) {
    increaseSymbolTableBuckets(symtab);
    /** Need to repeat this, because bucket where new symbol goes has
     *  probably changed.  If this actually finds the symbol, something
     *  bad has happened b/c the check above should have caught this, and
     *  no new symbols were added to the table during the rehash.
     */
    assert(!findSymbolByName(symtab, name, hashCode, &p));
  }
  
  SymbolTableLink* link = (SymbolTableLink*)malloc(sizeof(SymbolTableLink));
  if (!link) {
    setSymbolTableStatus(symtab, SymbolTableAllocationFailedError,
			 "Could not allocate new hash table link");
    return -1;
  }
  link->symbol.name = strdup(name);
  link->symbol.address = address;
  link->next = NULL;

  if (insertIntoAddressList(symtab, &(link->symbol), symIndex)) {
    free((void*)link->symbol.name);
    free((void*)link);
    return -1;
  }

  if (p) {
    assert(!p->next);
    p->next = link;
  } else {
    const uint32_t bucketIndex = hashCode % symtab->numBuckets;
    assert(!symtab->buckets[bucketIndex]);
    symtab->buckets[bucketIndex] = link;
  }

  ++symtab->numSymbols;
  return 0;
}

void clearSymbolTable(SymbolTable symtab) {
  SymbolTableLink** const endOfBuckets = symtab->buckets + symtab->numBuckets;

  /** Destroy all the links in the hash table and set the bucket entries
   *  to NULL
   */
  for (SymbolTableLink** p = symtab->buckets; p != endOfBuckets; ++p) {
    if (*p) {
      while ((*p)->next) {
	const SymbolTableLink* next = (*p)->next;
	(*p)->next = next->next;
	free((void*)next->symbol.name);
	free((void*)next);
      }
      free((void*)((*p)->symbol.name));
      free((void*)(*p));
      *p = NULL;
    }
  }

  /** Setting numSymbols to zero truncates the symbol list */
  symtab->numSymbols = 0;
}

SymbolIterator startOfSymbolTable(SymbolTable symtab) {
  return symtab->numSymbols ? (SymbolIterator)symtab->symbols : NULL;
}

SymbolIterator nextSymbolInTable(SymbolTable symtab, SymbolIterator p) {
  const Symbol** const end =
    (const Symbol**)symtab->symbols + symtab->numSymbols;
  ++p;
  return (p < end) ? p : NULL;
}

static void increaseSymbolTableBuckets(SymbolTable symtab) {
  uint32_t newNumBucketsIndex = symtab->numBucketsIndex + 1;

  if (newNumBucketsIndex == NUM_HASH_TABLE_BUCKET_SIZES) {
    /** Reached maximum number of hash table buckets */
    return;
  }

  /** Each increment in HASH_TABLE_NUM_BUCKETS approximately doubles
   *  the number of buckets, the load factor goes down by about half.
   *  Since increaseSymbolTableBuckets fires when adding a new symbol
   *  increases the load just past MAX_LOAD, approximately halving
   *  the current load should put the table below MAX_LOAD unless the
   *  number of buckets has reached its limit.
   */
  const uint32_t newNumBuckets = HASH_TABLE_NUM_BUCKETS[newNumBucketsIndex];
  SymbolTableBucket* newBuckets = (SymbolTableBucket*)malloc(
      newNumBuckets * sizeof(SymbolTableBucket)
  );

  if (!newBuckets) {
    return;  /** Can't allocate space for more buckets */
  }

  memset(newBuckets, 0, newNumBuckets * sizeof(SymbolTableBucket));

  /** Rehash all the links */
  SymbolTableBucket* endOfBuckets = symtab->buckets + symtab->numBuckets;
  for (SymbolTableBucket* p = symtab->buckets; p != endOfBuckets; ++p) {
    if (*p) {
      SymbolTableLink* q= *p;
      while (q) {
	/** This whill change after it is put into its new bucket */
	SymbolTableLink* next = q->next;
	putLinkIntoBucket(newBuckets, newNumBuckets, q);
	q = next;
      }
    }
  }

  /** Free the old table */
  free((void*)symtab->buckets);
  symtab->buckets = newBuckets;
  symtab->numBuckets = newNumBuckets;
  symtab->numBucketsIndex = newNumBucketsIndex;
}

static void putLinkIntoBucket(SymbolTableBucket* buckets, uint32_t numBuckets,
			      SymbolTableLink* link) {
  uint32_t index = hashName(link->symbol.name) % numBuckets;
  link->next = buckets[index];
  buckets[index] = link;
}

static int64_t findSymbolByAddress(SymbolTable symtab, uint64_t target) {
  int64_t i = 0;
  int64_t j = symtab->numSymbols;

  while (i < j) {
    /** Safe b/c maximum symbol table size is 2^32 - 1 */
    int64_t k = (i + j) >> 1;
    if (symtab->symbols[k]->address < target) {
      i = k + 1;
    } else if (target < symtab->symbols[k]->address) {
      j = k;
    } else {
      return k;
    }
  }

  assert(i == j);
  return -i - 1;
}

static int findSymbolByName(SymbolTable symtab, const char* name,
			    const uint32_t hashCode,
			    SymbolTableLink** symbol) {
  const uint32_t bucket = hashCode % symtab->numBuckets;
  SymbolTableLink* p = symtab->buckets[bucket];
  if (!p) {
    *symbol = NULL;
    return 0;
  } else {
    while (p->next) {
      if (!strcmp(name, p->symbol.name)) {
	*symbol = p;
	return -1;
      }
      p = p->next;
    }
    *symbol = p;
    return !strcmp(name, p->symbol.name);
  }
}

static uint32_t hashName(const char* name) {
  uint32_t code = 0;
  for (const char* p = name; *p; ++p) {
    code = 31 * code + (*p);
  }
  return code;
}

static int insertIntoAddressList(SymbolTable symtab, Symbol* symbol,
				 int64_t index) {
  Symbol** end = symtab->symbols + symtab->numSymbols;

  if (end == symtab->endOfSymbols) {
    if (increaseAddressListSize(symtab)) {
      return -1;
    }
    /** Symbol list might have moved in memory */
    end = symtab->symbols + symtab->numSymbols;
  }

  if (index < symtab->numSymbols) {
    memmove(symtab->symbols + index + 1, symtab->symbols + index,
	    (symtab->numSymbols - index) * sizeof(Symbol*));
  }
  symtab->symbols[index] = symbol;
  return 0;
}

static int increaseAddressListSize(SymbolTable symtab) {
  const int64_t numAllocated = symtab->endOfSymbols - symtab->symbols;

  if (numAllocated < symtab->maxSize) {
    int64_t newSize = numAllocated * 2;
    if (newSize > symtab->maxSize) {
      newSize = symtab->maxSize;
    }

    Symbol** newList = (Symbol**)realloc((void*)symtab->symbols,
					 newSize * sizeof(Symbol*));
    if (!newList) {
      setSymbolTableStatus(symtab, SymbolTableAllocationFailedError,
			   "Failed to increase size of symbol list");
      return -1;
    }

    //memcpy(newList, symtab->symbols, numAllocated * sizeof(Symbol*));
    symtab->symbols = newList;
    symtab->endOfSymbols = newList + newSize;
  }

  return 0;
}
