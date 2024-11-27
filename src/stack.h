#ifndef __STACK_H__
#define __STACK_H__

#include <stddef.h>
#include <stdint.h>

typedef struct StackImpl_* Stack;

/** Create a new stack
 *
 *  Arguments:
 *    initialSize   Initial size of the stack, in bytes
 *    maxSize       Maximum size of the stack.
 *
 *  Returns:
 *    A new stack instance, or NULL if a stack could not be allocated.
 */
Stack createStack(size_t initialSize, size_t maxSize);

/** Destroy a stack
 *
 *  Destroys the given stack and frees the memory allocated to it.
 *
 *  Arguments:
 *    s   The stack to destroy
 *
 *  Returns:
 *    Nothing
 */
void destroyStack(Stack s);

/** Returns the number of bytes pushed onto the stack */
size_t stackSize(Stack s);

/** Returns the maximum size of the stack, in bytes */
size_t stackMaxSize(Stack s);

/** Returns the number of bytes currently allocated to the stack */
size_t stackAllocated(Stack s);

/** Returns a pointer to the bottom of the stack
 *
 *  The stack grows upward in memory, so bottom < top.
 */
uint8_t* bottomOfStack(Stack s);

/** Returns a pointer to the top of the stack
 *
 *  The stack grows upward in memory, so bottom < top.  Note that the
 *  first element on the stack is at topOfStack(stack)[-1] not
 *  topOfStack(stack)[0].
 */
uint8_t* topOfStack(Stack s);

/** Push "size" bytes onto the stack
 *
 *  Arguments:
 *    s      The stack
 *    item   The data to push
 *    size   How many bytes to push
 *
 *  Returns:
 *    0 if data was successfully pushed onto the stack, or nonzero if the
 *    operation failed.  Use getStackStatus() or getStackStatusMsg() to
 *    obtain a specific error code or message describing the failure.
 */
int pushStack(Stack s, const void* item, size_t size);

/** Pop "size" bytes from the stack
 *
 *  Arguments:
 *    s      The stack
 *    item   If not NULL, where to store the popped bytes.  If NULL, the
 *               popped bytes are discarded.
 *    size   The number of bytes to pop
 *
 *  Returns:
 *    0 if "size" bytes were successfully popped from the stack, or nonzero if
 *    the operation failed.  Use getStackStatus() or getStackStatusMsg() to
 *    obtain a specific error code or message describing the failure.
 *    0 if "size" bytes were successfully popped from the stack, and an error
 *    code if the operation failed.
 */
int popStack(Stack s, void* item, size_t size);

/** Read "size" bytes from the stack top
 *
 *  Arguments:
 *    s     The stack
 *    p     Where to write the data
 *    size  Number of bytes to copy from the stack top
 *
 *  Returns:
 *    0 if successful, or nonzero if the operation failed.  Use
 *    getStackStatus() or getStackStatusMsg() to obtain a specific error code
 *    or message describing the failure.
 */
int readStackTop(Stack s, void* p, size_t size);

/** Swap "size" bytes at the top of the stack with the next "size" bytes
 *  on the stack.
 *
 *  Arguments:
 *    s     The stack
 *    size  Number of bytes to swap
 *
 *  Returns:
 *    0 if successful, or nonzero if the operation failed.  Use
 *    getStackStatus() or getStackStatusMsg() to obtain a specific error code
 *    or message describing the failure.
 */
int swapStackTop(Stack s, size_t size);

/** Duplicate the "size" bytes at the top of the stack and push them onto
 *  the stack.
 * 
 *  Arguments:
 *    s     The stack
 *    size  The number of bytes to duplicate
 *
 *  Returns:
 *    0 if successful, or nonzero if the operation failed.  Use
 *    getStackStatus() or getStackStatusMsg() to obtain a specific error code
 *    or message describing the failure.
 */ 
int dupStackTop(Stack s, size_t size);

/** Pop all data from the stack, leaving it empty
 *
 *  Arguments:
 *    s     The stack
 * 
 *  Returns:
 *    Nothing
 */
void clearStack(Stack s);

/** Set the contents of the stack.
 *
 *  Replace the contents of the stack with that of ["data", "data" + "size"),
 *  resizing the stack if necessary.  This is effectively the same as
 *  popping the entire content of the stack then pushing "size" bytes
 *  from "data."
 *
 *  Arguments:
 *    s      The stack
 *    data   Data to put onto the stack
 *    size   Number of bytes to put
 *
 *  Returns
 *    0 if successful, or nonzero if the operation failed.  Use
 *    getStackStatus() or getStackStatusMsg() to obtain a specific error code
 *    or message describing the failure.
 */
int setStack(Stack s, const uint8_t* data, uint64_t size);

/** Return an error code for the last stack operation.
 * 
 *  Will be zero if no error occurred
 */
int getStackStatus(Stack s);

/** Return an error message describing why the last operation failed.
 *
 *  Will return the string "OK" if the last operation succeeded.  The
 *  stack owns the error message and will manage its lifetime.
 */
const char* getStackStatusMsg(Stack s);

/** Clear the last error that occurred on the stack
 *
 *  After calling clearStackStatus(s), getStackStatus(s) will return 0
 *  (no error) and getStackStatusMsg(s) will return "OK"
 */
void clearStackStatus(Stack s);

/** Stack operation error codes returned by getStackStatus() */

#ifdef __cplusplus
/** Maximum stack size exceeded */
const int StackOverflowError = -1;

/** Could not allocate memory */
const int StackMemoryAllocationFailedError = -2;

/** Not enough data on the stack for the operation.
 * 
 *  Popping, swapping or duplicating more data than is currently on the
 *  stack all cause this error
 */
const int StackUnderflowError = -3;

/** An argument passed to one of the stack manipulation functions is invalid */
const int StackInvalidArgumentError = -4;
#else
/** Maximum stack size exceeded */
const int StackOverflowError;

/** Could not allocate memory */
const int StackMemoryAllocationFailedError;

/** Not enough data on the stack for the operation.
 * 
 *  Popping, swapping or duplicating more data than is currently on the
 *  stack all cause this error
 */
const int StackUnderflowError;

/** An argument passed to one of the stack manipulation functions is invalid */
const int StackInvalidArgumentError;
#endif

#endif
