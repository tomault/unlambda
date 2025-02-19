#ifndef __VM_H__
#define __VM_H__

#include <logging.h>
#include <stdint.h>
#include <stack.h>
#include <symtab.h>
#include <vmmem.h>

/** The Unlambda virtual machine itself */

typedef struct UnlambdaVmImpl_* UnlambdaVM;

/** Create an Unlambda virtual machine instance
 *
 *  Arguements:
 *    maxCallStackSize      Maximum number of entries on the call stack.
 *    maxAddressStackSize   Maximum number of entries on the address stack
 *    initialMemorySize     Initial size of the VM's memory, in bytes
 *    maxMemorySize         Maximum size of the VM's memory, in bytes
 *
 *  Returns
 *    A new UnlambdaVM instance, or NULL if one could not be created
 */
UnlambdaVM createUnlambdaVM(uint32_t maxCallStackSize,
			    uint32_t maxAddressStackSize,
			    uint64_t initialMemorySize,
			    uint64_t maxMemorySize);

/** Destroy and Unlambda VM and release all the resources it owns */
void destroyUnlambdaVM(UnlambdaVM vm);

/** Get a code describing the outcome of the last operation performed on
 *  a virtual machine
 *
 *  Arguments:
 *    vm   The virtual machine
 *
 *  Returns:
 *    0 if the last operation succeeded, or an error code describing the
 *    failure if it did not.
 */
int getVmStatus(UnlambdaVM vm);

/** Get a message describing the outcome of the last operation performed on
 *  the virtual machine
 *
 *  Arguments:
 *    vm   The virtual machine
 *
 *  Returns:
 *    The message "OK" if the last operation succeeded, or an error message
 *    describing the failure if it did not.
 */
const char* getVmStatusMsg(UnlambdaVM vm);

/** Reset the VM's last operation code to 0/"OK" */
void clearVmStatus(UnlambdaVM vm);

/** Get the name of the currently loaded program
 *
 *  Returns an empty string if no program is loaded
 */
const char* getVmProgramName(UnlambdaVM vm);

/** Get the address of the program counter */
uint64_t getVmPC(UnlambdaVM vm);

/** Set the VM's program counter */
int setVmPC(UnlambdaVM vm, uint64_t address);

/** Get the VM's call stack */
Stack getVmCallStack(UnlambdaVM vm);

/** Get the VM's address stack */
Stack getVmAddressStack(UnlambdaVM vm);

/** Get the VM's symbol table.  Useful for debugging */
SymbolTable getVmSymbolTable(UnlambdaVM vm);

/** Get the VM's memory */
VmMemory getVmMemory(UnlambdaVM vm);

/** Get a pointer to the location of the PC in the VM's memory
 *
 *  Equivalent to ptrToVmAddress(vm, getVmPC(vm));
 */
uint8_t* ptrToVmPC(UnlambdaVM vm);

/** Get a pointer to a location in the VM's memory
 *
 *  Arguements:
 *    vm        The virtual machine
 *    address   Address to get a pointer to
 *
 *  Returns:
 *    A pointer to the given address in the VM's memory, if it exists, and
 *    NULL if it does not.
 */
uint8_t* ptrToVmAddress(UnlambdaVM vm, uint64_t address);

/** Load a program from disk into the VM's memory for execution
 *
 *  Each VM can only load a program once.  To execute another program,
 *  destroy the current VM, create a new one, and load that program into
 *  the VM.
 *
 *  The program file contains an Unlambda VM program image, typically
 *  produced by the Unlambda compiler "unlc."
 *
 *  Arguments:
 *    vm            The virtual machine
 *    filename      Name of the file with the program to load
 *    loadSymbols   If nonzero, load debugging symbols.
 *
 *  Returns:
 *    0 on success, and a nonzero value on failure.  Use getVmStatus() or
 *    getVmStatusMsg() to obtain a specific error code or message describing
 *    the failure.
 */
int loadProgramIntoVm(UnlambdaVM vm, const char* filename, int loadSymbols);

/** Load a program from memory into the VM's memory for execution
 *
 *  Each VM can only load a program once.  To execute another program,
 *  destroy the current VM, create a new one, and load that program into
 *  the VM.
 *
 *  If the program area in the VM's memory hasn't been configured yet,
 *  loadVmProgramFromMemory() will configure it to be large enough to
 *  hold the program.  If the program area has been configured but is
 *  too small, loadVmProgramFromMemory will report a VmIllegalArgumentError
 *
 *  TODO: Add the ability to set the symbol table.
 *
 *  Arguments:
 *    vm            The VM to load the program into
 *    program       The program to load
 *    programSize   The size of the program to load
 */
int loadVmProgramFromMemory(UnlambdaVM vm, const char* name,
			    const uint8_t* program, uint64_t programSize);

/** Execute one instruction
 *
 *  A program must be loaded into the VM before calling this function
 *  (obviously).
 *
 *  Arguments:
 *    vm   The virtual machine
 *
 *  Returns:
 *    0 on success, nonzero on failure.  
 */
int stepVm(UnlambdaVM vm);

/** Set the VM's logger
 *
 *  Also sets the logger the VM's memory uses to this logger.
 *
 *  Setting the logger to NULL disables logging.
 */
void setVmLogger(UnlambdaVM vm, Logger logger);

/** Get the VM's logger */
Logger getVmLogger(UnlambdaVM vm);

/** Status codes returned by getVmStatus() */
#ifdef __cplusplus

/** Indicates a program is already loaded */
const int VmProgramAlreadyLoadedError = -1;

/** Indicates an I/O error occured while executing an operation such as
 *  loading a program.
 *
 *  Consult errno for more details
 */
const int VmIOError = -2;

/** The program image loadProgramIntoVm() tried to load was malformed in
 *  some way and could not be loaded
 */
const int VmBadProgramImageError = -3;

/** The VM ran out of memory
 *
 *  This could occur when loading a program, if the program is too big
 *  to fit into memory, or during program execution, if the program
 *  runs out of heap.
 */
const int VmOutOfMemoryError = -4;

/** The VM has halted
 *
 *  Returned if the VM has executed a HALT instruction
 */
const int VmHalted = -5;

/** The VM has halted becuase it executed a PANIC instruction */
const int VmPanicError = -6;

/** The VM attempted to execute an illegal instruction */
const int VmIllegalInstructionError = -7;

/** The VM attempted to read or write to an illegal or out-of-bounds address */
const int VmIllegalAddressError = -8;

/** The VM call stack has underflowed */
const int VmCallStackUnderflowError = -9;

/** The VM call stack has overflowed (exceeded its maximum size) */
const int VmCallStackOverflowError = -10;

/** The VM address stack has underflowed */
const int VmAddressStackUnderflowError = -11;

/** The VM address stack has overflowed */
const int VmAddressStackOverflowError = -12;

/** No program loaded yet */
const int VmNoProgramLoadedError = -13;

/** The VM has encountered a fatal error */
const int VmFatalError = -14;

/** One of the arguments to a function is invalid */
const int VmIllegalArgumentError = -15;

#else

/** Indicates a program is already loaded */
const int VmProgramAlreadyLoadedError;

/** Indicates an I/O error occured while executing an operation such as
 *  loading a program.
 *
 *  Consult errno for more details
 */
const int VmIOError;

/** The program image loadProgramIntoVm() tried to load was malformed in
 *  some way and could not be loaded
 */
const int VmBadProgramImageError;

/** The VM ran out of memory
 *
 *  This could occur when loading a program, if the program is too big
 *  to fit into memory, or during program execution, if the program
 *  runs out of heap.
 */
const int VmOutOfMemoryError;

/** The VM has halted
 *
 *  Returned if the VM has executed a HALT instruction
 */
const int VmHalted;

/** The VM has halted becuase it executed a PANIC instruction */
const int VmPanicError;

/** The VM attempted to execute an illegal instruction */
const int VmIllegalInstructionError;

/** The VM attempted to read or write to an illegal or out-of-bounds address */
const int VmIllegalAddressError;

/** The VM call stack has underflowed */
const int VmCallStackUnderflowError;

/** The VM call stack has overflowed (exceeded its maximum size) */
const int VmCallStackOverflowError;

/** The VM address stack has underflowed */
const int VmAddressStackUnderflowError;

/** The VM address stack has overflowed */
const int VmAddressStackOverflowError;

/** No program loaded yet */
const int VmNoProgramLoadedError;

/** The VM has encountered a fatal error */
const int VmFatalError;

/** One of the arguments to a function is invalid */
const int VmIllegalArgumentError;


#endif

#endif
