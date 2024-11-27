#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include <stdint.h>

typedef struct Symbol_ {
  const char* name;
  uint64_t address;
} Symbol;

typedef const Symbol** SymbolIterator;

typedef struct SymbolTableImpl_* SymbolTable;

/** Create a new symbol table
 *  
 *  Arguments:
 *    maxSize   Maximum number of symbols that can be stored in the table
 *
 *  Returns:
 *    A new symbol table, or NULL if the table could not be created.
 */
SymbolTable createSymbolTable(uint32_t maxSize);

/** Destroy an existing symbol table
 *
 *  Frees all memory the table uses.
 *
 *  Arguments:
 *    symtab  The table to destroy
 */
void destroySymbolTable(SymbolTable symtab);

/** Get the status of the last symbol table operation
 *
 *  Note that symbolTableSize(), numSymbolTableBuckets(), findSymbol(),
 *  getSymbolAtAddress(), getSymbolBeforeAddress() and getSymbolAfterAddress()
 *  do not change the symbol table status.
 *
 *  Arguments:
 *    symtab  The table whose status will be inquired
 *
 *  Returns:
 *    0 if the last operation succeeded, or one of the SymbolTable*Error
 *    codes if it failed.
 */
int getSymbolTableStatus(SymbolTable symtab);

/** Get a message describing the outcome of the last operation on this table
 *
 *  Note that symbolTableSize(), numSymbolTableBuckets(), findSymbol(),
 *  getSymbolAtAddress(), getSymbolBeforeAddress() and getSymbolAfterAddress()
 *  do not change the symbol table status.
 *
 *  Arguments:
 *    symtab  The table whose status will be inquired
 *
 *  Returns:
 *    "OK" if the last operation succeeded, or an error message if it failed
 */
const char* getSymbolTableStatusMsg(SymbolTable symtab);

/** Reset the error status of the given symbol table
 *
 *  After calling this function, calling getSymbolTableStatus() on the
 *  given table will return 0 and calling getSymbolTableStatusMsg() will
 *  return "OK"
 */
void clearSymbolTableStatus(SymbolTable symtab);

/** Returns the number of symbols in the given table */
uint32_t symbolTableSize(SymbolTable symtab);

/** Returns the number of buckets the given symbol table uses */
uint32_t numSymbolTableBuckets(SymbolTable symtab);

/** Find a symbol by name
 *
 *  Internally, the symbol table uses a hash table to map names to symbols,
 *  so this operation is O(1) in the size of the symbol table, provided
 *  the name hash function distributes names evenly across the buckets.
 *
 *  Arguments:
 *    symtab  The symbol table to search
 *    name    Find symbol with this name
 *
 *  Returns:
 *    A pointer to the symbol with the given name, or NULL if no symbol
 *    with that name is stored in the table.
 */
const Symbol* findSymbol(SymbolTable symtab, const char* name);

/** Find a symbol by its address
 *
 *  Internally, this uses a binary search to find the address, so ti is
 *  reasonably fast (logarithmic in the number of symbols in the table).
 *
 *  Arguments:
 *    symtab   The symbol table to search
 *    address  The address to find
 *
 *  Returns:
 *    A pointer to the symbol at the given address or NULL if there is
 *    no symbol at the given address.
 */
const Symbol* getSymbolAtAddress(SymbolTable symtab, uint64_t address);

/** Find the first symbol before the given address
 *
 *  Logarithmic in the number of symbols stored in the table.
 *
 *  Arguments
 *    symtab   The symbol table to search
 *    address  The address to find
 *
 *  Returns:
 *    A pointer to the symbol whose address is closest to but less than
 *    "address" or NULL if no such symbol exists.
 */
const Symbol* getSymbolBeforeAddress(SymbolTable symtab, uint64_t address);

/** Find the first symbol after the given address
 *
 *  Logarithmic in the number of symbols stored in the table.
 *
 *  Arguments
 *    symtab   The symbol table to search
 *    address  The address to find
 *
 *  Returns:
 *    A pointer to the symbol whose address is closest to but greater than
 *    "address" or NULL if no such symbol exists.
 */
const Symbol* getSymbolAfterAddress(SymbolTable symtab, uint64_t address);

/** Add a symbol to the table
 *
 *  Symbol names and addresses must be unique, so adding a symbol with the
 *  same name or address as another symbol will generate an error
 *
 *  Arguments:
 *    symtab   The symbol table to add the new symbol to
 *    name     Name of the new symbol
 *    address  Address of the new symbol
 *
 *  Returns:
 *    0 if successful, nonezero if an error occurred.  If an error occurred,
 *    getSymbolTableStatus() and getSymbolTableStatusMsg() will return a
 *    code and a message describing the error.
 */
int addSymbolToTable(SymbolTable symtab, const char* name, uint64_t address);

/** Remove all symbols from the given table, leaving it empty
 *
 *  Does not change the number of allocated hash buckets for the name hash
 *  table, nor release the memory used by the symbol list, but does deallocate
 *  the memory used by the individual symbols.
 *
 *  Arguments:
 *    symtab:  The table to clear
 */
void clearSymbolTable(SymbolTable symtab);

/** Returns an iterator pointing to the first symbol in the table, or NULL
 *  if the table is empty
 *
 *  Symbols are sorted by address, so this iterator will proceed from
 *  symbols at lower addresses to symbols at higher addresses.
 */
SymbolIterator startOfSymbolTable(SymbolTable symtab);

/** Returns an iterator pointing to the symbol after the symbol at "p" in
 *  the given symbol table, or NULL if "p" points to the last symbol.
 */
SymbolIterator nextSymbolInTable(SymbolTable symtab, SymbolIterator p);

#ifdef __cplusplus
/** The function received an invalid argument */
const int SymbolTableInvalidArgumentError = -1;

/** The symbol table attempted to allocate more memory and failed */
const int SymbolTableAllocationFailedError = -2;

/** The caller attempted to add a symbol with the same name as another symbol
 *  already in the table.
 */
const int SymbolExistsError = -3;

/** The caller attempted to add a symbol at the same address as another symbol
 *  already in the table.
 */
const int SymbolAtThatAddressError = -4;

/** The symbol table has reached its maximum size */
const int SymbolTableFullError = -5;

#else
const int SymbolTableInvalidArgumentError;
const int SymbolTableAllocationFailedError;
const int SymbolExistsError;
const int SymbolAtThatAddressError;
const int SymbolTableFullError;
#endif

#endif
