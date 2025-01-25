#include "vm_image.h"
#include "fileio.h"
#include "symtab.h"
#include "vmmem.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/** A program image file looks like this:
 *  program-image := header program symbols?
 *  header := magic-number program-size num-symbols start-address header-pad 
 *  magic-number := "MOO4COWS"
 *  program-size := uint32_t  // Program size in bytes
 *  symbols-size := uint32_t  // Number of symbols in symbol table
 *  start-address := uint32_t // Where to begin program execution
 *  header-pad := int8[4]     // Padding to 24 bytes
 *  program := uint8_t+       // $program_size bytes
 *  symbols := symbol+        // $num_symbols symbol instances
 *  symbol := length address name 
 *  length := uint8_t         // = len(name) + 8 bytes for address
 *  address := uint64_t       // Location of symbol in VM memory
 *  name := char+             // $length - 8 characters
 */
const int VmImageIllegalArgumentError = -1;
const int VmImageProgramAlreadyLoadedError = -2;
const int VmImageIOError = -3;
const int VmImageFormatError = -4;
const int VmImageOutOfMemoryError = -5;

static int readHeader(const char* filename, int fd, uint32_t* programSize,
		      uint32_t* numSymbols, uint32_t* startAddress,
		      const char** errMsg);
static int writeHeader(const char* filename, int fd, uint32_t programSize,
		       uint32_t numSymbols, uint32_t startAddress,
		       const char** errMsg);
static int loadSymbolsFromFile(const char* filename, int fd,
			       uint32_t numSymbols, SymbolTable symtab,
			       const char** errMsg);
static int loadSymbols(int fd, uint32_t numSymbols, SymbolTable symtab,
		       const char** errMsg);
static int saveSymbols(const char* filename, int fd, SymbolTable symtab,
		       const char** errMsg);

int loadVmProgramHeader(const char* filename, uint64_t* programSize,
			uint64_t* numSymbols, uint64_t* startAddress,
			const char** errMsg) {
  int fd = openFile(filename, O_RDONLY, 0, errMsg);
  if (fd < 0) {
    return VmImageIOError;
  }

  uint32_t ps, ns, sa;
  int result = readHeader(filename, fd, &ps, &ns, &sa, errMsg);
  if (!result) {
    *programSize = ps;
    *numSymbols = ns;
    *startAddress = sa;
  }

  close(fd);
  return result;
}

int loadVmProgramImage(const char* filename, UnlambdaVM vm,
		       int loadSymbols, uint64_t* startAddress,
		       const char** errMsg) {
  *errMsg = NULL;
  
  VmMemory memory = getVmMemory(vm);
  int fd = openFile(filename, O_RDONLY, 0, errMsg);
  uint32_t programSize = 0, numSymbols = 0, sa = 0;
  int result = 0;
  
  if (fd < 0) {
    return VmImageIOError;
  }

  result = readHeader(filename, fd, &programSize, &numSymbols, &sa, errMsg);
  if (result) {
    close(fd);
    return result;
  }

  if (reserveVmMemoryForProgram(memory, programSize)) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Cannot load a program of %u bytes into a memory of %" PRIu64
	     " bytes", programSize, currentVmmSize(memory));
    *errMsg = strdup(msg);
    close(fd);
    return VmImageOutOfMemoryError;
  }

  /** Read the program code */
  if (readFromFile(filename, fd, (void*)getProgramStartInVmm(memory),
		   programSize, errMsg)) {
    close(fd);
    return VmImageIOError;
  }

  /** Load the symbols, if requested */
  if (loadSymbols) {
    result = loadSymbolsFromFile(filename, fd, numSymbols,
				 getVmSymbolTable(vm), errMsg);
  } else {
    result = 0;
  }

  *startAddress = sa;
  close(fd);
  return result;
}

int saveVmProgramImage(const char* filename, const uint8_t* program,
		       uint64_t programSize, uint64_t startAddress,
		       SymbolTable symtab, const char** errMsg) {
  *errMsg = NULL;

  if (programSize > 0xFFFFFFFF) {
    *errMsg = strdup("Maximum program size is 4g");
    return -1;
  }
  
  if (startAddress >= programSize) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Program start (%" PRIu64 ") lies outside "
	     "the program (ends before %" PRIu64 ")", startAddress,
	     programSize);
    *errMsg = strdup(msg);
    return VmImageIllegalArgumentError;
  }
  
  int fd = openFile(filename, O_WRONLY|O_CREAT, 0666, errMsg);
  if (fd < 0) {
    return VmImageIllegalArgumentError;
  }

  uint32_t numSymbols = symtab ? symbolTableSize(symtab) : 0;
  int result = writeHeader(filename, fd, programSize, numSymbols, startAddress,
			   errMsg);
  if (result) {
    close(fd);
    return result;
  }

  if (writeToFile(filename, fd, (const void*)program, programSize, errMsg)) {
    close(fd);
    return -1;
  }

  if (symtab) {
    result = saveSymbols(filename, fd, symtab, errMsg);
  } else {
    result = 0;
  }

  close(fd);
  return result;
}


static int readHeader(const char* filename, int fd, uint32_t* programSize,
		      uint32_t* numSymbols, uint32_t* startAddress,
		      const char** errMsg) {
  uint8_t header[24];

  if (readFromFile(filename, fd, (void*)header, sizeof(header), errMsg)) {
    return VmImageIOError;
  }

  if (strncmp((const char*)header, "MOO4COWS", 8)) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Error reading header from %s: Not an Unlambda VM program image",
	     filename);
    *errMsg = strdup(msg);
    return VmImageFormatError;
  }

  /** TODO: Fix endianness issues */
  *programSize = *(uint32_t*)(header + 8);
  *numSymbols = *(uint32_t*)(header + 12);
  *startAddress = *(uint32_t*)(header + 16);

  return 0;
}

static int writeHeader(const char* filename, int fd, uint32_t programSize,
		       uint32_t numSymbols, uint32_t startAddress,
		       const char** errMsg) {
  uint8_t header[24];
  strcpy((char*)header, "MOO4COWS");
  *(uint32_t*)(header + 8) = programSize;
  *(uint32_t*)(header + 12) = numSymbols;
  *(uint32_t*)(header + 16) = startAddress;
  *(uint32_t*)(header + 20) = 0;

  if (writeToFile(filename, fd, (const void*)header, sizeof(header), errMsg)) {
    return VmImageIOError;
  }

  return 0;
}

static int loadSymbolsFromFile(const char* filename, int fd,
			       uint32_t numSymbols, SymbolTable symtab,
			       const char** errMsg) {
  uint64_t address = 0;
  char buffer[257];
  uint8_t length = 0;

  for (uint32_t symnum = 0; symnum < numSymbols; ++symnum) {
    off_t offset = lseek(fd, 0, SEEK_CUR);

    if (readFromFile(filename, fd, (void*)&length, 1, errMsg)) {
      return VmImageIOError;
    }

    if (readFromFile(filename, fd, (void*)buffer, length, errMsg)) {
      return VmImageIOError;
    }

    buffer[length] = 0;

    if (addSymbolToTable(symtab, (const char*)(buffer + 8),
			 *(uint64_t*)buffer)) {
      char msg[200];
      snprintf(msg, sizeof(msg),
	       "Error reading symbol at offset %zd from %s: Cannot add "
	       "symbol to symbol table (%s)", offset, filename,
	       getSymbolTableStatusMsg(symtab));
      *errMsg = strdup(msg);
      return VmImageFormatError;
    }
  }

  return 0;
}

static const size_t MAX_SYMBOL_NAME_LEN = 247;
static int saveSymbols(const char* filename, int fd, SymbolTable symtab,
		       const char** errMsg) {
  SymbolIterator s = startOfSymbolTable(symtab);
  uint8_t buffer[257];

  while (s) {
    size_t nameLen = strlen((*s)->name);

    if (strlen((*s)->name) > MAX_SYMBOL_NAME_LEN) {
      char msg[200];
      snprintf(msg, sizeof(msg),
	       "Error saving symbol \"%s\" to %s: Name is too long",
	       (*s)->name, filename);
      *errMsg = strdup(msg);
      return VmImageFormatError;
    }

    buffer[0] = (uint8_t)(nameLen + 8);
    *(uint64_t*)(buffer + 1) = (*s)->address;
    strncpy((char*)(buffer + 9), (*s)->name, nameLen);

    if (writeToFile(filename, fd, (const void*)buffer, nameLen + 9, errMsg)) {
      return VmImageIOError;
    }
      
    s = nextSymbolInTable(symtab, s);
  }

  return 0;
}
