extern "C" {
#include <stack.h>
}

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

TEST(stack_tests, createStack) {
  Stack s = createStack(1024, 4096);

  ASSERT_NE(s, (void*)0);
  EXPECT_EQ(stackSize(s), 0);
  EXPECT_EQ(stackMaxSize(s), 4096);
  EXPECT_EQ(stackAllocated(s), 1024);
  EXPECT_EQ(bottomOfStack(s), topOfStack(s));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK"); 

  destroyStack(s);
}

TEST(stack_tests, createStackWithInvalidSize) {
  Stack s = createStack(0, 0);
  EXPECT_EQ(s, (void*)0);

  s = createStack(1024, 1023);
  EXPECT_EQ(s, (void*)0);
}

TEST(stack_tests, pushValues) {
  Stack s = createStack(0, 32);
  uint64_t value = 0x1111111111111111;

  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), sizeof(value));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK"); 
  
  value = 0x2222222222222222;
  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), 2 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 2 * sizeof(value));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK"); 

  value = 0x3333333333333333;
  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 3 * sizeof(value));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK"); 

  value = 0x4444444444444444;
  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), 4 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 4 * sizeof(value));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK"); 

  uint64_t* data = (uint64_t*)topOfStack(s);
  EXPECT_EQ(data[-1], 0x4444444444444444);
  EXPECT_EQ(data[-2], 0x3333333333333333);
  EXPECT_EQ(data[-3], 0x2222222222222222);
  EXPECT_EQ(data[-4], 0x1111111111111111);

  destroyStack(s);
}

TEST(stack_tests, pushEmptyValue) {
  Stack s = createStack(0, 32);
  uint64_t value = 0x1111111111111111;  

  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), sizeof(value));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK"); 

  EXPECT_EQ(pushStack(s, &value, 0), 0);
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), sizeof(value));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK"); 

  uint64_t* data = (uint64_t*)topOfStack(s);
  EXPECT_EQ(data[-1], 0x1111111111111111);

  destroyStack(s);
}

TEST(stack_tests, pushNull) {
  Stack s = createStack(0, 32);

  EXPECT_NE(pushStack(s, NULL, 8), 0);
  EXPECT_EQ(stackSize(s), 0);
  EXPECT_EQ(stackAllocated(s), 0);
  EXPECT_EQ(topOfStack(s), bottomOfStack(s));
  EXPECT_EQ(getStackStatus(s), StackInvalidArgumentError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "\"item\" is NULL");

  destroyStack(s);
}

TEST(stack_tests, pushCausingOverflow) {
  Stack s = createStack(0, 8);
  uint64_t value = 0x1111111111111111;

  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 8);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), sizeof(value));
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  value = 0xFF;
  EXPECT_NE(pushStack(s, &value, 1), 0);
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 8);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), sizeof(value));
  EXPECT_EQ(getStackStatus(s), StackOverflowError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)),
	    "Stack overflow - increasing the size of the stack by 1 bytes "
	    "would exceed the maximum size of 8 bytes");

  destroyStack(s);  
}

TEST(stack_tests, popValues) {
  Stack s = createStack(0, 16);
  uint64_t value = 0x0123456789ABCDEF;

  // Push two values onto the stack
  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  EXPECT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 2 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 16);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 16);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-2], 0x0123456789ABCDEF);
  
  // Pop & verify the two values
  value = 0;

  EXPECT_EQ(popStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(value, 0xFEDCBA9876543210);
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 8);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  EXPECT_EQ(popStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(value, 0x0123456789ABCDEF);
  EXPECT_EQ(stackSize(s), 0);
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  destroyStack(s);
}

TEST(stack_tests, popAndThrowAwayValues) {
  Stack s = createStack(0, 16);
  uint64_t value = 0x0123456789ABCDEF;

  // Push two values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 2 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 16);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 16);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-2], 0x0123456789ABCDEF);
  
  // Pop & verify the two values
  value = 0;

  EXPECT_EQ(popStack(s, NULL, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 8);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  EXPECT_EQ(popStack(s, NULL, sizeof(value)), 0);
  EXPECT_EQ(stackSize(s), 0);
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  destroyStack(s);
}

TEST(stack_tests, popEmptyValue) {
  Stack s = createStack(0, 16);
  uint64_t value = 0x0123456789ABCDEF;

  // Push two values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 2 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 16);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 16);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-2], 0x0123456789ABCDEF);

  // Popping a value of size 0 should leave the stack unchanged
  value = 0;

  EXPECT_EQ(popStack(s, &value, 0), 0);
  EXPECT_EQ(value, 0);
  EXPECT_EQ(stackSize(s), 2 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 16);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  destroyStack(s);
}

TEST(stack_tests, popCausingOverflow) {
  Stack s = createStack(0, 16);
  uint64_t value = 0x0123456789ABCDEF;

  // Push two values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 2 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 16);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 16);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-2], 0x0123456789ABCDEF);
  
  // Pop two values & verify the stack is empty
  value = 0;

  ASSERT_EQ(popStack(s, NULL, sizeof(value)), 0);
  ASSERT_EQ(popStack(s, NULL, sizeof(value)), 0);
  ASSERT_EQ(stackSize(s), 0);
  ASSERT_EQ(stackAllocated(s), 16);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 0);
  ASSERT_EQ(getStackStatus(s), 0);
  ASSERT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Popping a third value should cause an underflow
  EXPECT_NE(popStack(s, NULL, 1), 0);
  // Verify the stack state hasn't changed
  EXPECT_EQ(stackSize(s), 0);
  EXPECT_EQ(stackAllocated(s), 16);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 0);
  EXPECT_EQ(getStackStatus(s), StackUnderflowError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)),
	    "Cannot pop 1 bytes from a stack with only 0 bytes on it");

  destroyStack(s);
}

TEST(stack_tests, readFromTop) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Read the top of the stack into a buffer
  uint64_t buffer[2];
  EXPECT_EQ(readStackTop(s, buffer, sizeof(buffer)), 0);
  EXPECT_EQ(buffer[0], 0xFEDCBA9876543210);
  EXPECT_EQ(buffer[1], 0x01234567FEDCBA98);

  // Stack should not change
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  EXPECT_EQ(getStackStatus(s), 0);

  destroyStack(s);
}

TEST(stack_tests, readIntoNull) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Try to read into NULL
  EXPECT_NE(readStackTop(s, NULL, 1), 0);
  EXPECT_EQ(getStackStatus(s), StackInvalidArgumentError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "\"p\" is NULL");

  // Stack should not change
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  destroyStack(s);
}

TEST(stack_tests, readTooMuch) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Try to read 25 bytes from a stack of size 24
  uint8_t buffer[25];
  EXPECT_NE(readStackTop(s, buffer, sizeof(buffer)), 0);
  EXPECT_EQ(getStackStatus(s), StackUnderflowError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)),
	    "Cannot read 25 bytes from a stack with only 24 bytes on it");

  // Stack should not change
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  destroyStack(s);
}

TEST(stack_tests, swapStackTop) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Swap the top two
  EXPECT_EQ(swapStackTop(s, 8), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the swap took place correctly
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  // A swap should never cause the stack to relocate
  ASSERT_EQ(p, (uint64_t*)topOfStack(s));

  EXPECT_EQ(p[-1], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-2], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-3], 0x0123456789ABCDEF);

  destroyStack(s);
}

TEST(stack_tests, swapNothing) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Swap zero bytes
  EXPECT_EQ(swapStackTop(s, 0), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the stack is unchanged
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  ASSERT_EQ(p, (uint64_t*)topOfStack(s));

  EXPECT_EQ(p[-1], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-2], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-3], 0x0123456789ABCDEF);

  destroyStack(s);
}

TEST(stack_tests, swapTooMuch) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Swap 13 bytes, which is one byte too many
  EXPECT_NE(swapStackTop(s, 13), 0);
  EXPECT_EQ(getStackStatus(s), StackUnderflowError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)),
	    "Cannot swap the top 13 bytes on a stack that only has 24 bytes");

  
  // The stack should not have changed
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  ASSERT_EQ(p, (uint64_t*)topOfStack(s));
  EXPECT_EQ(p[-1], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-2], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-3], 0x0123456789ABCDEF);

  destroyStack(s);
}

TEST(stack_tests, duplicateStackTop) {
  Stack s = createStack(0, 40);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 32);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Duplicate the top entry
  EXPECT_EQ(dupStackTop(s, 8), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the stack is correct
  EXPECT_EQ(stackSize(s), 4 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 32);

  p = (uint64_t*)topOfStack(s);
  EXPECT_EQ(p[-1], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-2], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-3], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-4], 0x0123456789ABCDEF);

  // TODO: Push a value to verify writes to the stack still occur correctly
  value = 0xFFEEDDCCBBAA9988;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the stack is correct
  EXPECT_EQ(stackSize(s), 5 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 40);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 40);

  p = (uint64_t*)topOfStack(s);
  EXPECT_EQ(p[-1], 0xFFEEDDCCBBAA9988);
  EXPECT_EQ(p[-2], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-3], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-4], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-5], 0x0123456789ABCDEF);

  destroyStack(s);
}

TEST(stack_tests, duplicateNothing) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Duplicate nothing
  EXPECT_EQ(dupStackTop(s, 0), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the stack hasn't changed
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  ASSERT_EQ(p, (uint64_t*)topOfStack(s));
  EXPECT_EQ(p[-1], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-2], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-3], 0x0123456789ABCDEF);

  destroyStack(s);
}

TEST(stack_tests, duplicateTooMuch) {
  Stack s = createStack(0, 50);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 32);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Try to duplicate 25 bytes
  EXPECT_NE(dupStackTop(s, 25), 0);
  EXPECT_EQ(getStackStatus(s), StackUnderflowError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)),
	    "Cannot duplicate 25 bytes on a stack that has only 24 bytes");

  // Verify stack has not changed
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  ASSERT_EQ(p, (uint64_t*)topOfStack(s));
  EXPECT_EQ(p[-1], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-2], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-3], 0x0123456789ABCDEF);

  destroyStack(s);
}

TEST(stack_tests, duplicateCausingOverflow) {
  Stack s = createStack(0, 32);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 32);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Try to duplicate 16 bytes, which will cause a stack overflow
  EXPECT_NE(dupStackTop(s, 16), 0);
  EXPECT_EQ(getStackStatus(s), StackOverflowError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)),
	    "Stack overflow - increasing the size of the stack by 16 bytes "
	    "would exceed the maximum size of 32 bytes");

  // Stack should not have changed
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  ASSERT_EQ(p, (uint64_t*)topOfStack(s));
  EXPECT_EQ(p[-1], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-2], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-3], 0x0123456789ABCDEF);

  destroyStack(s);
}

TEST(stack_tests, clearStack) {
  Stack s = createStack(0, 32);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 32);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  clearStack(s);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  EXPECT_EQ(stackSize(s), 0);
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s), bottomOfStack(s));

  // TODO: Push a value to verify writes to the stack still occur correctly
  value = 0xFFEEDDCCBBAA9988;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the stack is correct
  EXPECT_EQ(stackSize(s), sizeof(value));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 8);

  p = (uint64_t*)topOfStack(s);
  EXPECT_EQ(p[-1], 0xFFEEDDCCBBAA9988);

  destroyStack(s);
}

TEST(stack_tests, setStackData) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Change the stack to have 16 bytes of new data on it
  uint64_t newData[2] = { 0xDEADBEEF01234567, 0xFEEDBABE99887766 };
  EXPECT_EQ(setStack(s, (uint8_t*)newData, sizeof(newData)), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the new stack
  EXPECT_EQ(stackSize(s), sizeof(newData));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 16);

  p = (uint64_t*)topOfStack(s);
  EXPECT_EQ(p[-1], 0xFEEDBABE99887766);
  EXPECT_EQ(p[-2], 0xDEADBEEF01234567);

  // TODO: Push a value to verify writes to the stack still occur correctly
  value = 0xFFEEDDCCBBAA9988;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the stack is correct
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  p = (uint64_t*)topOfStack(s);
  EXPECT_EQ(p[-1], 0xFFEEDDCCBBAA9988);
  EXPECT_EQ(p[-2], 0xFEEDBABE99887766);
  EXPECT_EQ(p[-3], 0xDEADBEEF01234567);

  destroyStack(s);
}

TEST(stack_tests, setStackIncreasingAllocatedMemory) {
  Stack s = createStack(0, 32);
  uint64_t value = 0x0123456789ABCDEF;

  // Push one value
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack state
  ASSERT_EQ(stackSize(s), sizeof(value));
  ASSERT_EQ(stackAllocated(s), 16);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 8);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x0123456789ABCDEF);

  uint64_t newData[3] = { 0xDEADBEEF01234567, 0xFEEDBABE99887766,
			  0x1122334455667788 };
  EXPECT_EQ(setStack(s, (uint8_t*)newData, sizeof(newData)), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the new stack state
  EXPECT_EQ(stackSize(s), sizeof(newData));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  p = (uint64_t*)topOfStack(s);
  EXPECT_EQ(p[-1], 0x1122334455667788);
  EXPECT_EQ(p[-2], 0xFEEDBABE99887766);
  EXPECT_EQ(p[-3], 0xDEADBEEF01234567);

  // TODO: Push a value to verify writes to the stack still occur correctly
  value = 0xFFEEDDCCBBAA9988;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);
  EXPECT_EQ(getStackStatus(s), 0);
  EXPECT_EQ(std::string(getStackStatusMsg(s)), "OK");

  // Verify the stack is correct
  EXPECT_EQ(stackSize(s), 4 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 32);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 32);

  p = (uint64_t*)topOfStack(s);
  EXPECT_EQ(p[-1], 0xFFEEDDCCBBAA9988);
  EXPECT_EQ(p[-2], 0x1122334455667788);
  EXPECT_EQ(p[-3], 0xFEEDBABE99887766);
  EXPECT_EQ(p[-4], 0xDEADBEEF01234567);

  destroyStack(s);
}

TEST(stack_tests, setStackDataCausingOverflow) {
  Stack s = createStack(0, 24);
  uint64_t value = 0x0123456789ABCDEF;

  // Push three values onto the stack
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0xFEDCBA9876543210;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  value = 0x01234567FEDCBA98;
  ASSERT_EQ(pushStack(s, &value, sizeof(value)), 0);

  // Verify the stack is set up correctly
  ASSERT_EQ(stackSize(s), 3 * sizeof(value));
  ASSERT_EQ(stackAllocated(s), 24);
  ASSERT_EQ(topOfStack(s) - bottomOfStack(s), 24);
  ASSERT_EQ(getStackStatus(s), 0);

  uint64_t* p = (uint64_t*)topOfStack(s);
  ASSERT_EQ(p[-1], 0x01234567FEDCBA98);
  ASSERT_EQ(p[-2], 0xFEDCBA9876543210);
  ASSERT_EQ(p[-3], 0x0123456789ABCDEF);

  // Try to change the stack so it now contains 32 bytes
  uint64_t newData[4] = { 0xDEADBEEF01234567, 0xFEEDBABE99887766,
			  0x1122334455667788, 0x99AABBCCDDEEFF00 };
  EXPECT_NE(setStack(s, (uint8_t*)newData, sizeof(newData)), 0);
  EXPECT_EQ(getStackStatus(s), StackOverflowError);
  EXPECT_EQ(std::string(getStackStatusMsg(s)),
	    "Stack overflow - increasing the size of the stack by 32 bytes "
	    "would exceed the maximum size of 24 bytes");

  // Stack should not change
  EXPECT_EQ(stackSize(s), 3 * sizeof(value));
  EXPECT_EQ(stackAllocated(s), 24);
  EXPECT_EQ(topOfStack(s) - bottomOfStack(s), 24);

  ASSERT_EQ(p, (uint64_t*)topOfStack(s));
  EXPECT_EQ(p[-1], 0x01234567FEDCBA98);
  EXPECT_EQ(p[-2], 0xFEDCBA9876543210);
  EXPECT_EQ(p[-3], 0x0123456789ABCDEF);

  destroyStack(s);
}
