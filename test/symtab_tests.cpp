extern "C" {
#include <symtab.h>
}

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>

namespace {
  struct DebugSymbol {
    std::string name;
    uint64_t address;

    DebugSymbol() : name(), address() { }
    DebugSymbol(const std::string& n, uint64_t a) : name(n), address(a) { }
    DebugSymbol(const DebugSymbol&) = default;
    DebugSymbol(DebugSymbol&&) = default;

    bool operator==(const DebugSymbol& other) const {
      return (name == other.name) && (address == other.address);
    }

    bool operator!=(const DebugSymbol& other) const {
      return (name != other.name) || (address != other.address);
    }
  };

  std::ostream& operator<<(std::ostream& out, const DebugSymbol& s) {
    return out << "(" << s.name << ", " << s.address << ")";
  }

  std::vector<DebugSymbol> makeDebugSymbolList(SymbolTable symtab) {
    std::vector<DebugSymbol> symbols;
    SymbolIterator i = startOfSymbolTable(symtab);

    while (i) {
      symbols.emplace_back((*i)->name, (*i)->address);
      i = nextSymbolInTable(symtab, i);
    }

    return std::move(symbols);
  }

  ::testing::AssertionResult verifySymbolList(
      const std::vector<DebugSymbol>& symbols,
      const std::vector<DebugSymbol>& truth
  ) {
    auto i = symbols.begin();
    auto j = truth.begin();
    uint32_t ndx = 0;

    while ((i != symbols.end()) && (j != truth.end())) {
      if (*i != *j) {
	return ::testing::AssertionFailure()
	  << "Symbols at index " << ndx << " differ: " << *i << " vs. "
	  << *j;
      }
      ++i;
      ++j;
      ++ndx;
    }

    if (i != symbols.end()) {
      std::ostringstream msg;
      msg << *i;
      while (++i != symbols.end()) {
	msg << ", " << *i;
      }
      return ::testing::AssertionFailure()
	<< "Value has extra symbols: [ " << msg.str() << " ]";
    }

    if (j != truth.end()) {
      std::ostringstream msg;
      msg << *j;
      while (++j != truth.end()) {
	msg << ", " << *j;
      }
      return ::testing::AssertionFailure()
	<< "Value is missing symbols: [ " << msg.str() << " ]";
    }

    return ::testing::AssertionSuccess();
  }
}

TEST(stack_tests, createSymbolTable) {
  SymbolTable symtab = createSymbolTable(24);

  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  EXPECT_EQ(symbolTableSize(symtab), 0);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  destroySymbolTable(symtab);
}

TEST(stack_tests, addSymbols) {
  SymbolTable symtab = createSymbolTable(24);

  EXPECT_EQ(addSymbolToTable(symtab, "A", 3), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  EXPECT_EQ(symbolTableSize(symtab), 1);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);
  
  EXPECT_EQ(addSymbolToTable(symtab, "B", 2), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  EXPECT_EQ(symbolTableSize(symtab), 2);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_EQ(addSymbolToTable(symtab, "C", 4), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  EXPECT_EQ(symbolTableSize(symtab), 3);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  SymbolIterator i = startOfSymbolTable(symtab);
  ASSERT_NE(i, (void*)0);
  EXPECT_EQ(std::string((*i)->name), "B");
  EXPECT_EQ((*i)->address, 2);

  i = nextSymbolInTable(symtab, i);
  ASSERT_NE(i, (void*)0);
  EXPECT_EQ(std::string((*i)->name), "A");
  EXPECT_EQ((*i)->address, 3);

  i = nextSymbolInTable(symtab, i);
  ASSERT_NE(i, (void*)0);
  EXPECT_EQ(std::string((*i)->name), "C");
  EXPECT_EQ((*i)->address, 4);

  i = nextSymbolInTable(symtab, i);
  EXPECT_EQ(i, (void*)0);

  destroySymbolTable(symtab);
}

TEST(stack_tests, addDuplicateSymbols) {
  SymbolTable symtab = createSymbolTable(24);

  EXPECT_EQ(addSymbolToTable(symtab, "A", 3), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  EXPECT_EQ(symbolTableSize(symtab), 1);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);
  
  EXPECT_EQ(addSymbolToTable(symtab, "B", 2), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  EXPECT_EQ(symbolTableSize(symtab), 2);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_EQ(addSymbolToTable(symtab, "C", 4), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  EXPECT_EQ(symbolTableSize(symtab), 3);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_NE(addSymbolToTable(symtab, "B", 99), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), SymbolExistsError);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)),
	    "Symbol with name \"B\" already exists");

  EXPECT_EQ(symbolTableSize(symtab), 3);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_NE(addSymbolToTable(symtab, "D", 3), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), SymbolAtThatAddressError);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)),
	    "Symbol with name \"A\" already maps to address 0x3");
  
  EXPECT_EQ(symbolTableSize(symtab), 3);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  destroySymbolTable(symtab);
}

TEST(stack_tests, findSymbols) {
  SymbolTable symtab = createSymbolTable(32);
  const std::vector<DebugSymbol> trueSymbolList{
    { "0", 48 }, { "A", 65 }, { "B", 66 }, { "C", 67 }, { "R", 82 }, { "S", 83 }
  };

  ASSERT_EQ(symbolTableSize(symtab), 0);
  ASSERT_EQ(numSymbolTableBuckets(symtab), 17);
  
  // The hash codes for "0", "A", "R" are 17 apart, so they should hash to
  // the same bucket
  EXPECT_EQ(addSymbolToTable(symtab, "0", 48), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "A", 65), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "R", 82), 0);

  // The hash codes for "B" and "S" are also 17 apart, so they should also
  // hash to the same bucket
  EXPECT_EQ(addSymbolToTable(symtab, "B", 66), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "S", 83), 0);

  // "C" will go in its own bucket
  EXPECT_EQ(addSymbolToTable(symtab, "C", 67), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");

  EXPECT_EQ(symbolTableSize(symtab), 6);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), trueSymbolList));

  // First symbol in its chain
  const Symbol* s = findSymbol(symtab, "0");
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "0");
  EXPECT_EQ(s->address, 48);

  s = findSymbol(symtab, "B");
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "B");
  EXPECT_EQ(s->address, 66);

  s = findSymbol(symtab, "C");
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "C");
  EXPECT_EQ(s->address, 67);

  // Symbol in the middle of its chain
  s = findSymbol(symtab, "A");
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "A");
  EXPECT_EQ(s->address, 65);

  // Symbol at the end of its chain
  s = findSymbol(symtab, "R");
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "R");
  EXPECT_EQ(s->address, 82);

  s = findSymbol(symtab, "S");
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "S");
  EXPECT_EQ(s->address, 83);


  // Nonexistent symbol that hashes to an occupied bucket
  s = findSymbol(symtab, "1");  // Same bucket as "B" and "S"
  EXPECT_EQ(s, (void*)0);

  // Nonexistent symbol that hashes to an unoccupied bucket
  s = findSymbol(symtab, "D");
  EXPECT_EQ(s, (void*)0);

  destroySymbolTable(symtab);
}

TEST(stack_tests, getSymbolAtAddress) {
  SymbolTable symtab = createSymbolTable(32);
  const std::vector<DebugSymbol> trueSymbolList{
    { "0", 48 }, { "A", 65 }, { "B", 66 }, { "C", 67 }, { "R", 82 }, { "S", 83 }
  };

  ASSERT_EQ(symbolTableSize(symtab), 0);
  ASSERT_EQ(numSymbolTableBuckets(symtab), 17);

  // Retrieving a symbol from an empty table returns NULL
  EXPECT_EQ(getSymbolAtAddress(symtab, 65), (void*)0);

  // Add six items.  They should sort "0", "A", "B", "C", "R", "S"
  EXPECT_EQ(addSymbolToTable(symtab, "0", 48), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "A", 65), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "R", 82), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "B", 66), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "S", 83), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "C", 67), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");

  EXPECT_EQ(symbolTableSize(symtab), 6);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), trueSymbolList));

  // Symbol in middle of table's address range
  const Symbol* s = getSymbolAtAddress(symtab, 67);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "C");
  EXPECT_EQ(s->address, 67);

  // Symbol at highest address
  s = getSymbolAtAddress(symtab, 83);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "S");
  EXPECT_EQ(s->address, 83);

  // Symbol at lowest address
  s = getSymbolAtAddress(symtab, 48);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "0");
  EXPECT_EQ(s->address, 48);

  // Non-existent addresses before the symbol at the lowest address,
  // above the symbol at the highest address and in between the symbols
  // with the lowest and highest addresses
  EXPECT_EQ(getSymbolAtAddress(symtab, 47), (void*)0);
  EXPECT_EQ(getSymbolAtAddress(symtab, 84), (void*)0);
  EXPECT_EQ(getSymbolAtAddress(symtab, 75), (void*)0);

  destroySymbolTable(symtab);
}

TEST(stack_tests, getSymbolBeforeAddress) {
  SymbolTable symtab = createSymbolTable(32);
  const std::vector<DebugSymbol> trueSymbolList{
    { "0", 48 }, { "A", 65 }, { "B", 66 }, { "C", 67 }, { "R", 82 }, { "S", 83 }
  };

  ASSERT_EQ(symbolTableSize(symtab), 0);
  ASSERT_EQ(numSymbolTableBuckets(symtab), 17);

  // Retrieving a symbol from an empty table returns NULL
  EXPECT_EQ(getSymbolBeforeAddress(symtab, 65), (void*)0);

  // Add six items.  They should sort "0", "A", "B", "C", "R", "S"
  EXPECT_EQ(addSymbolToTable(symtab, "0", 48), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "A", 65), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "R", 82), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "B", 66), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "S", 83), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "C", 67), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");

  EXPECT_EQ(symbolTableSize(symtab), 6);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), trueSymbolList));

  // Address in the middle of the table not corresponding to a symbol
  const Symbol* s = getSymbolBeforeAddress(symtab, 68);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "C");
  EXPECT_EQ(s->address, 67);

  // Symbol before highest address
  s = getSymbolBeforeAddress(symtab, 83);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "R");
  EXPECT_EQ(s->address, 82);

  // Symbol before lowest address.  Should be NULL
  s = getSymbolBeforeAddress(symtab, 48);
  EXPECT_EQ(s, (void*)0);

  // Symbol before address in the middle of the table.  Should return the
  // symbol before
  s = getSymbolBeforeAddress(symtab, 65);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "0");
  EXPECT_EQ(s->address, 48);

  // Symbol before address below the lowest in the table.  Should be NULL
  s = getSymbolBeforeAddress(symtab, 47);
  EXPECT_EQ(s, (void*)0);

  // Symbol above highest address.  Should return symbol with highest address
  s = getSymbolBeforeAddress(symtab, 100);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "S");
  EXPECT_EQ(s->address, 83);

  destroySymbolTable(symtab);
}

TEST(stack_tests, getSymbolAfterAddress) {
  SymbolTable symtab = createSymbolTable(32);
  const std::vector<DebugSymbol> trueSymbolList{
    { "0", 48 }, { "A", 65 }, { "B", 66 }, { "C", 67 }, { "R", 82 }, { "S", 83 }
  };

  ASSERT_EQ(symbolTableSize(symtab), 0);
  ASSERT_EQ(numSymbolTableBuckets(symtab), 17);

  // Retrieving a symbol from an empty table returns NULL
  EXPECT_EQ(getSymbolAfterAddress(symtab, 65), (void*)0);

  // Add six items.  They should sort "0", "A", "B", "C", "R", "S"
  EXPECT_EQ(addSymbolToTable(symtab, "0", 48), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "A", 65), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "R", 82), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "B", 66), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "S", 83), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "C", 67), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");

  EXPECT_EQ(symbolTableSize(symtab), 6);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), trueSymbolList));

  // Address in the middle of the table not corresponding to a symbol
  const Symbol* s = getSymbolAfterAddress(symtab, 68);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "R");
  EXPECT_EQ(s->address, 82);

  // Symbol after highest address
  s = getSymbolAfterAddress(symtab, 83);
  EXPECT_EQ(s, (void*)0);

  // Symbol after lowest address.
  s = getSymbolAfterAddress(symtab, 48);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "A");
  EXPECT_EQ(s->address, 65);

  // Symbol after address in the middle of the table.  Should return the
  // symbol after
  s = getSymbolAfterAddress(symtab, 65);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "B");
  EXPECT_EQ(s->address, 66);

  // Symbol after address below the lowest in the table.  Should be return
  // symbol at lowest address
  s = getSymbolAfterAddress(symtab, 47);
  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(std::string(s->name), "0");
  EXPECT_EQ(s->address, 48);

  // Symbol above highest address.  Should return NULL
  s = getSymbolAfterAddress(symtab, 100);
  EXPECT_EQ(s, (void*)0);

  destroySymbolTable(symtab);
}

TEST(stack_tests, clearSymbolTable) {
  SymbolTable symtab = createSymbolTable(32);
  const std::vector<DebugSymbol> trueSymbolList{
    { "0", 48 }, { "A", 65 }, { "B", 66 }, { "C", 67 }, { "R", 82 }, { "S", 83 }
  };
  const std::vector<DebugSymbol> afterClearingSymbols{
    { "A", 97 }, { "R", 114 }
  };
  const std::vector<DebugSymbol> noSymbols{ };

  ASSERT_EQ(symbolTableSize(symtab), 0);
  ASSERT_EQ(numSymbolTableBuckets(symtab), 17);

  // Retrieving a symbol from an empty table returns NULL
  EXPECT_EQ(getSymbolAfterAddress(symtab, 65), (void*)0);

  // Add six items.  They should sort "0", "A", "B", "C", "R", "S"
  EXPECT_EQ(addSymbolToTable(symtab, "0", 48), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "A", 65), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "R", 82), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "B", 66), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "S", 83), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "C", 67), 0);
  EXPECT_EQ(getSymbolTableStatus(symtab), 0);
  EXPECT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");

  EXPECT_EQ(symbolTableSize(symtab), 6);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);

  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), trueSymbolList));

  clearSymbolTable(symtab);
  EXPECT_EQ(symbolTableSize(symtab), 0);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);
  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), noSymbols));

  EXPECT_EQ(addSymbolToTable(symtab, "A", 97), 0);
  EXPECT_EQ(addSymbolToTable(symtab, "R", 114), 0);
  EXPECT_EQ(symbolTableSize(symtab), 2);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 17);
  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab),
			       afterClearingSymbols));

  destroySymbolTable(symtab);
}

static int lexicographicCharacterOrdering(const void* left, const void* right) {
  return (int)(*(const char*)left) - (int)(*(const char*)right);
}

TEST(stack_tests, rehashSymbolTable) {
  // Put 17 symbols into the hash table producing a distribution of chain
  // lengths from 0 to 3.  Then add an 18th symbol, which violates the
  // load constraint and causes an increase in the number of buckets and
  // a rehash.  Check that all symbols can be found in the table.
  SymbolTable symtab = createSymbolTable(19);
  char symbolName[2];
  const char symbolNames[] = "0ARBaSCD3p#b9XmzQT7";

  // Null terminator for one-character symbol names
  symbolName[1] = 0;

  ASSERT_EQ(getSymbolTableStatus(symtab), 0);
  ASSERT_EQ(std::string(getSymbolTableStatusMsg(symtab)), "OK");
  ASSERT_EQ(symbolTableSize(symtab), 0);
  ASSERT_EQ(numSymbolTableBuckets(symtab), 17);
  
  // Add the first 17 symbols to the hash table
  for (int i = 0; i < 17; ++i) {
    symbolName[0] = symbolNames[i];
    ASSERT_EQ(symbolName[1], 0);
    ASSERT_EQ(addSymbolToTable(symtab, symbolName, (uint64_t)symbolNames[i]),
	      0) << "Failed to add symbol \"" << symbolName << "\" ("
		 << std::string(getSymbolTableStatusMsg(symtab)) << ")";
  }

  ASSERT_EQ(symbolTableSize(symtab), 17);
  ASSERT_EQ(numSymbolTableBuckets(symtab), 17);

  // Construct the pre-rehash ground-truth sorted symbol list and verify
  // iterating over the table produces symbols in that order
  char sortedSymbolNames[sizeof(symbolNames)];

  ::memcpy(sortedSymbolNames, symbolNames, 17);
  ::qsort(sortedSymbolNames, 17, 1, lexicographicCharacterOrdering);
  sortedSymbolNames[17] = 0;

  std::vector<DebugSymbol> trueSymbolOrder;
  for (int i = 0; i < 17; ++i) {
    symbolName[0] = sortedSymbolNames[i];
    trueSymbolOrder.emplace_back(symbolName, (uint64_t)sortedSymbolNames[i]);
  }
  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), trueSymbolOrder));

  // Adding one more symbol should force a bucket increase & reheash
  symbolName[0] = symbolNames[17];
  EXPECT_EQ(addSymbolToTable(symtab, symbolName, (uint64_t)symbolNames[17]), 0);
  EXPECT_EQ(symbolTableSize(symtab), 18);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 31);

  // Verify we can find everything
  for (int i = 0; i < 18; ++i) {
    symbolName[0] = symbolNames[i];

    const Symbol* s= findSymbol(symtab, symbolName);
    ASSERT_NE(s, (void*)0) << "Could not find symbol \"" << symbolName
			   << "\" after the rehash";
    EXPECT_EQ(std::string(s->name), symbolName)
      << "After the rehash, a search for \"" << symbolName << "\" returned \""
      << s->name << "\" instead";
    EXPECT_EQ(s->address, (uint64_t)symbolNames[i])
      << "After the rehash, symbol \"" << symbolName << " is at address "
      << s->address << ", but it should be at address "
      << (uint64_t)symbolNames[i] << " instead.";
  }

  // Construct the sorted symbol list and verify iterating over the hash
  // table still produces the correct symbols in the correct order
  ::memcpy(sortedSymbolNames, symbolNames, 18);
  ::qsort(sortedSymbolNames, 18, 1, lexicographicCharacterOrdering);
  sortedSymbolNames[18] = 0;

  trueSymbolOrder.clear();
  for (int i = 0; i < 18; ++i) {
    symbolName[0] = sortedSymbolNames[i];
    trueSymbolOrder.emplace_back(symbolName, (uint64_t)sortedSymbolNames[i]);
  }
  EXPECT_TRUE(verifySymbolList(makeDebugSymbolList(symtab), trueSymbolOrder));
  
  // TODO: Add bucket iterators to the symbol table and verify everything
  // wound up in the correct bucket
  

  // Add one more symbol to verify we can still do so after the rehash
  symbolName[0] = symbolNames[18];
  ASSERT_EQ(addSymbolToTable(symtab, symbolName, (uint64_t)symbolNames[18]), 0);
  EXPECT_EQ(symbolTableSize(symtab), 19);
  EXPECT_EQ(numSymbolTableBuckets(symtab), 31);

  // Verify we can find everything
  for (int i = 0; i < 19; ++i) {
    symbolName[0] = symbolNames[i];
    const Symbol* s = findSymbol(symtab, symbolName);
    EXPECT_NE(s, (void*)0)
      << "Could not find symbol \"" << symbolName << " after adding symbol "
      << "\"" << symbolNames[18] << "\"";
    if (s) {
      EXPECT_EQ(std::string(s->name), symbolName)
	<< "After adding \"" << symbolNames[18] << "\", a search for symbol "
	<< "\"" << symbolName << "\" returned symbol \"" << s->name << "\"";
      EXPECT_EQ(s->address, (uint64_t)symbolNames[i])
	<< "After adding \"" << symbolNames[18] << "\", symbol \""
	<< symbolName << " has address " << s->address << ", but it should "
	<< "have address " << (uint64_t)symbolNames[i];
    }
  }

  // TODO: Verify the buckets are still correct

  destroySymbolTable(symtab);
}
