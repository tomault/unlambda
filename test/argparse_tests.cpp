extern "C" {
  #include <argparse.h>
}

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <stdio.h>

namespace {
  ::testing::AssertionResult verifyNextArg(CmdLineArgParser parser,
					   const char* next,
					   const char* expected) {
    if (expected) {
      if (!next) {
	return ::testing::AssertionFailure()
	  << "Next argument is NULL, but it should be \"" << expected << "\"";
      } else if (strcmp(next, expected)) {
	return ::testing::AssertionFailure()
	  << "Next argument is \"" << next << "\", but it should be \""
	  << expected << "\"";
      } else if (getCmdLineArgParserStatus(parser)) {
	return ::testing::AssertionFailure()
	  << "getCmdLineArgParserStatus() is "
	  << getCmdLineArgParserStatus(parser) << ", but it should be 0";
      } else if (std::string(getCmdLineArgParserStatusMsg(parser)) != "OK") {
	return ::testing::AssertionFailure()
	  << "getCmdLineArgParserStatusMsg() is \""
	  << getCmdLineArgParserStatusMsg(parser) << "\" but it should be "
	  << "\"OK\"";
      } else {
	return ::testing::AssertionSuccess();
      }
    } else if (next) {
      return ::testing::AssertionFailure()
	<< "Next argument is \"" << next << "\", but it should be NULL";
    } else if (getCmdLineArgParserStatus(parser) != NoMoreCmdLineArgsError) {
      return ::testing::AssertionFailure()
	<< "getCmdLineArgParserStatus() is "
	<< getCmdLineArgParserStatus(parser) << ", but it should be "
	<< NoMoreCmdLineArgsError;
    } else if (std::string(getCmdLineArgParserStatusMsg(parser)) !=
	         "No more arguments") {
      return ::testing::AssertionFailure()
	<< "getCmdLineArgParserStatusMsg() is \""
	<< getCmdLineArgParserStatusMsg(parser) << "\", but it should be "
	<< "\"No more arguments\"";
    } else {
      return ::testing::AssertionSuccess();
    }
  }

  ::testing::AssertionResult verifyNextUInt(CmdLineArgParser parser,
					    uint64_t next, uint64_t expected) {
    if (next != expected) {
      return ::testing::AssertionFailure()
	<< "Next argument has value " << next << ", but it should have value "
	<< expected;
    } else if (getCmdLineArgParserStatus(parser)) {
      return ::testing::AssertionFailure()
	<< "getCmdLineArgParserStatus() is "
	<< getCmdLineArgParserStatus(parser) << ", but it should be 0";
    } else if (std::string(getCmdLineArgParserStatusMsg(parser)) != "OK") {
      return ::testing::AssertionFailure()
	<< "getCmdLineArgParserStatusMsg() is \""
	<< getCmdLineArgParserStatusMsg(parser) << "\" but it should be "
	<< "\"OK\"";
    } else {
      return ::testing::AssertionSuccess();
    }
  }
}

TEST(argparse_tests, createArgParser) {
  const char* ARGV[] = { "moo", "-o", "cow", "penguin", NULL };
  const int ARGC = 4;

  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_EQ(getCmdLineArgParserStatus(parser), 0);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)), "OK");
  EXPECT_TRUE(hasMoreCmdLineArgs(parser));
  EXPECT_EQ(currentCmdLineArg(parser), (void*)0);

  destroyCmdLineArgParser(parser);
}

TEST(argparse_tests, scanThroughArguments) {
  const char* ARGV[] = { "moo", "-o", "cow", "penguin", NULL };
  const int ARGC = 4;

  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_EQ(getCmdLineArgParserStatus(parser), 0);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)), "OK");

  EXPECT_TRUE(hasMoreCmdLineArgs(parser));
  EXPECT_EQ(currentCmdLineArg(parser), (void*)0);

  EXPECT_TRUE(verifyNextArg(parser, nextCmdLineArg(parser), "-o"));
  EXPECT_TRUE(hasMoreCmdLineArgs(parser));
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "-o");

  EXPECT_TRUE(verifyNextArg(parser, nextCmdLineArg(parser), "cow"));
  EXPECT_TRUE(hasMoreCmdLineArgs(parser));
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "cow");

  EXPECT_TRUE(verifyNextArg(parser, nextCmdLineArg(parser), "penguin"));
  EXPECT_FALSE(hasMoreCmdLineArgs(parser));
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "penguin");

  EXPECT_TRUE(verifyNextArg(parser, nextCmdLineArg(parser), NULL));
  EXPECT_FALSE(hasMoreCmdLineArgs(parser));
  EXPECT_EQ(currentCmdLineArg(parser), (void*)0);
  
  destroyCmdLineArgParser(parser);
}

TEST(argparse_tests, nextArgInSet) {
  const char* OPTIONS[] = { "apple", "banana", "pear" };
  const uint32_t NUM_OPTIONS = sizeof(OPTIONS)/sizeof(OPTIONS[0]);
  const char* ARGV[] = { "", "pear", "apple", "banana", "cow", NULL };
  const int ARGC = 5;
  
  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_TRUE(verifyNextArg(
    parser, nextCmdLineArgInSet(parser, OPTIONS, NUM_OPTIONS), "pear"
  ));
  EXPECT_TRUE(verifyNextArg(
    parser, nextCmdLineArgInSet(parser, OPTIONS, NUM_OPTIONS), "apple"
  ));
  EXPECT_TRUE(verifyNextArg(
    parser, nextCmdLineArgInSet(parser, OPTIONS, NUM_OPTIONS), "banana"
  ));

  EXPECT_EQ(nextCmdLineArgInSet(parser, OPTIONS, NUM_OPTIONS), (void*)0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value is \"cow\", but it should be one of: "
	    "\"apple\", \"banana\", \"pear\"");
  ASSERT_NE(currentCmdLineArg(parser), (void*)0);
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "cow");

  EXPECT_TRUE(verifyNextArg(
    parser, nextCmdLineArgInSet(parser, OPTIONS, NUM_OPTIONS), NULL
  ));
  destroyCmdLineArgParser(parser);
}

TEST(argparse_tests, nextUInt64) {
  const char* ARGV[] = { "", "18446744073709551615", "18446744073709551616",
			 "abc", "34boo", NULL };
  const int ARGC = 5;
  
  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsUInt64(parser),
			     0xFFFFFFFFFFFFFFFF));

  EXPECT_EQ(nextCmdLineArgAsUInt64(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value is too large");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "18446744073709551616");
  
  EXPECT_EQ(nextCmdLineArgAsUInt64(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "abc");

  EXPECT_EQ(nextCmdLineArgAsUInt64(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "34boo");

  EXPECT_EQ(nextCmdLineArgAsUInt64(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), NoMoreCmdLineArgsError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "No more arguments");

  destroyCmdLineArgParser(parser);
}

TEST(argparse_tests, nextUInt32) {
  const char* ARGV[] = { "", "4294967295", "4294967296", "moo", NULL };
  const int ARGC = 4;

  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsUInt32(parser),
			     0xFFFFFFFF));
  
  EXPECT_EQ(nextCmdLineArgAsUInt32(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer < 4294967296");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "4294967296");
  
  EXPECT_EQ(nextCmdLineArgAsUInt32(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "moo");

  EXPECT_EQ(nextCmdLineArgAsUInt32(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), NoMoreCmdLineArgsError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "No more arguments");

  destroyCmdLineArgParser(parser);  
}

TEST(argparse_tests, nextUInt16) {
  const char* ARGV[] = { "", "65535", "65536", "moo", NULL };
  const int ARGC = 4;

  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsUInt16(parser), 0xFFFF));
  
  EXPECT_EQ(nextCmdLineArgAsUInt16(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer < 65536");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "65536");
  
  EXPECT_EQ(nextCmdLineArgAsUInt16(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "moo");

  EXPECT_EQ(nextCmdLineArgAsUInt16(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), NoMoreCmdLineArgsError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "No more arguments");

  destroyCmdLineArgParser(parser);  
}

TEST(argparse_tests, nextUInt8) {
  const char* ARGV[] = { "", "255", "256", "moo", NULL };
  const int ARGC = 4;

  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsUInt8(parser), 0xFF));
  
  EXPECT_EQ(nextCmdLineArgAsUInt8(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer < 256");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "256");
  
  EXPECT_EQ(nextCmdLineArgAsUInt8(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value must be a nonnegative integer");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "moo");

  EXPECT_EQ(nextCmdLineArgAsUInt8(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), NoMoreCmdLineArgsError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "No more arguments");

  destroyCmdLineArgParser(parser);  
}

TEST(argparse_tests, nextMemorySizeArg) {
  const char* ARGV[] = { "", "18446744073709551615", "18014398509481983k",
			 "17592186044415m", "17179869183g", "1p", "1gb",
			 "18014398509481984k", "17592186044416m",
			 "17179869184g", NULL };
  const int ARGC = 10;

  CmdLineArgParser parser = createCmdLineArgParser(ARGC, (char**)ARGV);
  ASSERT_NE(parser, (void*)0);

  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsMemorySize(parser),
			     0xFFFFFFFFFFFFFFFF));
  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsMemorySize(parser),
			     0xFFFFFFFFFFFFFC00));
  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsMemorySize(parser),
			     0xFFFFFFFFFFF00000));
  EXPECT_TRUE(verifyNextUInt(parser, nextCmdLineArgAsMemorySize(parser),
			     0xFFFFFFFFC0000000));

  EXPECT_EQ(nextCmdLineArgAsMemorySize(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Unknown size suffix \"p\"");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "1p");

  EXPECT_EQ(nextCmdLineArgAsMemorySize(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Unknown size suffix \"gb\"");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "1gb");
  
  EXPECT_EQ(nextCmdLineArgAsMemorySize(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value is too large");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "18014398509481984k");
  
  EXPECT_EQ(nextCmdLineArgAsMemorySize(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value is too large");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "17592186044416m");

  EXPECT_EQ(nextCmdLineArgAsMemorySize(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), InvalidCmdLineArgError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "Value is too large");
  EXPECT_EQ(std::string(currentCmdLineArg(parser)), "17179869184g");
  
  EXPECT_EQ(nextCmdLineArgAsUInt8(parser), 0);
  EXPECT_EQ(getCmdLineArgParserStatus(parser), NoMoreCmdLineArgsError);
  EXPECT_EQ(std::string(getCmdLineArgParserStatusMsg(parser)),
	    "No more arguments");

  destroyCmdLineArgParser(parser);  
}
