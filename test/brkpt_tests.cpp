extern "C" {
#include <array.h>
#include <brkpt.h>

/** Testing only access to a BreakpointList's current candidate for
 *  next breakpoint and PC from the last call to isAtBreakpoint()
 */
  uint32_t breakpointListCurrentCandidate(BreakpointList brkList);
  void setBreakpointListCurrentCandidate(BreakpointList brkList, uint32_t c);
  uint64_t breakpointListLastPC(BreakpointList brkList);
  void setBreakpointListLastPC(BreakpointList brkList, uint64_t address);
}

#include <gtest/gtest.h>
#include <testing_utils.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace unl_test = unlambda::testing;

namespace {
  ::testing::AssertionResult createAndInitBL(const uint64_t* breakpoints,
					     const uint32_t size,
					     const uint32_t maxSize,
					     BreakpointList& bl) {
    bl = createBreakpointList(maxSize);
    if (!bl) {
      return ::testing::AssertionFailure()
	<< "Failed to create breakpoint list";
    }

    for (uint32_t i = 0; i < size; ++i) {
      if (addBreakpointToList(bl, breakpoints[i])) {
	int code = getBreakpointListStatus(bl);
	std::string details(getBreakpointListStatusMsg(bl));

	destroyBreakpointList(bl);

	return ::testing::AssertionFailure()
	  << "Failed to add " << breakpoints[i] << " to list ("
	  << code << "/" << details << ")";
      }
    }

    return ::testing::AssertionSuccess();
  }

  ::testing::AssertionResult verifyBreakpointList(BreakpointList bl,
						  const uint64_t* trueAddresses,
						  const uint32_t trueSize,
						  const uint64_t trueLastPC,
						  const uint32_t trueCC) {

    // Verify size
    if (breakpointListSize(bl) != trueSize) {
      return ::testing::AssertionFailure()
	<< "Breakpoint list has size " << breakpointListSize(bl)
	<< ", but it should have size " << trueSize;
    }

    // Verify content
    const uint64_t* blAddresses = startOfBreakpointList(bl);
    for (uint32_t i = 0; i < trueSize; ++i) {
      if (blAddresses[i] != trueAddresses[i]) {
	return ::testing::AssertionFailure()
	  << "Breakpoint list is "
	  << unl_test::toString(blAddresses, breakpointListSize(bl))
	  << ", but it should be "
	  << unl_test::toString(trueAddresses, trueSize);
      }
    }

    // Verify last PC and current breakpoint candidate
    if (breakpointListLastPC(bl) != trueLastPC) {
      return ::testing::AssertionFailure()
	<< "Breakpoint list's last PC is " << breakpointListLastPC(bl)
	<< ", but it should be " << trueLastPC;
    }

    if (breakpointListCurrentCandidate(bl) != trueCC) {
      return ::testing::AssertionFailure()
	<< "Breakpoint list's current candidate for next breakpoint is "
	<< breakpointListCurrentCandidate(bl) << ", but it should be "
	<< trueCC;
    }

    return ::testing::AssertionSuccess();
  }
  
}

// Create a new breakpoint list
TEST(breakpointlist_tests, createBreakpointList) {
  BreakpointList bl = createBreakpointList(16);

  ASSERT_NE(bl, (void*)0);
  EXPECT_EQ(getBreakpointListStatus(bl), 0);
  EXPECT_EQ(std::string(getBreakpointListStatusMsg(bl)), "OK");
  
  EXPECT_EQ(breakpointListSize(bl), 0);
  EXPECT_EQ(breakpointListMaxSize(bl), 16);
  EXPECT_EQ(startOfBreakpointList(bl), endOfBreakpointList(bl));
  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);

  destroyBreakpointList(bl);
}

// Attempt to create a breakpoint list with a zero maximum size
// This should fail and return NULL
TEST(breakpointlist_tests, createBreakpointListWithZeroMaxSize) {
  // maxSize cannot be zero.
  BreakpointList bl = createBreakpointList(0);
  EXPECT_EQ(bl, (void*)0);
}

// Add several breakpoints to an empty list
TEST(breakpointlist_tests, addBreakpointsToList) {
  BreakpointList bl = createBreakpointList(16);

  ASSERT_NE(bl, (void*)0);
  ASSERT_EQ(getBreakpointListStatus(bl), 0);
  ASSERT_EQ(std::string(getBreakpointListStatusMsg(bl)), "OK");
  ASSERT_EQ(breakpointListSize(bl), 0);

  const uint64_t breakpoints[] = { 512, 319, 640, 571 };
  const uint32_t blCandidateLoc[] = { 1, 2, 2, 3 };
  const size_t numBreakpoints = ARRAY_SIZE(breakpoints);

  setBreakpointListLastPC(bl, 571);

  for (int i = 0; i < numBreakpoints; ++i) {
    if (addBreakpointToList(bl, breakpoints[i])) {
      FAIL() << "Adding " << breakpoints[i] << " to breakpoint list failed ("
	     << getBreakpointListStatus(bl)  << "/"
	     << std::string(getBreakpointListStatusMsg(bl)) << ")";
    }
    EXPECT_EQ(getBreakpointListStatus(bl), 0)
      << "Adding " << breakpoints[i] << " to breakpoint list failed "
      << "(BL status is " << getBreakpointListStatus(bl) << ")";
    EXPECT_EQ(std::string(getBreakpointListStatusMsg(bl)), "OK")
      << "Adding " << breakpoints[i] << " to breakpoint list failed "
      << "(BL status message is ["
      << std::string(getBreakpointListStatusMsg(bl)) << "])";

    EXPECT_EQ(breakpointListCurrentCandidate(bl), blCandidateLoc[i])
      << "After adding " << breakpoints[i] << " to the breakpoint list, "
      << "the current breakpoint candidate is "
      << breakpointListCurrentCandidate(bl) << ", but it should be "
      << blCandidateLoc[i];
  }

  ASSERT_EQ(breakpointListSize(bl), numBreakpoints);
  EXPECT_EQ(breakpointListMaxSize(bl), 16);
  EXPECT_EQ(endOfBreakpointList(bl) - startOfBreakpointList(bl),
	    numBreakpoints);

  const uint64_t sortedBreakpoints[] = { 319, 512, 571, 640 };
  ASSERT_TRUE(verifyBreakpointList(bl, sortedBreakpoints, numBreakpoints,
				   571, 3));

  destroyBreakpointList(bl);
}

// Attempt to add a breakpoint that exceeds the maximum number of breakpoints
// the list allows
TEST(breakpointlist_tests, addBreakpointExceedingMaxSize) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint64_t sortedBreakpoints[] = {
    55, 319, 328, 512, 571, 640, 721, 999
  };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  static_assert(ARRAY_SIZE(sortedBreakpoints) == ARRAY_SIZE(breakpoints));

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, numBreakpoints, bl));

  EXPECT_NE(addBreakpointToList(bl, 16), 0);
  EXPECT_EQ(getBreakpointListStatus(bl), BreakpointListFullError);
  EXPECT_EQ(std::string(getBreakpointListStatusMsg(bl)),
	    "Breakpoint list is full");

  EXPECT_TRUE(verifyBreakpointList(bl, sortedBreakpoints, numBreakpoints,
				   0, 0));

  destroyBreakpointList(bl);
}

// Add a duplicate breakpoint to a list
TEST(breakpointlist_tests, addDuplicateBreakpoint) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint64_t sortedBreakpoints[] = {
    55, 319, 328, 512, 571, 640, 721, 999
  };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  static_assert(ARRAY_SIZE(sortedBreakpoints) == ARRAY_SIZE(breakpoints));

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  EXPECT_EQ(addBreakpointToList(bl, breakpoints[3]), 0);
  EXPECT_EQ(getBreakpointListStatus(bl), 0);
  EXPECT_EQ(std::string(getBreakpointListStatusMsg(bl)), "OK");

  EXPECT_TRUE(verifyBreakpointList(bl, sortedBreakpoints, numBreakpoints,
				   0, 0));

  std::cout << unl_test::toString(startOfBreakpointList(bl),
				  breakpointListSize(bl)) << std::endl;

  destroyBreakpointList(bl);
}

// Remove a breakpoint from the a breakpoint list
TEST(breakpointlist_tests, removeBreakpoint) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  setBreakpointListLastPC(bl, 571);
  setBreakpointListCurrentCandidate(bl, 5);

  EXPECT_EQ(removeBreakpointFromList(bl, 328), 0);
  EXPECT_EQ(getBreakpointListStatus(bl), 0);
  EXPECT_EQ(std::string(getBreakpointListStatusMsg(bl)), "OK");

  // List is now 55, 319, 512, 571, 640, 721, 999.  CC is at 4
  const uint64_t sortedBreakpoints[] = { 55, 319, 512, 571, 640, 721, 999 };
  EXPECT_TRUE(verifyBreakpointList(bl, sortedBreakpoints,
				   ARRAY_SIZE(sortedBreakpoints), 571, 4));

  destroyBreakpointList(bl);
}

// Remove a nonexistent breakpoint from a breakpoint list
TEST(breakpointlist_tests, removeNonexistentBreakpoint) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  setBreakpointListLastPC(bl, 571);
  setBreakpointListCurrentCandidate(bl, 5);

  EXPECT_EQ(removeBreakpointFromList(bl, 400), 0);
  EXPECT_EQ(getBreakpointListStatus(bl), 0);
  EXPECT_EQ(std::string(getBreakpointListStatusMsg(bl)), "OK");

  // List is same as before
  const uint64_t sortedBreakpoints[] = {
    55, 319, 328, 512, 571, 640, 721, 999
  };
  EXPECT_TRUE(verifyBreakpointList(bl, sortedBreakpoints,
				   ARRAY_SIZE(sortedBreakpoints), 571, 5));

  destroyBreakpointList(bl);
}

// Remove all breakpoints from a breakpoint list with clearBreakpointList()
TEST(breakpointlist_tests, clearBreakpointList) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  setBreakpointListLastPC(bl, 571);
  setBreakpointListCurrentCandidate(bl, 5);

  EXPECT_EQ(clearBreakpointList(bl), 0);
  EXPECT_EQ(getBreakpointListStatus(bl), 0);
  EXPECT_EQ(std::string(getBreakpointListStatusMsg(bl)), "OK");

  EXPECT_TRUE(verifyBreakpointList(bl, NULL, 0, 571, 0));

  destroyBreakpointList(bl);
}

// Search for breakpoints by address
TEST(breakpointlist_tests, findBreakpointByAddress) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  EXPECT_EQ(findBreakpointInList(bl, 55), startOfBreakpointList(bl));
  EXPECT_EQ(findBreakpointInList(bl, 999), startOfBreakpointList(bl) + 7);
  EXPECT_EQ(findBreakpointInList(bl, 400), (void*)0);

  destroyBreakpointList(bl);
}

// Search for breakpoints at or after a given address
TEST(breakpointlist_tests, findBreakpointAtOrAfter) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  //     55, 319, 328, 512, 571, 640, 721, 999
  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  EXPECT_EQ(findBreakpointAtOrAfter(bl, 721), startOfBreakpointList(bl) + 6);
  EXPECT_EQ(findBreakpointAtOrAfter(bl, 44), startOfBreakpointList(bl));
  EXPECT_EQ(findBreakpointAtOrAfter(bl, 600), startOfBreakpointList(bl) + 5);
  EXPECT_EQ(findBreakpointAtOrAfter(bl, 1024), startOfBreakpointList(bl) + 8);

  destroyBreakpointList(bl);
}

// Search for breakpoints after a given address
TEST(breakpointlist_tests, findBreakpointAfter) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 512, 319, 640, 571, 55, 721, 328, 999 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  //     55, 319, 328, 512, 571, 640, 721, 999
  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  EXPECT_EQ(findBreakpointAfter(bl, 44), startOfBreakpointList(bl));
  EXPECT_EQ(findBreakpointAfter(bl, 55), startOfBreakpointList(bl) + 1);
  EXPECT_EQ(findBreakpointAfter(bl, 600), startOfBreakpointList(bl) + 5);
  EXPECT_EQ(findBreakpointAfter(bl, 721), startOfBreakpointList(bl) + 7);
  EXPECT_EQ(findBreakpointAfter(bl, 999), startOfBreakpointList(bl) + 8);
  EXPECT_EQ(findBreakpointAfter(bl, 1024), startOfBreakpointList(bl) + 8);

  destroyBreakpointList(bl);
}

// Check that isAtBreakpoint detects a breakpoint with the PC moving forward
// one instruction
TEST(breakpointlist_tests, detectBreakpointMovingForwardOneInstruction) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 55, 57 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 0);
  EXPECT_FALSE(isAtBreakpoint(bl, 54));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 54);
  EXPECT_TRUE(isAtBreakpoint(bl, 55));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 55);
  EXPECT_TRUE(isAtBreakpoint(bl, 55));
  
  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 55);
  EXPECT_FALSE(isAtBreakpoint(bl, 56));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 1);
  EXPECT_EQ(breakpointListLastPC(bl), 56);
  EXPECT_TRUE(isAtBreakpoint(bl, 57));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 1);
  EXPECT_EQ(breakpointListLastPC(bl), 57);
  EXPECT_FALSE(isAtBreakpoint(bl, 58));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 2);
  EXPECT_EQ(breakpointListLastPC(bl), 58);

  destroyBreakpointList(bl);
}

// Check that isAtBreakpoint detects a breakpoint with the PC moving forward
// several instructions
TEST(breakpointlist_tests, detectBreakpointMovingForwardSeveralInstructions) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 55, 57, 72, 101 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 0);
  EXPECT_TRUE(isAtBreakpoint(bl, 55));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 55);
  EXPECT_FALSE(isAtBreakpoint(bl, 61));
  
  EXPECT_EQ(breakpointListCurrentCandidate(bl), 2);
  EXPECT_EQ(breakpointListLastPC(bl), 61);
  EXPECT_TRUE(isAtBreakpoint(bl, 72));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 2);
  EXPECT_EQ(breakpointListLastPC(bl), 72);
  EXPECT_FALSE(isAtBreakpoint(bl, 102));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 4);
  EXPECT_EQ(breakpointListLastPC(bl), 102);

  destroyBreakpointList(bl);
}

// Check that isAtBreakpoint detects a breakpoint with the PC moving backward
TEST(breakpointlist_tests, detectBreakpointMovingBackwardes) {
  BreakpointList bl = NULL;
  const uint64_t breakpoints[] = { 55, 57, 72, 101 };
  const uint32_t numBreakpoints = ARRAY_SIZE(breakpoints);

  ASSERT_TRUE(createAndInitBL(breakpoints, numBreakpoints, 16, bl));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 0);
  EXPECT_FALSE(isAtBreakpoint(bl, 75));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 3);
  EXPECT_EQ(breakpointListLastPC(bl), 75);
  EXPECT_TRUE(isAtBreakpoint(bl, 57));
  
  EXPECT_EQ(breakpointListCurrentCandidate(bl), 1);
  EXPECT_EQ(breakpointListLastPC(bl), 57);
  EXPECT_FALSE(isAtBreakpoint(bl, 54));

  EXPECT_EQ(breakpointListCurrentCandidate(bl), 0);
  EXPECT_EQ(breakpointListLastPC(bl), 54);

  destroyBreakpointList(bl);
}
