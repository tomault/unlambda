#ifndef __FILEIO_H__
#define __FILEIO_H__

#include <stddef.h>

/** Open a file
 *
 *  Arguments:
 *    filename   Name of the file to open
 *    flags      Same as POSIX open()
 *    mode       Same as POSIX open()
 *    errMsg     If an error occurs, this argument will be set to a message
 *                  describing the error.  The caller is responsible for
 *                  freeing the message.  If no error occurs, this argument
 *                  will be set to NULL.
 *
 *  Returns:
 *    If the function succeeds, openFile() returns a file descriptor.  If
 *    the function fails to open the file, it returns a value less than
 *    zero.
 */
int openFile(const char* filename, int flags, int mode, const char** errMsg);

/** Read "n" bytes from a file
 *
 *  Arguments:
 *    filename   Name of the file corresponding to fd
 *    fd         Descriptor for the file to read
 *    buffer     Where to store the data
 *    n          How many bytes to read
 *    errMsg     If an error occurs or the read is incomplete, this argument
 *               will be set to a message describing the error.  The caller
 *               is responsible for freeing the message.  If no error occurs
 *               and the function reads all the data, this argument will be
 *               set to NULL.
 *
 *  Returns:
 *    If the function succeeds and reads all the data, the return value is
 *    zero.  If the function fails to read all the data, the return value is
 *    nonzero and "errMsg" is set to a message describing the error.
 */
int readFromFile(const char* filename, int fd, void* buffer, size_t n,
		 const char** errMsg);

/** Write "n" bytes to a file
 *
 *  Arguments:
 *    filename   Name of the file corresponding to fd
 *    fd         Descriptor for the file to write to
 *    buffer     Data to write
 *    n          Number of bytes to write
 *    errMsg     If an error occurs or the write is incomplete, this argument
 *               will be set to a message describing the error.  The call is
 *               responsible for freeing this message.  If no error occurs and
 *               the function writes all the data, this argument will be set
 *               to NULL.
 *
 *  Returns:
 *    If the function succeeds and writes all the data, the return value is
 *    zero.  If the function fails to write all the data, the return value
 *    is nonzero and "errMsg" is set to a message describing the error.  
 */
int writeToFile(const char* filename, int fd, const void* buffer, size_t n,
		const char** errMsg);

#endif
