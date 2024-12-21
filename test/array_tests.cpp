extern "C" {
#include <array.h>
}

#include <gtest/gtest.h>
#include "testing_utils.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace unl_test = unlambda::testing;

// Create an initially-empty array
TEST(array_tests, createEmptyArray) {
  Array a = createArray(0, 256);

  ASSERT_NE(a, (void*)0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_EQ(arraySize(a), 0);
  EXPECT_EQ(arrayMaxSize(a), 256);
  EXPECT_EQ(arrayCapacity(a), 0);
  EXPECT_EQ(startOfArray(a), endOfArray(a));

  destroyArray(a);
}

// Create an array with an initial size 
TEST(array_tests, createArrayWithSize) {
  Array a = createArray(4, 256);

  ASSERT_NE(a, (void*)0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_EQ(arraySize(a), 4);
  EXPECT_EQ(arrayMaxSize(a), 256);
  EXPECT_EQ(arrayCapacity(a), 16);
  EXPECT_NE(startOfArray(a), (void*)0);
  EXPECT_EQ(endOfArray(a) - startOfArray(a), 4);

  destroyArray(a);  
}

// Attempt to create an array with a zero maximum size
TEST(array_tests, createArrayWithZeroMaximumSize) {
  Array a = createArray(0, 0);
  EXPECT_EQ(a, (void*)0);
}

// Attempt to create an array with an initial size larger than its max
TEST(array_tests, createArrayWithInvalidInitialSize) {
  Array a = createArray(17, 16);
  EXPECT_EQ(a, (void*)0);
}

// Get a pointer to an array element
TEST(array_tests, getPtrToArrayIndex) {
  Array a = createArray(4, 256);

  ASSERT_NE(a, (void*)0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_EQ(arraySize(a), 4);
  EXPECT_EQ(arrayMaxSize(a), 256);
  EXPECT_EQ(arrayCapacity(a), 16);
  EXPECT_NE(startOfArray(a), (void*)0);
  EXPECT_EQ(endOfArray(a) - startOfArray(a), 4);

  for (size_t i = 0; i < arraySize(a); ++i) {
    uint8_t* p = ptrToArrayIndex(a, i);
    if (p != (startOfArray(a) + i)) {
      FAIL() << "ptrToArrayIndex(a, " << i << ") points to index "
	     << (p - startOfArray(a));
    }
  }

  EXPECT_EQ(ptrToArrayIndex(a, arraySize(a)), (void*)0);

  destroyArray(a);
}

// Read a given element in the array
TEST(array_tests, valueAtArrayIndex) {
  const uint8_t arrayData[] = { 1, 2, 3, 4 };
  Array a = unl_test::createAndInitArray(arrayData, ARRAY_SIZE(arrayData), 256);

  ASSERT_NE(a, (void*)0);
  
  for (size_t i = 0; i < ARRAY_SIZE(arrayData); ++i) {
    EXPECT_EQ(valueAtArrayIndex(a, i), i + 1);
    EXPECT_EQ(getArrayStatus(a), 0);
    EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");
  }

  EXPECT_EQ(valueAtArrayIndex(a, ARRAY_SIZE(arrayData)), 0);
  EXPECT_EQ(getArrayStatus(a), ArrayInvalidArgumentError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "Index out of range");
  
  destroyArray(a);
}

// Find a byte in the array
TEST(array_tests, findValueInArray) {
  const uint8_t arrayData[] = { 2, 4, 1, 3, 4, 3, 4, 2, 1 };
  Array a = unl_test::createAndInitArray(arrayData, ARRAY_SIZE(arrayData), 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), ARRAY_SIZE(arrayData));

  // Search the entire array for for first occurence of a value
  EXPECT_EQ(findValueInArray(a, 0, arraySize(a), 1), startOfArray(a) + 2);
  EXPECT_EQ(findValueInArray(a, 0, arraySize(a), 0), (void*)0);

  // Search a subrange of the array for the first occurence of a value
  EXPECT_EQ(findValueInArray(a, 3, 7, 4), startOfArray(a) + 4);
  EXPECT_EQ(findValueInArray(a, 3, 7, 1), (void*)0);

  // Search a range with "end" past the end of the array
  EXPECT_EQ(findValueInArray(a, 3, arraySize(a) + 1, 2), startOfArray(a) + 7);

  // Search an empty range
  EXPECT_EQ(findValueInArray(a, 2, 2, 1), (void*)0);
  EXPECT_EQ(findValueInArray(a, 2, 1, 2), (void*)0);

  // Search a range outside of the array
  EXPECT_EQ(findValueInArray(a, arraySize(a), arraySize(a) + 2, 1), (void*)0);
  EXPECT_EQ(findValueInArray(a, arraySize(a) + 1, arraySize(a) + 10, 1),
	    (void*)0);
  
  destroyArray(a);
}

// Find a sequence in the array
TEST(array_tests, findSequenceInArray) {
  uint8_t arrayData[256];
  for (int i = 0; i < 128; ++i) {
    arrayData[i] = i;
    arrayData[i + 128] = i;
  }

  Array a = unl_test::createAndInitArray(arrayData, ARRAY_SIZE(arrayData), 256);
  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), ARRAY_SIZE(arrayData));

  const uint8_t targetSeq1[] = { 10, 11, 12, 13 };

  // Search entire array for sequence
  EXPECT_EQ(findSeqInArray(a, 0, arraySize(a), targetSeq1,
			   ARRAY_SIZE(targetSeq1)),
	    startOfArray(a) + 10);

  // Search second half for sequence
  EXPECT_EQ(findSeqInArray(a, arraySize(a) / 2, arraySize(a), targetSeq1,
			   ARRAY_SIZE(targetSeq1)),
	    startOfArray(a) + 138);

  // Sequence at start of range
  EXPECT_EQ(findSeqInArray(a, 10, 127, targetSeq1, ARRAY_SIZE(targetSeq1)),
	    startOfArray(a) + 10);

  // Sequence at end of range
  EXPECT_EQ(findSeqInArray(a, 0, 14, targetSeq1, ARRAY_SIZE(targetSeq1)),
	    startOfArray(a) + 10);

  // Sequence lies just outside the range
  EXPECT_EQ(findSeqInArray(a, 0, 13, targetSeq1, ARRAY_SIZE(targetSeq1)),
	    (void*)0);

  // Try to find a zero-length sequence
  EXPECT_EQ(findSeqInArray(a, 0, arraySize(a), targetSeq1, 0), (void*)0);

  // Range is empty
  EXPECT_EQ(findSeqInArray(a, 10, 10, targetSeq1, ARRAY_SIZE(targetSeq1)),
	    (void*)0);
  EXPECT_EQ(findSeqInArray(a, 14, 10, targetSeq1, ARRAY_SIZE(targetSeq1)),
	    (void*)0);


  // Range starts outside the array
  EXPECT_EQ(findSeqInArray(a, arraySize(a),
			   arraySize(a) + ARRAY_SIZE(targetSeq1), targetSeq1,
			   ARRAY_SIZE(targetSeq1)),
	    (void*)0);
  EXPECT_EQ(findSeqInArray(a, arraySize(a) + 1,
			   arraySize(a) + ARRAY_SIZE(targetSeq1) + 1,
			   targetSeq1, ARRAY_SIZE(targetSeq1)),
	    (void*)0);

  // Length of target exceeds size of range
  EXPECT_EQ(findSeqInArray(a, 10, 10 + ARRAY_SIZE(targetSeq1) - 1,
			   targetSeq1, ARRAY_SIZE(targetSeq1)),
	    (void*)0);
	    
  destroyArray(a);
}

// Append a sequence to an empty array
TEST(array_tests, appendToEmptyArray) {
  Array a = createArray(0, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), 0);
  ASSERT_EQ(arrayCapacity(a), 0);

  const uint8_t data[] = { 4, 7, 2, 5 };
  const size_t dataSize = ARRAY_SIZE(data);

  EXPECT_EQ(appendToArray(a, data, dataSize), 0);

  EXPECT_EQ(arraySize(a), dataSize);
  EXPECT_EQ(arrayCapacity(a), 16);

  EXPECT_TRUE(unl_test::verifyArray(a, data, dataSize));
  
  destroyArray(a);
}

// Append a sequence to an empty array, forcing multiple iterations in
// increaseArrayStorage()
TEST(array_tests, appendRequiringMultipleStorageIncreaseIterations) {
  Array a = createArray(0, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), 0);
  ASSERT_EQ(arrayCapacity(a), 0);

  const size_t dataSize = 53;
  uint8_t data[dataSize];
  for (int i = 0; i < dataSize; ++i) {
    data[i] = i * 2 + 1;
  }

  EXPECT_EQ(appendToArray(a, data, dataSize), 0);

  EXPECT_EQ(arraySize(a), dataSize);
  EXPECT_EQ(arrayCapacity(a), 64);

  EXPECT_TRUE(unl_test::verifyArray(a, data, dataSize));
  
  destroyArray(a);
}

// Append a sequence to an empty array, forcing increaseArrayStorage()
// to quit because it has reached the array's maximum size.
TEST(array_tests, appendToEmptyArrayExceedingMaxSize) {
  Array a = createArray(0, 60);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), 0);
  ASSERT_EQ(arrayCapacity(a), 0);

  const size_t dataSize = 53;
  uint8_t data[dataSize];
  for (int i = 0; i < dataSize; ++i) {
    data[i] = i * 2 + 1;
  }

  EXPECT_EQ(appendToArray(a, data, dataSize), 0);

  EXPECT_EQ(arraySize(a), dataSize);
  EXPECT_EQ(arrayCapacity(a), 60);

  EXPECT_TRUE(unl_test::verifyArray(a, data, dataSize));
  
  destroyArray(a);
}

// Append a sequence to an array with data in it
TEST(array_tests, appendToArrayWithData) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  const uint8_t data[] = { 1, 3, 7, 9, 11, 13, 15, 17, 19, 21 };
  const size_t dataSize = ARRAY_SIZE(data);

  EXPECT_EQ(appendToArray(a, data, dataSize), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_EQ(arraySize(a), dataSize + initialDataSize);
  EXPECT_EQ(arrayCapacity(a), 32);

  const size_t finalDataSize = initialDataSize + dataSize;
  uint8_t* finalData = (uint8_t*)malloc(finalDataSize);
  ASSERT_NE(finalData, (void*)0);

  ::memcpy(finalData, initialData, initialDataSize);
  ::memcpy(finalData + initialDataSize, data, dataSize);

  EXPECT_TRUE(unl_test::verifyArray(a, finalData, finalDataSize));
  ::free(finalData);
  
  destroyArray(a);
}

// Attempt to append a sequence to an array, exceeding its maximum size
TEST(array_tests, appendToArrayExceedingMaxSize) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize,
					 initialDataSize + 1);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), initialDataSize + 1);

  const uint8_t data[] = { 1, 3 };
  const size_t dataSize = ARRAY_SIZE(data);
  
  EXPECT_NE(appendToArray(a, data, dataSize), 0);
  EXPECT_EQ(getArrayStatus(a), ArraySequenceTooLongError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)),
	    "Appending 2 bytes to an array of 8 bytes would exceed the "
	    "array's maximum size of 9 bytes");

  // Verify array is unchanged
  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), initialDataSize + 1);

  destroyArray(a);
}

// Append a zero-length sequence to an array
TEST(array_tests, appendZeroLengthSequence) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(appendToArray(a, NULL, 0), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Insert a sequence into an array
TEST(array_tests, insertSequenceIntoArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  const uint8_t newData[] = { 1, 3, 5, 7 };
  const size_t newDataSize = ARRAY_SIZE(newData);

  EXPECT_EQ(insertIntoArray(a, 3, newData, newDataSize), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  const uint8_t result[] = { 2, 4, 6, 1, 3, 5, 7, 8, 10, 12, 14, 16 };
  const size_t resultSize = ARRAY_SIZE(result);
  EXPECT_TRUE(unl_test::verifyArray(a, result, resultSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Insert a sequence at the end of an array
TEST(array_tests, insertSequenceAtArrayEnd) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  const uint8_t newData[] = { 1, 3, 5, 7 };
  const size_t newDataSize = ARRAY_SIZE(newData);

  EXPECT_EQ(insertIntoArray(a, arraySize(a), newData, newDataSize), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  const uint8_t result[] = { 2, 4, 6, 8, 10, 12, 14, 16, 1, 3, 5, 7 };
  const size_t resultSize = ARRAY_SIZE(result);
  EXPECT_TRUE(unl_test::verifyArray(a, result, resultSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Insert a sequence, forcing the array to allocate new storage
TEST(array_tests, insertSequenceIncreasingStorage) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  const uint8_t newData[] = { 1, 3, 5, 7, 9, 11, 13, 15, 17 };
  const size_t newDataSize = ARRAY_SIZE(newData);

  EXPECT_EQ(insertIntoArray(a, arraySize(a), newData, newDataSize), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  const uint8_t result[] = { 2, 4, 6, 8, 10, 12, 14, 16,
			     1, 3, 5, 7, 9, 11, 13, 15, 17 };
  const size_t resultSize = ARRAY_SIZE(result);
  EXPECT_TRUE(unl_test::verifyArray(a, result, resultSize));
  EXPECT_EQ(arrayCapacity(a), 32);

  destroyArray(a);
}

// Insert a zero-length sequence into the array
TEST(array_tests, insertZeroLengthSequence) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(insertIntoArray(a, 0, NULL, 0), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  // Verify array is unchanged
  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Attempt to insert a sequence past the end of the array
TEST(array_tests, insertSequenceAfterArrayEnd) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  const uint8_t newData[] = { 1, 3, 5, 7 };
  const size_t newDataSize = ARRAY_SIZE(newData);

  EXPECT_NE(insertIntoArray(a, arraySize(a) + 1, newData, newDataSize), 0);
  EXPECT_EQ(getArrayStatus(a), ArrayInvalidArgumentError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)),
	    "\"location\" is outside the array");


  // Verify the array is unchanged
  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Attempt to insert a NULL sequence into an array
TEST(array_tests, insertNullIntoArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_NE(insertIntoArray(a, 4, NULL, 16), 0);
  EXPECT_EQ(getArrayStatus(a), ArrayInvalidArgumentError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "\"data\" is NULL");


  // Verify the array is unchanged
  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Attempt to insert a sequence that will cause the array to exceed its max size
TEST(array_tests, insertIntoArrayIncreasingMaximumSize) {
  const size_t initialDataSize = 248;
  uint8_t initialData[initialDataSize];

  for (int i = 0; i < initialDataSize; ++i) {
    initialData[i] = i * 2;
  }
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 256);
  
  const uint8_t newData[] = { 1, 3, 5, 7, 9, 11, 13, 15, 17 };
  const size_t newDataSize = ARRAY_SIZE(newData);

  EXPECT_NE(insertIntoArray(a, 120, newData, newDataSize), 0);
  EXPECT_EQ(getArrayStatus(a), ArraySequenceTooLongError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)),
	    "Inserting 9 bytes into an array of 248 bytes will exceed "
	    "the array's maximum size of 256 bytes");

  
  // Verify the array is unchanged
  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 256);

  destroyArray(a);
}

// Remove a range from the array
TEST(array_tests, removeRangeFromArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(removeFromArray(a, 2, 5), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  const uint8_t result[] = { 2, 4, 16 };
  const size_t resultSize = ARRAY_SIZE(result);

  EXPECT_TRUE(unl_test::verifyArray(a, result, resultSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Remove an empty range from the array
TEST(array_tests, removeEmptyRangeFromArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(removeFromArray(a, 2, 0), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  // Array is unchanged
  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Remove a range whose endpoint is past the end of the array
TEST(array_tests, removeRangeThatEndsOutSideArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(removeFromArray(a, 5, arraySize(a) - 4), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  const uint8_t result[] = { 2, 4, 6, 8, 10 };
  const size_t resultSize = ARRAY_SIZE(result);
  
  EXPECT_TRUE(unl_test::verifyArray(a, result, resultSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Attempt to remove a range whose start is outside the array
TEST(array_tests, removeRangeThatStartsOutSideArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_NE(removeFromArray(a, arraySize(a) + 1, 1), 0);
  EXPECT_EQ(getArrayStatus(a), ArrayInvalidArgumentError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)),
	    "\"location\" is outside the array");

  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Remove a range that starts at the end of the array
TEST(array_tests, removeRangeThatStartsAtArrayEnd) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(removeFromArray(a, arraySize(a), 1), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Remove all data from the array with removeFromArray\ */
TEST(array_tests, removeAllData) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(removeFromArray(a, 0, arraySize(a)), 0);;
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_TRUE(unl_test::verifyArray(a, NULL, 0));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Remove all data from the array with clearArray
TEST(array_tests, clearArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(clearArray(a), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_TRUE(unl_test::verifyArray(a, NULL, 0));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Fill a range in the array with a given byte
TEST(array_tests, fillRangeInArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(fillArray(a, 1, 4, 0), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  const uint8_t result[] = { 2, 0, 0, 0, 10, 12, 14, 16 };
  const size_t resultSize = ARRAY_SIZE(result);

  EXPECT_TRUE(unl_test::verifyArray(a, result, resultSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Fill from the middle to the end
TEST(array_tests, fillArrayToEnd) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(fillArray(a, 4, arraySize(a), 0), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  const uint8_t result[] = { 2, 4, 6, 8, 0, 0, 0, 0 };
  const size_t resultSize = ARRAY_SIZE(result);

  EXPECT_TRUE(unl_test::verifyArray(a, result, resultSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Fill a zero-length range in the array with a given byte
TEST(array_tests, fillZeroLengthRange) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_EQ(fillArray(a, 2, 2, 0), 0);
  EXPECT_EQ(getArrayStatus(a), 0);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "OK");

  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Attempt to fill a range whose end preceeds its start
TEST(array_tests, fillInvalidRange) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_NE(fillArray(a, 5, 4, 0), 0);
  EXPECT_EQ(getArrayStatus(a), ArrayInvalidArgumentError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)), "\"end\" < \"start\"");

  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  destroyArray(a);
}

// Attempt to fill an array whose start or end is outside the erray
TEST(array_tests, fillRangeOutsizeArray) {
  const uint8_t initialData[] = { 2, 4, 6, 8, 10, 12, 14, 16 };
  const size_t initialDataSize = ARRAY_SIZE(initialData);
  Array a = unl_test::createAndInitArray(initialData, initialDataSize, 256);

  ASSERT_NE(a, (void*)0);
  ASSERT_EQ(arraySize(a), initialDataSize);
  ASSERT_EQ(arrayCapacity(a), 16);

  EXPECT_NE(fillArray(a, arraySize(a) + 1, arraySize(a) + 2, 0), 0);
  EXPECT_EQ(getArrayStatus(a), ArrayInvalidArgumentError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)),
	    "\"start\" is outside the array");

  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);

  EXPECT_NE(fillArray(a, arraySize(a) -1, arraySize(a) + 1, 0), 0);
  EXPECT_EQ(getArrayStatus(a), ArrayInvalidArgumentError);
  EXPECT_EQ(std::string(getArrayStatusMsg(a)),
	    "\"end\" is outside the array");

  EXPECT_TRUE(unl_test::verifyArray(a, initialData, initialDataSize));
  EXPECT_EQ(arrayCapacity(a), 16);
  
  destroyArray(a);
}
