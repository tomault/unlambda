extern "C" {
  #include <logging.h>
  #include <stack.h>
}

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <stdio.h>

namespace {
  ::testing::AssertionResult verifyLoggerOutput(
    const char* loggerOutput, const char* expectedOutput
  ) {
    char* cleanedOutput = NULL;
    size_t cleanedOutputLength = 0;
    FILE* cleaningStream = open_memstream(&cleanedOutput, &cleanedOutputLength);

    /** Because the timestamps will vary between runs, write a "cleaned"
     *  version of the logger output with the timestamps stripped to
     *  cleaningStream, then use the cleaned output to verify the
     *  logger's output.
     *
     *  TODO:  At least verify the timestamps have the expected format.
     */
    if (!cleaningStream) {
      return ::testing::AssertionFailure()
	<< "Could not open memstream for cleaned logger output";
    }

    const char* p = loggerOutput;
    while(*p) {
      int spaceCnt = 0;
      const char* q = NULL;
      
      for (q = p; *q && (*q != '\n'); ++q) ;
      while ((p < q) && (spaceCnt < 2)) {
	if (*p == ' ') {
	  ++spaceCnt;
	}
	++p;
      }
      fwrite(p, 1, q - p + (*q ? 1 : 0), cleaningStream);
      p = q + (*q ? 1 : 0);
    }

    fclose(cleaningStream);
    if (strcmp(cleanedOutput, expectedOutput)) {
      auto result = ::testing::AssertionFailure()
	<< "Logger output differs from expected output.  Logger output:\n"
	<< std::string(loggerOutput) << "\n"
	<< "Logger output without timestamps:\n"
	<< cleanedOutput << "\n"
	<< "Expected output:\n"
	<< expectedOutput << "\n";
      free((void*)cleanedOutput);
      return result;
    }
    free((void*)cleanedOutput);
    return ::testing::AssertionSuccess();
  }
}

TEST(logging_tests, createNewLogger) {
  Logger logger = createLogger(stdout, LogGeneralInfo | LogMemoryAllocations);

  ASSERT_NE(logger, (void*)0);

  EXPECT_EQ(loggingModulesEnabled(logger),
	    LogGeneralInfo | LogMemoryAllocations);

  EXPECT_TRUE(loggingModuleIsEnabled(logger, LogGeneralInfo));
  EXPECT_TRUE(loggingModuleIsEnabled(logger, LogMemoryAllocations));
  EXPECT_FALSE(loggingModuleIsEnabled(logger, LogInstructions));
  EXPECT_FALSE(loggingModuleIsEnabled(logger, LogStacks));
  EXPECT_FALSE(loggingModuleIsEnabled(logger, LogCodeBlocks));
  EXPECT_FALSE(loggingModuleIsEnabled(logger, LogStateBlocks));
  EXPECT_FALSE(loggingModuleIsEnabled(logger, LogGC1));
  EXPECT_FALSE(loggingModuleIsEnabled(logger, LogGC2));
  
  destroyLogger(logger);
}

TEST(logging_tests, enableAndDisableModules) {
  Logger logger = createLogger(stdout, LogGeneralInfo | LogMemoryAllocations);

  ASSERT_NE(logger, (void*)0);
  ASSERT_EQ(loggingModulesEnabled(logger),
	    LogGeneralInfo | LogMemoryAllocations);

  enableLoggingModules(logger, LogInstructions | LogGC1);
  EXPECT_EQ(loggingModulesEnabled(logger),
	    LogGeneralInfo | LogInstructions | LogMemoryAllocations
	      | LogGC1);

  disableLoggingModules(logger, LogMemoryAllocations);
  EXPECT_EQ(loggingModulesEnabled(logger),
	    LogGeneralInfo | LogInstructions | LogGC1);

  destroyLogger(logger);
}

TEST(logging_tests, writeLogMessage) {
  static const char EXPECTED_OUTPUT[] =
    "INFO Cows are cool\nINFO Penguins are cute\n";
  
  char* logText = NULL;
  size_t logTextLength = 0;
  FILE* output = open_memstream(&logText, &logTextLength);
  ASSERT_NE(output, (void*)0);

  Logger logger = createLogger(output, LogGeneralInfo);
  ASSERT_NE(logger, (void*)0);

  /** The first and third messages are written to an enabled logging module,
   *  so they will be written to the output stream.  The second one is not
   *  and will be ignored
   */
  logMessage(logger, LogGeneralInfo, "Cows are cool");
  logMessage(logger, LogInstructions, "Cats go meow");
  logMessage(logger, LogGeneralInfo, "Penguins are cute");

  destroyLogger(logger);
  fclose(output);

  EXPECT_TRUE(verifyLoggerOutput(logText, EXPECTED_OUTPUT));
  free((void*)logText);
}

TEST(logging_tests, writeLogMessageWithArguments) {
  static const char EXPECTED_OUTPUT[] = "INFO 1 + 3 = 4\n";
  
  char* logText = NULL;
  size_t logTextLength = 0;
  FILE* output = open_memstream(&logText, &logTextLength);
  ASSERT_NE(output, (void*)0);

  Logger logger = createLogger(output, LogGeneralInfo);
  ASSERT_NE(logger, (void*)0);

  logMessage(logger, LogGeneralInfo, "%d + %d = %d", 1, 3, 4);
  
  destroyLogger(logger);
  fclose(output);

  EXPECT_TRUE(verifyLoggerOutput(logText, EXPECTED_OUTPUT));
  free((void*)logText);
}

TEST(logging_tests, verifyModuleNames) {
  static const char EXPECTED_OUTPUT[] =
    "INFO moo\nINST moo\nSTAC moo\nMEMO moo\nCBLK moo\nSBLK moo\nGC1  moo\n"
    "GC2  moo\n";
  
  char* logText = NULL;
  size_t logTextLength = 0;
  FILE* output = open_memstream(&logText, &logTextLength);
  ASSERT_NE(output, (void*)0);

  Logger logger = createLogger(output, LogAllModules);
  ASSERT_NE(logger, (void*)0);

  logMessage(logger, LogGeneralInfo, "moo");
  logMessage(logger, LogInstructions, "moo");
  logMessage(logger, LogStacks, "moo");
  logMessage(logger, LogMemoryAllocations, "moo");
  logMessage(logger, LogCodeBlocks, "moo");
  logMessage(logger, LogStateBlocks, "moo");
  logMessage(logger, LogGC1, "moo");
  logMessage(logger, LogGC2, "moo");
  
  destroyLogger(logger);
  fclose(output);

  EXPECT_TRUE(verifyLoggerOutput(logText, EXPECTED_OUTPUT));
  free((void*)logText);
}

TEST(logging_tests, enablingGC2enablesGC1) {
  static const char EXPECTED_OUTPUT[] = "GC1  moo\n";
  
  char* logText = NULL;
  size_t logTextLength = 0;
  FILE* output = open_memstream(&logText, &logTextLength);
  ASSERT_NE(output, (void*)0);

  Logger logger = createLogger(output, LogGC2);
  ASSERT_NE(logger, (void*)0);

  logMessage(logger, LogGC1, "moo");
  
  destroyLogger(logger);
  fclose(output);

  EXPECT_TRUE(verifyLoggerOutput(logText, EXPECTED_OUTPUT));
  free((void*)logText);
}

TEST(logging_tests, logAddressStack) {
  static const char EXPECTED_OUTPUT[] =
    "STAC Address stack is [100, 200 (COW), 300, 400]\n";

  Stack addressStack = createStack(0, 5 * sizeof(uint64_t));
  uint64_t value = 500;
  ASSERT_EQ(pushStack(addressStack, &value, sizeof(value)), 0);
  value = 400;
  ASSERT_EQ(pushStack(addressStack, &value, sizeof(value)), 0);
  value = 300;
  ASSERT_EQ(pushStack(addressStack, &value, sizeof(value)), 0);
  value = 200;
  ASSERT_EQ(pushStack(addressStack, &value, sizeof(value)), 0);
  value = 100;
  ASSERT_EQ(pushStack(addressStack, &value, sizeof(value)), 0);

  SymbolTable symtab = createSymbolTable(16);
  ASSERT_EQ(addSymbolToTable(symtab, "COW", 200), 0);
  ASSERT_EQ(addSymbolToTable(symtab, "PENGUIN", 300), 0);
  ASSERT_EQ(addSymbolToTable(symtab, "CAT", 150), 0);

  char* logText = NULL;
  size_t logTextLength = 0;
  FILE* output = open_memstream(&logText, &logTextLength);
  ASSERT_NE(output, (void*)0);

  Logger logger = createLogger(output, LogStacks);
  ASSERT_NE(logger, (void*)0);

  logAddressStack(logger, addressStack, 250, symtab);
  
  destroyLogger(logger);
  fclose(output);
  destroySymbolTable(symtab);
  destroyStack(addressStack);

  EXPECT_TRUE(verifyLoggerOutput(logText, EXPECTED_OUTPUT));
  free((void*)logText);
}

TEST(logging_tests, logCallStack) {
  static const char EXPECTED_OUTPUT[] =
    "STAC Call stack is [(100, 50), (200, 150 (MOO+25)), (300, 250), "
    "(400, 350)]\n";

  Stack callStack = createStack(0, 10 * sizeof(uint64_t));
  uint64_t value = 500;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 450;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 400;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 350;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 300;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 250;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 200;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 150;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value = 100;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);
  value =  50;
  ASSERT_EQ(pushStack(callStack, &value, sizeof(value)), 0);

  SymbolTable symtab = createSymbolTable(16);
  ASSERT_EQ(addSymbolToTable(symtab, "COW", 200), 0);
  ASSERT_EQ(addSymbolToTable(symtab, "MOO", 125), 0);
  ASSERT_EQ(addSymbolToTable(symtab, "PENGUIN", 250), 0);
  ASSERT_EQ(addSymbolToTable(symtab, "CAT", 175), 0);

  char* logText = NULL;
  size_t logTextLength = 0;
  FILE* output = open_memstream(&logText, &logTextLength);
  ASSERT_NE(output, (void*)0);

  Logger logger = createLogger(output, LogStacks);
  ASSERT_NE(logger, (void*)0);

  logCallStack(logger, callStack, 250, symtab);
  
  destroyLogger(logger);
  fclose(output);
  destroySymbolTable(symtab);
  destroyStack(callStack);

  EXPECT_TRUE(verifyLoggerOutput(logText, EXPECTED_OUTPUT));
  free((void*)logText);
}
