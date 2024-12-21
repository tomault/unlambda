#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <stddef.h>
#include <stdint.h>

/** A dynamically-sized array */
typedef struct ArrayImpl_* Array;

/** Create a new array
 *
 *  Arguments:
 *    initialSize   The initial size of the array in bytes.  Data in the
 *                    range [0, initialSize) will be randomly-initialized.
 *    maxSize       Maximum size of the array, in bytes
 *
 *  Returns:
 *    A new array, or NULL if an error occurred
 */
Array createArray(size_t initialSize, size_t maxSize);

/** Destroy an array, releasing all the memory it uses
 *
 *  Arguments:
 *    array   The array to destroy
 */
void destroyArray(Array array);

/** Return the status code for the last array operation
 *
 *  Arguments:
 *    array  The array whose operation status will be returned
 *
 *  Returns:
 *    0 if the last operation was successful or one of the (nonzero)
 *    Array* status codes.
 */
int getArrayStatus(Array array);

/** Return a status message for the last array operation
 *
 *  Arguments:
 *    array  The array whose operation status will be returned
 *
 *  Returns:
 *    "OK" if the last operation succeeded or an error message describing
 *    why the last operation failed.
 */
const char* getArrayStatusMsg(Array array);

/** Reset the array status to 0/"OK" */
void clearArrayStatus(Array array);

/** Return the current size of the array */
size_t arraySize(Array array);

/** Return the maximum size of the array */
size_t arrayMaxSize(Array array);

/** Return the number of bytes of storage allocated for the array */
size_t arrayCapacity(Array array);

/** Return a pointer to the start of the array */
uint8_t* startOfArray(Array array);

/** Return a pointer to the end of the array
 *
 *  Note that endOfArray(array) - startOfArray(array) == arraySize(array)
 */
uint8_t* endOfArray(Array array);

/** Return a pointer to a given position in an array
 *
 *  Returns:
 *    A pointer to "index" in "array" or NULL if "index" is greater
 *    than or equal to arraySize(array).  Does not change the array's
 *    operation status.
 */
uint8_t* ptrToArrayIndex(Array array, size_t index);

/** Return the value at "index" in "array"
 *
 *  Arguments:
 *    array  Array to query
 *    index  Index of value to return.  Must be between 0 (inclusive) and
 *             arraySize(array) (exclusive).
 *
 *  Returns:
 *    The value at array[index].  If "index" is invalid, returns 0 and
 *    sets the array's error status.
 */
uint8_t valueAtArrayIndex(Array array, size_t index);

/** Find the first occurence of a specific value in [start, end) 
 *
 *  Arguments:
 *    array  The array to search
 *    start  Start searching at this position
 *    end    End searching one before this position
 *    value  Value to find
 *
 *  Returns:
 *    A pointer to the first occurence of "value" in [start, end), or NULL
 *    if "value" does not occur in [start, end).
 */
uint8_t* findValueInArray(Array array, size_t start, size_t end, uint8_t value);

/** Find the first occurence of a specific sequence of bytes in [start, end)
 *
 *  Arguments:
 *    array  The array to search
 *    start  Start searching at this position
 *    end    End searching one before this position
 *    seq    Sequence to find
 *    size   Length of sequence to find, in bytes.  A zero-length sequence
 *             is never found in the array (e.g. always returns NULL)
 *
 *  Returns:
 *    A pointer to the first occurence of "seq" in [start, end) or NULL if
 *    no such sequence occurs.  The sequence must be completely contained
 *    in [start, end) to be found.
 */
uint8_t* findSeqInArray(Array array, size_t start, size_t end,
			const uint8_t* seq, size_t size);

/** Append a sequence of bytes to the array
 *
 *  Increases the size of the array by "size" bytes, but fails if that would
 *  increase the size beyond the array's maximum size.
 *
 *  Arguments:
 *    array  The array to append "seq" to
 *    seq    The sequence of bytes to append
 *    size   The number of bytes in "seq"
 *
 *  Returns:
 *    0 on success or nonzero on failure.  If the operation fails, it will
 *    set the array's status code and message to values describing what
 *    went wrong.
 */
int appendToArray(Array array, const uint8_t* seq, size_t size);

/** Insert a sequence of bytes into the array at a specific location
 *
 *  Increases the size of the array by "size" bytes, but fails if that would
 *  increase the size beyond the array's maximum size.
 *
 *  Arguments:
 *    array     The array to append "seq" to
 *    location  Where to insert the sequence
 *    seq       The sequence of bytes to append
 *    size      The number of bytes in "seq"
 *
 *  Returns:
 *    0 on success or nonzero on failure.  If the operation fails, it will
 *    set the array's status code and message to values describing what
 *    went wrong.
 */
int insertIntoArray(Array array, size_t location, const uint8_t* value,
		    size_t size);

/** Remove a sequence of bytes from the array
 *
 *  If location + size > arraySize(array), then "size" is clamped to 
 *  arraySize(array) - location.  Decreases the size of the array by
 *  "size" bytes and removes the bytes in the range [location, 
 *  location + size).
 *
 *  Arguments:
 *    array     Array to remove bytes from
 *    location  Where to start removing bytes
 *    size      Number of bytes to remove.
 *
 *  Returns:
 *    0 on success or nonzero on failure.  If the operation fails, it will
 *    set the array's status code and message to values describing what
 *    went wrong.
 */
int removeFromArray(Array array, size_t location, size_t size);

/** Remove all data from the array, leaving it empty
 *
 *  This is the same as removeFromArray(array, 0, arraySize(array)).
 *
 *  Arguments:
 *    array   The array to clear
 *
 *  Returns:
 *    0 on success or nonzero on failure.  If the operation fails, it will
 *    set the array's status code and message to values describing what
 *    went wrong.
 */
int clearArray(Array array);

/** Fill the array from [start, end) with a specific value
 *
 *  Arguments:
 *    array  The array to fill
 *    start  Where to start filling
 *    end    One past the location to stop filling
 *    value  Value to fill [start, end) with.
 *
 *  Returns:
 *    0 on success or nonzero on failure.  If the operation fails, it will
 *    set the array's status code and message to values describing what
 *    went wrong.
 */
int fillArray(Array array, size_t start, size_t end, uint8_t value);

#ifndef __cplusplus
/** An argument to an array operation is invalid */
const int ArrayInvalidArgumentError;

/** Attempt to add a value to an array that would cause it to exceed its
 *  its maximum size
 */
const int ArraySequenceTooLongError;

/** Could not allocate memory to increase array size */
const int ArrayOutOfMemoryError;

#else

/** An argument to an array operation is invalid */
const int ArrayInvalidArgumentError = -1;

/** Attempt to add a value to an array that would cause it to exceed its
 *  its maximum size
 */
const int ArraySequenceTooLongError = -2;

/** Could not allocate memory to increase array size */
const int ArrayOutOfMemoryError = -3;

#endif

#endif
