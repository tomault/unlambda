#ifndef __VM__IMAGE_H__
#define __VM__IMAGE_H__

#include "symtab.h"
#include "vm.h"

/** Read the header of a program image */
int loadVmProgramHeader(const char* filename, uint64_t* programSize,
			uint64_t* numSymbols, uint64_t* startAddress,
			const char** errMsg);

/** Load program from the given file into the VM
 *
 *  Will fail if a program is already loaded into the VM
 *
 *  Arguments
 *    filename      File to load the program from
 *    vm            VM to load it into
 *    loadSymbols   Load debugging symbols if nonzero, omit them if zero
 *    errMsg        If not NULL and an error occurs, upon return this will
 *                    contain a message describing the error.  Will be set
 *                    to NULL if the operation succeeds.
 *
 *  Returns:
 *    0 on success or one of the VmImage* error codes if the program could
 *    not be loaded.
 */
int loadVmProgramImage(const char* filename, UnlambdaVM vm,
		       int loadSymbols, uint64_t* startAddress,
		       const char** errMsg);

/** Save a program and (optionally) its debugging sybmols
 *
 *  Arguments:
 *    filename       File to save the program to
 *    program        The program itself
 *    programSize    Program size, in bytes
 *    symbols        Debugging symbols.  If NULL, no symbols are written
 *    errMsg         If not NULL and an error occurs, upon return this
 *                     argument will contain a message describing the error.
 *                     Will be set to NULL if the operation succeeds.
 *
 *  Returns:
 *    0 on success or one of the VmImage* error codes if the program
 *      could not be saved
 */
int saveVmProgramImage(const char* filename, const uint8_t* program,
		       uint64_t programSize, uint64_t startAddress,
		       SymbolTable symbols, const char** errMsg);

#ifdef __cplusplus
/** One of the arguments to the function is invalid */
const int VmImageIllegalArgumentError = -1;

/** Indicates a VM already has a program loaded into it */
const int VmImageProgramAlreadyLoadedError = -2;

/** An I/O error occured while loading or saving a program.  Consult errno
 *  for more details.
 */
const int VmImageIOError = -3;

/** The program image file was malformed and could not be loaded */
const int VmImageFormatError = -4;

/** Program would not fit into the VM's memory
 *
 *  The current implementation of VmMemory does not increase the memory
 *  size when asked to accomodate a program larger than its current size.
 *  This will be fixed in the future.
 */
const int VmImageOutOfMemoryError =-5;

#else

/** One of the arguments to the function is invalid */
const int VmImageIllegalArgumentError;

/** Indicates a VM already has a program loaded into it */
const int VmImageProgramAlreadyLoadedError;

/** An I/O error occured while loading or saving a program.  Consult errno
 *  for more details.
 */
const int VmImageIOError;

/** The program image file was malformed and could not be loaded */
const int VmImageFormatError;

/** Program would not fit into the VM's memory
 *
 *  The current implementation of VmMemory does not increase the memory
 *  size when asked to accomodate a program larger than its current size.
 *  This will be fixed in the future.
 */
const int VmImageOutOfMemoryError;

#endif

#endif
