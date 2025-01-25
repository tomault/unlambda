extern "C" {
#include <vm.h>
#include <vm_image.h>
#include <vmmem.h>
}

#include <gtest/gtest.h>
#include <testing_utils.hpp>
#include <assert.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <vector>

namespace unl_test = unlambda::testing;

namespace {
  const uint8_t VM_IMAGE_1[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'S' ,
    0x0E, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x08, 0x0F, 0x0A, 0x01, 0xAD, 0xBE, 0xED,
    0xFE, 0xEF, 0xBE, 0xAD, 0xDE, 0x05, 0x0B, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 'C' ,
    'O' , 'W' , 0x0F, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
    0xAA, 0x99, 0x88, 'P' , 'E' , 'N' , 'G' , 'U' ,
    'I' , 'N' ,
  };
  const size_t VM_IMAGE_1_SIZE = sizeof(VM_IMAGE_1);
  const uint64_t VM_IMAGE_1_PROGRAM_SIZE = 14;
  const uint64_t VM_IMAGE_1_NUM_SYMBOLS = 2;
  const uint64_t VM_IMAGE_1_START_ADDRESS = 4;

  const uint8_t BAD_MAGIC[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'Z' ,
    0x0E, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x08, 0x0F, 0x0A, 0x01, 0xAD, 0xBE, 0xED,
    0xFE, 0xEF, 0xBE, 0xAD, 0xDE, 0x05, 0x0B, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 'C' ,
    'O' , 'W' , 0x0F, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
    0xAA, 0x99, 0x88, 'P' , 'E' , 'N' , 'G' , 'U' ,
    'I' , 'N' ,
  };
  const size_t BAD_MAGIC_SIZE = sizeof(BAD_MAGIC);

  ::testing::AssertionResult prepareVmImage(unl_test::TemporaryFile& file,
					    const uint8_t* data,
					    size_t size) {
    ssize_t nWritten = file.write(data, size);
    if (nWritten < 0) {
      return ::testing::AssertionFailure()
	<< "Failed to initialize " << file.name() << ": " << file.lastError();
    } else if (nWritten != size) {
      return ::testing::AssertionFailure()
	<< "Failed to initialize " << file.name() << ": Wrote " << nWritten
	<< "bytes, but needed to write " << size << " bytes";
    }
    return ::testing::AssertionSuccess();
  }
  
  ::testing::AssertionResult prepareVmImage1(unl_test::TemporaryFile& file) {
    return prepareVmImage(file, VM_IMAGE_1, VM_IMAGE_1_SIZE);
  }
}

TEST(vm_image_tests, readImageHeader) {
  unl_test::TemporaryFile testFile;
  uint64_t programSize = 0, numSymbols = 0, startAddress = 0;
  const char* errorMessage = NULL;

  std::cout << "Use temporary file " << testFile.name() << std::endl;
  
  ASSERT_TRUE(prepareVmImage1(testFile));

  if (loadVmProgramHeader(testFile.name().c_str(), &programSize, &numSymbols,
			  &startAddress, &errorMessage)) {
    FAIL() << "Call to loadVmProgramHeader(" << testFile.name() << ") failed: "
	   << (errorMessage ? errorMessage : "NULL");
  }

  EXPECT_EQ(programSize, VM_IMAGE_1_PROGRAM_SIZE);
  EXPECT_EQ(numSymbols, VM_IMAGE_1_NUM_SYMBOLS);
  EXPECT_EQ(startAddress, VM_IMAGE_1_START_ADDRESS);
  EXPECT_EQ(errorMessage, (void*)0);
}

TEST(vm_image_tests, readTooShortImageHeader) {
  static const uint8_t SHORT_IMAGE_HEADER[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'S' ,
    0x0E, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  unl_test::TemporaryFile testFile;

  ASSERT_TRUE(prepareVmImage(testFile, SHORT_IMAGE_HEADER,
			     sizeof(SHORT_IMAGE_HEADER)));
  
  uint64_t programSize = 0, numSymbols = 0, startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramHeader(testFile.name().c_str(), &programSize,
				&numSymbols, &startAddress, &errorMessage),
	    VmImageIOError);

  std::ostringstream expectedErrorMsg;
  expectedErrorMsg << "Error reading from " << testFile.name()
		   << ": Attempted to read 24 bytes, but read only "
		   << sizeof(SHORT_IMAGE_HEADER) << " bytes";
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage), expectedErrorMsg.str());
  free((void*)errorMessage);
}

TEST(vm_image_tests, readImageHeaderWithBadMagicNumber) {
  unl_test::TemporaryFile testFile;

  ASSERT_TRUE(prepareVmImage(testFile, BAD_MAGIC, BAD_MAGIC_SIZE));

  uint64_t programSize = 0, numSymbols = 0, startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramHeader(testFile.name().c_str(), &programSize,
				&numSymbols, &startAddress, &errorMessage),
	    VmImageFormatError);

  std::ostringstream expectedErrorMsg;
  expectedErrorMsg << "Error reading header from " << testFile.name()
		   << ": Not an Unlambda VM program image";
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage), expectedErrorMsg.str());
  free((void*)errorMessage);
}

TEST(vm_image_tests, readHeaderOfNonexistentFile) {
  uint64_t programSize = 0, numSymbols = 0, startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramHeader("DOES_NOT_EXIST.unl", &programSize,
				&numSymbols, &startAddress, &errorMessage),
	    VmImageIOError);
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage),
	    "Error opening DOES_NOT_EXIST.unl: No such file or directory");
  free((void*)errorMessage);
}

TEST(vm_image_tests, loadImageIntoVm) {
  unl_test::TemporaryFile testFile;
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  ASSERT_TRUE(prepareVmImage1(testFile));

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  if (loadVmProgramImage(testFile.name().c_str(), vm, 1, &startAddress,
			 &errorMessage)) {
    destroyUnlambdaVM(vm);
    FAIL() << "Call to loadVmProgramImage(" << testFile.name() << ") failed: "
	   << (errorMessage ? errorMessage : "NULL");
  }
  
  EXPECT_EQ(startAddress, 4);
  EXPECT_EQ(errorMessage, (void*)0);

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 16);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 16);
  EXPECT_TRUE(unl_test::verifyBytes(getProgramStartInVmm(memory),
				    VM_IMAGE_1 + 24, 14));
  
  SymbolTable symtab = getVmSymbolTable(vm);
  EXPECT_EQ(symbolTableSize(symtab), 2);

  const Symbol* s = findSymbol(symtab, "COW");
  EXPECT_NE(s, (void*)0);
  if (s) {
    EXPECT_EQ(std::string(s->name), "COW");
    EXPECT_EQ(s->address, (uint64_t)0x8877665544332211);
  }

  s = findSymbol(symtab, "PENGUIN");
  EXPECT_NE(s, (void*)0);
  if (s) {
    EXPECT_EQ(std::string(s->name), "PENGUIN");
    EXPECT_EQ(s->address, (uint64_t)0x8899AABBCCDDEEFF);
  }
  
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadWithoutSymbols) {
  unl_test::TemporaryFile testFile;
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  ASSERT_TRUE(prepareVmImage1(testFile));

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);

  if (loadVmProgramImage(testFile.name().c_str(), vm, 0, &startAddress,
			 &errorMessage)) {
    destroyUnlambdaVM(vm);
    FAIL() << "Call to loadVmProgramImage(" << testFile.name() << ") failed: "
	   << (errorMessage ? errorMessage : "NULL");
  }
  
  EXPECT_EQ(startAddress, 4);
  EXPECT_EQ(errorMessage, (void*)0);

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 16);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 16);
  EXPECT_TRUE(unl_test::verifyBytes(getProgramStartInVmm(memory),
				    VM_IMAGE_1 + 24, 14));
  
  SymbolTable symtab = getVmSymbolTable(vm);
  EXPECT_EQ(symbolTableSize(symtab), 0);
  
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadNonExistentImageIntoVm) {
  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramImage("DOES_NOT_EXIST.unl", vm, 0, &startAddress,
			       &errorMessage),
	    VmImageIOError);
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage), 
	    "Error opening DOES_NOT_EXIST.unl: No such file or directory");

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 0);

  free((void*)errorMessage);
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadImageWithBadMagicNumber) {
  unl_test::TemporaryFile testFile;

  ASSERT_TRUE(prepareVmImage(testFile, BAD_MAGIC, BAD_MAGIC_SIZE));

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramImage(testFile.name().c_str(), vm, 1, &startAddress,
			       &errorMessage),
	    VmImageFormatError);

  std::ostringstream expectedErrorMsg;
  expectedErrorMsg << "Error reading header from " << testFile.name()
		   << ": Not an Unlambda VM program image";
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage), expectedErrorMsg.str());

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 0);

  free((void*)errorMessage);
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadImageTooBigForVm) {
  static const uint8_t HEADER[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'S' ,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  unl_test::TemporaryFile testFile;
  uint8_t* imageData = (uint8_t*)malloc(257 + sizeof(HEADER));

  ASSERT_NE(imageData, (void*)0);
  memcpy(imageData, HEADER, sizeof(HEADER));
  memset(imageData + sizeof(HEADER), 0x10, 257);

  ASSERT_TRUE(prepareVmImage(testFile, imageData, sizeof(HEADER) + 257));
  free((void*)imageData);

  UnlambdaVM vm = createUnlambdaVM(16, 16, 256, 256);
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;
  
  EXPECT_EQ(loadVmProgramImage(testFile.name().c_str(), vm, 1, &startAddress,
			       &errorMessage),
	    VmImageOutOfMemoryError);
  ASSERT_NE(errorMessage, (void*)0);
  EXPECT_EQ(std::string(errorMessage), "Cannot load a program of 257 bytes "
	    "into a memory of 256 bytes");

  VmMemory memory = getVmMemory(vm);
  
  EXPECT_EQ(currentVmmSize(memory), 256);
  EXPECT_EQ(vmmHeapSize(memory), 256);
  EXPECT_EQ(vmmBytesFree(memory), 256 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 0);

  free((void*)errorMessage);
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadImageWithTruncatedProgram) {
  static const uint8_t TRUNCATED_PROGRAM[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'S' ,
    0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x08, 0x0F, 0x0A, 0x01, 0xAD, 0xBE, 0xED,
  };
  unl_test::TemporaryFile testFile;

  ASSERT_TRUE(prepareVmImage(testFile, TRUNCATED_PROGRAM,
			     sizeof(TRUNCATED_PROGRAM)));

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramImage(testFile.name().c_str(), vm, 1, &startAddress,
			       &errorMessage),
	    VmImageIOError);
  ASSERT_NE(errorMessage, (void*)0);

  std::ostringstream expectedErrorMsg;
  expectedErrorMsg << "Error reading from " << testFile.name() << ": "
		   << "Attempted to read 14 bytes, but read only 8 bytes";
  EXPECT_EQ(std::string(errorMessage), expectedErrorMsg.str());

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 16);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 16);  
  
  free((void*)errorMessage);
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadImageWithMissingSymbol) {
  static const uint8_t MISSING_SYMBOL[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'S' ,
    0x0E, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x08, 0x0F, 0x0A, 0x01, 0xAD, 0xBE, 0xED,
    0xFE, 0xEF, 0xBE, 0xAD, 0xDE, 0x05, 0x0B, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 'C' ,
    'O' , 'W', 
  };

  unl_test::TemporaryFile testFile;

  ASSERT_TRUE(prepareVmImage(testFile, MISSING_SYMBOL, sizeof(MISSING_SYMBOL)));

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramImage(testFile.name().c_str(), vm, 1, &startAddress,
			       &errorMessage),
	    VmImageIOError);
  ASSERT_NE(errorMessage, (void*)0);

  std::ostringstream expectedErrorMsg;
  expectedErrorMsg << "Error reading from " << testFile.name() << ": "
		   << "Attempted to read 1 bytes, but read only 0 bytes";
  EXPECT_EQ(std::string(errorMessage), expectedErrorMsg.str());

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 16);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 16);  
  
  free((void*)errorMessage);
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadImageWithTruncatedSymbol) {
  static const uint8_t TRUNCATED_SYMBOL[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'S' ,
    0x0E, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x08, 0x0F, 0x0A, 0x01, 0xAD, 0xBE, 0xED,
    0xFE, 0xEF, 0xBE, 0xAD, 0xDE, 0x05, 0x0B, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 'C' ,
    'O' , 'W' , 0x0F, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
    0xAA, 0x99, 0x88, 'P' , 'E' , 'N' ,
  };
  unl_test::TemporaryFile testFile;

  ASSERT_TRUE(prepareVmImage(testFile, TRUNCATED_SYMBOL,
			     sizeof(TRUNCATED_SYMBOL)));

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramImage(testFile.name().c_str(), vm, 1, &startAddress,
			       &errorMessage),
	    VmImageIOError);
  ASSERT_NE(errorMessage, (void*)0);

  std::ostringstream expectedErrorMsg;
  expectedErrorMsg << "Error reading from " << testFile.name() << ": "
		   << "Attempted to read 15 bytes, but read only 11 bytes";
  EXPECT_EQ(std::string(errorMessage), expectedErrorMsg.str());

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 16);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 16);  
  
  free((void*)errorMessage);
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, loadImageWithDuplicateSymbolName) {
  const uint8_t DUPLICATE_SYMBOL[] = {
    'M' , 'O' , 'O' , '4' , 'C' , 'O' , 'W' , 'S' ,
    0x0E, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x08, 0x0F, 0x0A, 0x01, 0xAD, 0xBE, 0xED,
    0xFE, 0xEF, 0xBE, 0xAD, 0xDE, 0x05, 0x0B, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 'C' ,
    'O' , 'W' , 0x0F, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
    0xAA, 0x99, 0x88, 'P' , 'E' , 'N' , 'G' , 'U' ,
    'I' , 'N' , 0x0B, 0xF0, 0xED, 0xCB, 0xA9, 0x87,
    0x65, 0x43, 0x21, 'C' , 'O' , 'W'
  };
  unl_test::TemporaryFile testFile;

  ASSERT_TRUE(prepareVmImage(testFile, DUPLICATE_SYMBOL,
			     sizeof(DUPLICATE_SYMBOL)));

  UnlambdaVM vm = createUnlambdaVM(16, 16, 1024, 1024);
  uint64_t startAddress = 0;
  const char* errorMessage = NULL;

  EXPECT_EQ(loadVmProgramImage(testFile.name().c_str(), vm, 1, &startAddress,
			       &errorMessage),
	    VmImageFormatError);
  ASSERT_NE(errorMessage, (void*)0);

  std::ostringstream expectedErrorMsg;
  expectedErrorMsg << "Error reading symbol at offset 66 from "
		   << testFile.name() << ": Cannot add symbol to symbol "
		   << "table (Symbol with name \"COW\" already exists)";
  EXPECT_EQ(std::string(errorMessage), expectedErrorMsg.str());

  VmMemory memory = getVmMemory(vm);

  EXPECT_EQ(currentVmmSize(memory), 1024);
  EXPECT_EQ(vmmHeapSize(memory), 1024 - 16);
  EXPECT_EQ(vmmBytesFree(memory), 1024 - 16 - sizeof(HeapBlock));
  EXPECT_EQ(getVmmProgramMemorySize(memory), 16);  
  
  free((void*)errorMessage);
  destroyUnlambdaVM(vm);
}

TEST(vm_image_tests, saveProgramImage) {
  static const uint8_t PROGRAM[] = {
    0x0E, 0x08, 0x0F, 0x0A, 0x01, 0xAD, 0xBE, 0xED,
    0xFE, 0xEF, 0xBE, 0xAD, 0xDE, 0x05,
  };
  static const uint64_t START_ADDRESS = 4;
  unl_test::TemporaryFile testFile;
  SymbolTable symtab = createSymbolTable(256);
  const char* errorMessage = NULL;

  addSymbolToTable(symtab, "COW", 0x8877665544332211);
  addSymbolToTable(symtab, "PENGUIN", 0x8899AABBCCDDEEFF);

  EXPECT_EQ(saveVmProgramImage(testFile.name().c_str(), PROGRAM,
			       sizeof(PROGRAM), START_ADDRESS, symtab,
			       &errorMessage),
	    0);
  EXPECT_EQ(errorMessage, (void*)0);

  EXPECT_TRUE(testFile.verify(VM_IMAGE_1, VM_IMAGE_1_SIZE));
  destroySymbolTable(symtab);
}
