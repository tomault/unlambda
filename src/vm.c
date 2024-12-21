/**   Imlementation of the Unlambda virtual machine
 *
 *    The Unlambda VM ("UVM") is a stack machine with two stacks:
 *    * An address stack that holds addresses of functions and saved VM states
 *      on the heap.
 *    * A call stack that contains the return addresses for function calls
 *
 *    VM instructions manipulate one of these two stacks and potentially
 *    allocate memory on the heap.  The UVM uses a garbage collector to
 *    identify and free objects on the heap that are no longer reachable.
 *
 *    To facilitate efficient garbage collection, return "addresses" on the
 *    call stack actually consist of a pair of addresses - the actual return
 *    address and the address of the function called.  The garbage collector
 *    uses the latter to identify code blocks on the heap that are still
 *    reachable and should not be collected.  This is an implementation
 *    detail and invisible to the program executing in the VM.
 *
 *    Likewise, entries on the address stack are also a pair of addresses:
 *    one for the referenced item and one to the actual allocated block.
 *    The objects allocated by the function creation instructions (MKK, MKS0
 *    and so on) are very small (on the order of 20 - 40 bytes), so the
 *    allocator for created functions allocates N contiguous instances of
 *    the created function from the heap all at once and serves allocation
 *    requests from this block until it is exhaused, at which time the allocator
 *    allocates a new block of N contiguous instances and serves from that
 *    block.  To faciliate garbage collection, every "address" on the address
 *    stack is actually two addresses: the address of the created function
 *    and the address of the "block" of created functions from which it
 *    was allocated.  The garbage collector uses both addresses to mark
 *    a function instance as reachable and free the instances that are not
 *    reachable.
 *
 *    "Addresses" on the address stack that reference a saved program state
 *    instead of a created function will have one pointer pointing to the start
 *    of the memory allocated for the saved state on the heap and the other
 *    pointing to the actual start of the saved state itself.  The difference
 *    between the two is a four-byte "dead zone" filled with PANIC instructions
 *    in case the block is accidentally the argument for a PCALL instruction.
 *
 *    Again, that the "addresses" on the address stack are actually pairs
 *    of addresses is an implementation detail that is invisible to the
 *    program executing on the VM.
 *
 *    The garbage collector is not currently a compacting collector, which
 *    can lead to heap fragmentation and inefficient usage.  This collector
 *    is good enough for this proof-of-concept implementation of the UVM.
 *
 *    The maximum size of the call and address stacks are 2^32 entries.  The
 *    maximum heap size is 2^64 bytes.  host computer will probably run out
 *    actual memory before the UVM reaches any of these limits.
 *
 *    Data manipulation
 *    *  PUSH a    ;  Push address a onto the address stack
 *    *  POP       :  Pop a value from the address stack
 *    *  SWAP      :  Swap the top two addresses on the address stack
 *    *  DUP       :  Duplicate the address on top of the address stack
 *
 *    Control flow
 *    *  PCALL       :  Push the address of the next instruction onto the
 *                        call stack, pop the address on top of the call
 *                        stack then jump to that address.
 *    *  RET         :  Return from a function call by popping the address
 *                        on top of the call stack and jumping to that address.
 *
 *    Function creation
 *    * MKK          :  Pop the address at the top of the address stack.
 *                        Call it u.  Make a function on the heap that
 *                        evaluates its argument, discards the result
 *                        and returns u.  Push the address of that function
 *                        onto the address stack.
 *    * MKS0         :  Pop the address on the top of the address stack.
 *                        Call it u.  Make a function that implements ``suy
 *                        when invoked on a function that evaluates to y.
 *                        Push the address of the new function onto the
 *                        address stack.
 *    * MKS1         :  Pop the top two addresses on the stack and call them
 *                        u (top) and v (second).  Make a function on the
 *                        heap that implements ```suvz when invoked on a
 *                        function  that when called evaluates to z.  Push the
 *                        address of the newly-created function onto the
 *                        address stack.
 *    * MKS2         :  Pop the top two addresses on the stack and call them
 *                        u (top) and v (second).  Make a function on the
 *                        heap that computes u(v) and push the address for
 *                        that function onto the address stack.
 *    * MKD          :  Pop the address on top of the address stack and
 *                        call it x.  Make a function on the heap that
 *                        (1) calls x, (2) calls its argument, and then
 *                        calls the result of (1) with the result of (2).
 *                        This function implements the "d" special form.
 *                        Push the address of that function onto the stack.
 *    * MKC          :  The top of the address stack should point to a
 *                        saved interpreter state (created with the SAVE
 *                        instruction).  Create a function on the heap
 *                        that when invoked, (1) calls its argument to evaluate
 *                        it, (2) restores the interpreter state, (3)
 *                        pushes the result of (1) onto the stack, and (4)
 *                        returns.  Push the address of that function onto
 *                        the stack.  
 *
 *    Interpreter state
 *    * SAVE n       :  Save the interpreter, except for the top n addresses
 *                        on the address stack, to the heap.  Push a value
 *                        onto the address stack that can be used by RESTORE
 *                        to restore the interpreter state.
 *    * RESTORE n    :  The top of the address stack should point to a
 *                        saved interpreter state.  Call this address s.
 *                        Pop s, then pop n addresses and hold onto them.
 *                        Restore the interpreter state to s, then push the
 *                        popped n addresses onto the address stack.
 *
 *    Miscellaneous
 *    * PRINT c      :  Print the 3-byte unicode character c
 *    * HALT         :  Halt the VM and exit
 *    * PANIC        :  Crash the program.  Every block of saved state
 *                        begins with a PANIC instruction in case it
 *                        is accidentally called.
 * 
 * Implementation of k[x]:
 *   PCALL   ; Replace x with u = x()
 *   MKK     ; Replace u with lambda y: u
 *   RET     ; Return
 *
 * Code created by MKK[u]:
 *   PCALL   ; Evaluate argument
 *   POP     ; Discard argument value
 *   PUSH u  ; Function value
 *   RET     ; Return
 *
 * Implementation of s[x]:
 *   PCALL   ; Replace x with u = x()
 *   MKS0    ; Make lambda y.(
 *           ;    (lambda v.lambda z.(lambda w.(u(w)(v(w))))(z()))(y())
 *           ; )
 *           ; Consumes u
 *   RET     ; Return
 *
 * Code created by MKS0[u]
 *   PCALL   ; Compute v = y()
 *   PUSH u  ; u = X()
 *   MKS1    ; Make lambda z.((lambda w.(u(w)(v(w))))(z()))
 *           ; Consumes both u and v
 *   RET     ; Return
 *
 * Code created by MKS1[u, v]:
 *   PCALL   ; Compute w = z()
 *   DUP     ; Duplicate stack top
 *   PUSH v  ;
 *   MKS2    ; Create a no-argument function q() that computes v(w).
 *           ; Pops both v and first w, leaving second w
 *   SWAP    ; Swap q and w
 *   PUSH u  ;
 *   PCALL   ; Compute a = u(w).  Pops u and second w and pushes a
 *   PCALL   ; Compute a(q).  This will pop and call q() to evaluate v(w)
 *           ;   then apply a to the result.
 *   RET     ; Done
 *
 * Code created by MKS2[v, w]
 *   PUSH w  ; Push the argument to v
 *   PUSH v  ; Push v itself
 *   PCALL   ; Compute v(w) and put on top of stack, consuming v & w
 *   RET
 *
 * Implementation of i[x]:
 *   PCALL   ; Replace x with x()
 *   RET     ; Return x()
 *
 * Implementation of v[x]:
 * V_IMPL:
 *   PCALL   ; Replace x with x()
 *   POP     ; Discard argument
 *   PUSH V_IMPL ; Return value is pointer to v's implementation
 *   RET
 *
 * Implementation of d[x]:
 *   MKD     ; Create function that implements delayed evaluation
 *           ; Replaces x with pointer to created function
 *   RET     ; Done
 *
 * Code created by MKD[x]:
 *   PUSH x  ; Have to evaluate x before y
 *   PCALL   ; Replace x with u = x()
 *   SWAP    ; Bring argument y to stack top
 *   PCALL   ; Replace argument y with v = y()
 *   SWAP    ; Swap u and v
 *   PCALL   ; Replace u and v with u(v)
 *   RET
 *
 * Implementation of c[x]:
 *   SAVE 1  ; Save interpreter state, not including x
 *   MKC     ; Make continuation from saved state.  Pops saved state.
 *           ;    Call the continuation cc
 *   SWAP    ; Put x on top
 *   PCALL   ; Replace x with u = x()
 *   PCALL   ; Compute u(cc).  Pops u and cc and pushes result
 *   RET     ; Continaution not invoked so return value of u(cc)
 *
 * Code created by MKC[state]:
 *   PCALL   ; Replace y with u = y()
 *   PUSH state ;  Address of state to restore
 *   RESTORE 1 ; Pop the address of the saved interpreter state - call it s.
 *             ; Then pop u.  Restore the interpreter state, then push u
 *             ; onto the restored address stack.
 *   RET     ; Return
 *
 * Implementation of .<c>
 *   PCALL   ; Replace x with u = x()
 *   PRINT c ; Display single-byte character
 *   RET     ; Return u
 *
 * Implementation of .r is just implementation of .'\n'
 *
 * How unlcc would compile ``ki`kv (= i)
 *         PUSH EY1   ; Code to eva;iate `kv
 *         PUSH EX1   ; Code to evaluate `ki, then call EY1 to evaluate `kv
 *                    ;    then apply 'ki to 'kv
 *         PCALL
 *         HALT
 *     EX1:
 *         PUSH EY2   ; Code to evaluate i
 *         PUSH EX2   ; Code to evaluate k, then call EY2 to evaluate i
 *                    ; then apply k to i
 *         PCALL      ; Top now contains `ki
 *         PCALL      ; Apply `ki to `kv
 *         RET
 *     EX2:
 *         PUSH k_impl  
 *         PCALL        ; Apply k to i
 *         RET
 *     EY2:
 *         PUSH i_impl
 *         RET
 *     EY1:
 *         PUSH EY3
 *         PUSH EX3
 *         PCALL
 *         RET
 *     EX3:
 *         PUSH k_impl
 *         PCALL
 *         RET
 *     EY3:
 *         PUSH i_impl
 *         RET
 *
 * Alternate compilation of ``ki`kv
 *     EX1:
 *         PUSH k_impl
 *         PCALL          ;  Apply k to i
 *         RET
 *     EY1:
 *         PUSH i_impl
 *         RET
 *     AP1:
 *         PUSH EY1
 *         PUSH EX1
 *         PCALL
 *         RET
 *     EX2:
 *         PUSH k_impl
 *         PCALL
 *         RET
 *     EY2:
 *         PUSH v_impl
 *         RET
 *     AP2:
 *         PUSH EY2
 *         PUSH EX2
 *         PCALL
 *         RET
 *     EX3:
 *         PUSH AP1
 *         PCALL       ; Evaluate `ki
 *         PCALL       ; Apply `ki to `kv
 *         RET
 *     EY3:
 *         PUSH AP2
 *         PCALL
 *         RET
 *     AP3:
 *         PUSH EY3
 *         PUSH EX3
 *         PCALL
 *         HALT
 **/

/** TODO: Add breakpoints */
/** TODO: Rename PANIC to BRK and have the VM behave as if it had encountered
 *        a breakpoint.
 */

#include "vm.h"
#include "vm_instructions.h"
#include "vm_image.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct UnlambdaVmImpl_ {
  /** Name of the currently-loaded program.  Empty string if no program */
  const char* programName;

  /** The call stack */
  Stack callStack;

  /** The address stack */
  Stack addressStack;

  /** The VM's memory
   *
   *  The program and the heap are stored here
   */
  VmMemory memory;

  /** The program's symbol table.
   *
   *  Empty if no program has been loaded.  Used by the debugger but not
   *  by the VM itself.
   */
  SymbolTable symtab;

  /** The VM's current state
   *
   *  See the VmState* constants for values of this field
   */
  int state;

  /** The VM's program counter
   *
   *  Points to the next instruction to execute.  Zero if no program has
   *  been loaded.
   */
  uint64_t pc;

  /** Outcome of last operation (0 = success) */
  int statusCode;

  /** Message describing outcome of last operation
   *
   *  OK_MSG if successful.
   */
  const char* statusMsg;

  /** Handler for GC errors */
  GcErrorHandler gcErrorHandler;
} UnlambdaVmImpl;

static const char NO_PROGRAM[] = "";
static const char OK_MSG[] = "OK";
static const char DEFAULT_ERR_MSG[] = "ERROR";

/** Values for vm->state */

/** The VM does not have a program */
const int VmStateNoProgram = 0;

/** The VM has a program and is ready to run */
const int VmStateReady = 1;

/** The VM has executed a HALT instruction */
const int VmStateHalted = 2;

/** The VM has executed a PANIC instruction */
const int VmStatePanic = 3;

/** Values for vm->statusCode */
const int VmProgramAlreadyLoadedError = -1;
const int VmIOError = -2;
const int VmBadProgramImageError = -3;
const int VmOutOfMemoryError = -4;
const int VmHalted = -5;
const int VmPanicError = -6;
const int VmIllegalInstructionError = -7;
const int VmIllegalAddressError = -8;
const int VmCallStackUnderflowError = -9;
const int VmCallStackOverflowError = -10;
const int VmAddressStackUnderflowError = -11;
const int VmAddressStackOverflowError = -12;
const int VmNoProgramLoadedError = -13;
const int VmFatalError = -14;
const int VmIllegalArgumentError = -15;

static int executeNextInstruction(UnlambdaVM vm);
static int executePushInstruction(UnlambdaVM vm);
static int executePopInstruction(UnlambdaVM vm);
static int executeSwapInstruction(UnlambdaVM vm);
static int executeDupInstruction(UnlambdaVM vm);
static int executePCallInstruction(UnlambdaVM vm);
static int executeReturnInstruction(UnlambdaVM vm);
static int executeMkkInstruction(UnlambdaVM vm);
static int executeMks0Instruction(UnlambdaVM vm);
static int executeMks1Instruction(UnlambdaVM vm);
static int executeMks2Instruction(UnlambdaVM vm);
static int executeMkdInstruction(UnlambdaVM vm);
static int executeMkcInstruction(UnlambdaVM vm);
static int executeSaveInstruction(UnlambdaVM vm);
static int executeRestoreInstruction(UnlambdaVM vm);
static int executePrintInstruction(UnlambdaVM vm);
static int pushAddressToVmStack(UnlambdaVM vm, Stack s, uint64_t addr,
				int stackOverflowErrorCode,
				const char* stackOverflowErrorMsg,
				const char* outOfMemoryErrorMsg);
static int popAddressFromVmStack(UnlambdaVM vm, Stack s, uint64_t* addr,
				 int stackUnderflowCode,
				 const char* stackUnderflowMsg);
static int pushToAddressStack(UnlambdaVM vm, uint64_t addr);
static int popFromAddressStack(UnlambdaVM vm, uint64_t* addr);
static int pushToCallStack(UnlambdaVM vm, uint64_t addr);
static int popFromCallStack(UnlambdaVM vm, uint64_t* addr);
static int readFromAddressStackTop(UnlambdaVM vm, uint64_t depth,
				   uint64_t* value);
static CodeBlock* allocateCodeBlock(UnlambdaVM vm, const char* instruction,
				    uint64_t size);
static VmStateBlock* allocateVmStateBlock(UnlambdaVM vm,
					  const char* instruction,
					  uint32_t callStackSize,
					  uint32_t addressStackSize);
static void reportBlockAllocationFailure(UnlambdaVM vm,
					 const char* instruction,
					 uint64_t size,
					 const char* details);
static void handleGcError(VmMemory memory, uint64_t address, HeapBlock* block,
			  const char* details, void* unused);

/** TODO: Add clearVmStatus() to vm operations */

/** TODO: Fix the garbage collector to skip addresses inside the program
 *        area of the VmMemory.
 */
/** TODO: PCALL takes the address of the code to execute, but the garbage
 *        collection code treats the operand of a PUSH instruction as
 *        as a pointer to the header of an allocated block rather than its
 *        data.  Fix this bug so the garbage collector treats the PUSH
 *        operand as a pointer to the first data byte in an alocated block.
 *        and looks for the header 8 bytes before.
 */
/** TODO: Addresses returned by the MK* instructions need to point to
 *        the data section of the allocated block, not the header
 */
/** TODO: Ensure operands exist for the SAVE, RESTORE and PRINT instructions */
/** TODO: Pass maximum symbol table size as an argument */
UnlambdaVM createUnlambdaVM(uint32_t maxCallStackSize,
			    uint32_t maxAddressStackSize,
			    uint64_t initialMemorySize,
			    uint64_t maxMemorySize) {
  static const int initialCallStackSize = 1024;
  static const int initialAddressStackSize = 1024;
  static const uint32_t maxSymbolTableSize = 256 * 1024 * 1024;
  UnlambdaVM vm = (UnlambdaVM)malloc(sizeof(UnlambdaVmImpl));
  if (!vm) {
    return NULL;
  }

  vm->callStack = createStack(
    16 * ((initialCallStackSize <= maxCallStackSize) ? initialCallStackSize
	                                             : maxCallStackSize),
    16 * maxCallStackSize
  );
  if (!vm->callStack) {
    return NULL;
  }

  vm->addressStack = createStack(
    8 * ((initialAddressStackSize <= maxAddressStackSize)
            ? initialAddressStackSize
	    : maxAddressStackSize),
    8 * maxAddressStackSize
  );
  if (!vm->addressStack) {
    destroyStack(vm->callStack);
    return NULL;
  }

  vm->memory = createVmMemory(initialMemorySize, maxMemorySize);
  if (!vm->memory) {
    destroyStack(vm->addressStack);
    destroyStack(vm->callStack);
    return NULL;
  }

  vm->symtab = createSymbolTable(maxSymbolTableSize);
  if (!vm->symtab) {
    destroyVmMemory(vm->memory);
    destroyStack(vm->addressStack);
    destroyStack(vm->callStack);
    return NULL;
  }

  vm->programName = NO_PROGRAM;
  vm->state = VmStateNoProgram;
  vm->pc = 0;
  vm->statusCode = 0;
  vm->statusMsg = OK_MSG;
  vm->gcErrorHandler = handleGcError;

  return vm;
}

void destroyUnlambdaVM(UnlambdaVM vm) {
  if (vm) {
    clearVmStatus(vm);
    destroySymbolTable(vm->symtab);
    destroyVmMemory(vm->memory);
    destroyStack(vm->addressStack);
    destroyStack(vm->callStack);

    if (vm->programName && (vm->programName != NO_PROGRAM)) {
      free((void*)vm->programName);
    }
  }
  free((void*)vm);
}

int getVmStatus(UnlambdaVM vm) {
  return vm->statusCode;
}

const char* getVmStatusMsg(UnlambdaVM vm) {
  return vm->statusMsg;
}

void clearVmStatus(UnlambdaVM vm) {
  if (vm->statusMsg && (vm->statusMsg != OK_MSG)
          && (vm->statusMsg != DEFAULT_ERR_MSG)) {
    free((void*)vm->statusMsg);
  }
  vm->statusCode = 0;
  vm->statusMsg = OK_MSG;
}

static void setVmStatus(UnlambdaVM vm, int statusCode, const char* statusMsg) {
  if (vm->statusMsg && (vm->statusMsg != OK_MSG)
          && (vm->statusMsg != DEFAULT_ERR_MSG)) {
    free((void*)vm->statusMsg);
  }

  vm->statusCode = statusCode;
  if ((statusMsg == OK_MSG) || (statusMsg == DEFAULT_ERR_MSG)) {
    vm->statusMsg = statusMsg;
  } else {
    vm->statusMsg = (const char*)malloc(strlen(statusMsg) + 1);
    if (vm->statusMsg) {
      strcpy((char*)vm->statusMsg, statusMsg);
    } else {
      vm->statusMsg = DEFAULT_ERR_MSG;
    }
  }
}

const char* getVmProgramName(UnlambdaVM vm) {
  return vm->programName;
}

uint64_t getVmPC(UnlambdaVM vm) {
  return vm->pc;
}

int setVmPC(UnlambdaVM vm, uint64_t address) {
  if (!isValidVmmAddress(getVmMemory(vm), address)) {
    setVmStatus(vm, VmIllegalArgumentError, "Invalid address");
    return -1;
  }
  vm->pc = address;
  return 0;
}

Stack getVmCallStack(UnlambdaVM vm) {
  return vm->callStack;
}

Stack getVmAddressStack(UnlambdaVM vm) {
  return vm->addressStack;
}

SymbolTable getVmSymbolTable(UnlambdaVM vm) {
  return vm->symtab;
}

VmMemory getVmMemory(UnlambdaVM vm) {
  return vm->memory;
}

uint8_t* ptrToVmPC(UnlambdaVM vm) {
  return ptrToVmmAddress(vm->memory, vm->pc);
}

uint8_t* ptrToVmAddress(UnlambdaVM vm, uint64_t address) {
  return ptrToVmmAddress(vm->memory, address);
}

int loadProgramIntoVm(UnlambdaVM vm, const char* filename, int loadSymbols) {
  if (vm->state != VmStateNoProgram) {
    setVmStatus(vm, VmProgramAlreadyLoadedError,
		"Program already loaded into VM");
    return -1;
  } else {
    const char* errMsg = NULL;
    int result = loadVmProgramImage(filename, vm, loadSymbols, &errMsg);

    if (!result) {
      vm->state = VmStateReady;
      assert(!errMsg);
      return 0;
    } else if (result == VmImageIllegalArgumentError) {
      char msg[200];
      snprintf(msg, sizeof(msg), "Error calling loadVmProgramImage(): %s",
	       errMsg);
      setVmStatus(vm, VmFatalError, msg);
    } else if (result == VmImageProgramAlreadyLoadedError) {
      setVmStatus(vm, VmProgramAlreadyLoadedError, errMsg);
    } else if (result == VmImageIOError) {
      char msg[200];
      snprintf(msg, sizeof(msg), "Error loading %s (%s)", filename, errMsg);
      setVmStatus(vm, VmIOError, msg);
    } else if (result == VmImageFormatError) {
      char msg[200];
      snprintf(msg, sizeof(msg), "Error loading %s (%s)", filename, errMsg);
      setVmStatus(vm, VmBadProgramImageError, msg);
    } else {
      char msg[200];
      snprintf(msg, sizeof(msg),
	       "Error loading %s (loadVmProgramImage() returned unknown "
	       "status code %d and error message \"%s\"", filename, result,
	       errMsg);
      setVmStatus(vm, VmBadProgramImageError, msg);
    }
  
    if (errMsg) {
      free((void*)errMsg);
    }
    return -1;
  }
}

int loadVmProgramFromMemory(UnlambdaVM vm, const char* name,
			    const uint8_t* program, uint64_t programSize) {
  VmMemory memory = getVmMemory(vm);

  if (vm->state != VmStateNoProgram) {
    setVmStatus(vm, VmProgramAlreadyLoadedError,
		"Program already loaded into VM");
    return -1;
  }
  
  if (!getVmmProgramMemorySize(memory)) {
    /** Set the program area to be equal to the program size */
    if (reserveVmMemoryForProgram(memory, programSize)) {
      if (getVmmStatus(memory) == VmmNotEnoughMemoryError) {
	setVmStatus(vm, VmOutOfMemoryError, getVmmStatusMsg(memory));
      } else {
	char msg[200];
	snprintf(msg, sizeof(msg),
		 "reserveMemoryForProgram() returned unknown or unexpected "
		 "error code %d (%s)", getVmmStatus(memory),
		 getVmmStatusMsg(memory));
	setVmStatus(vm, VmFatalError, msg);
      }
      return -1;
    }
  } else if (programSize > getVmmProgramMemorySize(memory)) {
    /** Configured program area isn't big enough */
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Cannot store a program of %" PRIu64 " bytes in a program area "
	     "of %" PRIu64 " bytes", programSize,
	     getVmmProgramMemorySize(memory));
    setVmStatus(vm, VmIllegalArgumentError, msg);
    return -1;
  }

  assert(getVmmProgramMemorySize(memory) >= programSize);

  /** Copy the program and fill any remaining bytes with HALT instructions */
  memcpy(getProgramStartInVmm(memory), program, programSize);
  memset(getProgramStartInVmm(memory) + programSize, HALT_INSTRUCTION,
	 getVmmProgramMemorySize(memory) - programSize);

  vm->programName = strdup(name);
  vm->state = VmStateReady;
  return 0;
}

int stepVm(UnlambdaVM vm) {
  if (vm->state == VmStateNoProgram) {
    setVmStatus(vm, VmNoProgramLoadedError, "No program");
    return -1;
  } else if (vm->state == VmStateReady) {
    return executeNextInstruction(vm);
  } else if (vm->state == VmStateHalted) {
    setVmStatus(vm, VmHalted, "VM halted");
    return -1;
  } else if (vm->state == VmStatePanic) {
    setVmStatus(vm, VmPanicError, "VM executed a PANIC instruction");
    return -1;
  } else {
    char msg[200];
    snprintf(msg, sizeof(msg), "VM is in unknown state %d", vm->state);
    setVmStatus(vm, VmFatalError, msg);
    return -1;
  }

  /** Never get here */
}

static int executeNextInstruction(UnlambdaVM vm) {
  uint8_t* pcp = ptrToVmPC(vm);

  if (!pcp) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "VM PC is located at illegal address 0x%" PRIx64, vm->pc);
    setVmStatus(vm, VmFatalError, msg);
    return -1;
  }

  /** TODO: Turn this into a dispatch table */
  switch(*pcp) {
    case PUSH_INSTRUCTION:
      return executePushInstruction(vm);

    case POP_INSTRUCTION:
      return executePopInstruction(vm);

    case SWAP_INSTRUCTION:
      return executeSwapInstruction(vm);

    case DUP_INSTRUCTION:
      return executeDupInstruction(vm);

    case PCALL_INSTRUCTION:
      return executePCallInstruction(vm);

    case RET_INSTRUCTION:
      return executeReturnInstruction(vm);

    case MKK_INSTRUCTION:
      return executeMkkInstruction(vm);

    case MKS0_INSTRUCTION:
      return executeMks0Instruction(vm);

    case MKS1_INSTRUCTION:
      return executeMks1Instruction(vm);

    case MKS2_INSTRUCTION:
      return executeMks2Instruction(vm);

    case MKD_INSTRUCTION:
      return executeMkdInstruction(vm);

    case MKC_INSTRUCTION:
      return executeMkcInstruction(vm);

    case SAVE_INSTRUCTION:
      return executeSaveInstruction(vm);

    case RESTORE_INSTRUCTION:
      return executeRestoreInstruction(vm);

    case PRINT_INSTRUCTION:
      return executePrintInstruction(vm);

    case HALT_INSTRUCTION:
      vm->state = VmStateHalted;
      setVmStatus(vm, VmHalted, "VM halted");
      return -1;

    case PANIC_INSTRUCTION:
      vm->state = VmStatePanic;
      setVmStatus(vm, VmPanicError, "VM executed a PANIC instruction");
      return -1;

    default:
      setVmStatus(vm, VmIllegalInstructionError,
		  "VM attempted to execute an illegal instruction");
      return -1;
  }
}

static int executePushInstruction(UnlambdaVM vm) {
  uint8_t* p = ptrToVmPC(vm);
  
  if ((p + 9) > ptrToVmMemoryEnd(vm->memory)) {
    char msg[100];
    snprintf(msg, sizeof(msg), "Cannot read 8 bytes from address %lu",
	     vm->pc + 1);
    setVmStatus(vm, VmIllegalAddressError, msg);
    return -1;
  }

  // TODO: Fix this for architectures that can't handle unaligned reads
  ++p;
  if (pushToAddressStack(vm, *(uint64_t*)p)) {
    return -1;
  }

  vm->pc += 9;  
  return 0;
}

static int executePopInstruction(UnlambdaVM vm) {
  if (popFromAddressStack(vm, NULL)) {
    return -1;
  }
  ++(vm->pc);
  return 0;
}

static int executeSwapInstruction(UnlambdaVM vm) {
  if (swapStackTop(vm->addressStack, 8)) {
    assert(getStackStatus(vm->addressStack) == StackUnderflowError);
    char msg[100];
    snprintf(msg, sizeof(msg), "Cannot SWAP a stack with only %lu entries",
	     stackSize(vm->addressStack) / 8);
    setVmStatus(vm, VmAddressStackUnderflowError, msg);
    return -1;
  }
  ++(vm->pc);
  return 0;
}

static int executeDupInstruction(UnlambdaVM vm) {
  if (dupStackTop(vm->addressStack, 8)) {
    const int status = getStackStatus(vm->addressStack);
    if (status == StackUnderflowError) {
      setVmStatus(vm, VmAddressStackUnderflowError,
		  "Cannot DUP the top of an empty stack");
    } else if (status == StackOverflowError) {
      setVmStatus(vm, VmAddressStackOverflowError, "Address stack overflow");
    } else {
      if (status != StackMemoryAllocationFailedError) {
	printf("status = %d\n", status);
      }
      assert(status == StackMemoryAllocationFailedError);
      setVmStatus(vm, VmFatalError,
		  "Cannot allocate more memory for the address stack");
    }
    return -1;
  }

  ++(vm->pc);
  return 0;
}

static int executePCallInstruction(UnlambdaVM vm) {
  uint64_t target = 0;

  if (popFromAddressStack(vm, &target)) {
    return -1;
  }

  if (!isValidVmmAddress(vm->memory, target)) {
    // Call to invalid address
    char details[200];
    snprintf(details, sizeof(details), "PCALL to invalid address 0x%" PRIx64,
	     target);
    setVmStatus(vm, VmIllegalAddressError, details);

    // Put the address back onto the address stack
    assert(!pushToAddressStack(vm, target));
    return -1;
  }

  if (pushToCallStack(vm, target)
        || pushToCallStack(vm, vm->pc + 1)) {
    // Push the address back onto the address stack.  Since we just popped
    // it, there should be space for it
    assert(!pushToAddressStack(vm, target));
    return -1;
  }

  vm->pc = target;
  return 0;
}

static int executeReturnInstruction(UnlambdaVM vm) {
  uint64_t target = 0;

  if (popFromCallStack(vm, &target)
          || popFromCallStack(vm, NULL)) {
    return -1;
  }

  vm->pc = target;
  return 0;
}

/** For all of the MK* instructions, or any instruction that pops arguments
 *  from the stack, allocates a code or state block, and then references
 *  those arguments in the generated code, do not pop the values from the
 *  stack until after the allocation is complete, or the garbage collector
 *  may collect the blocks referenced by the popped arguments before the
 *  new code or state block is constructed, leaving those blocks pointing
 *  to free blocks.  Instead, use readFromAddressStackTop() to read the
 *  arguments from the address stack without popping them, then construct
 *  the code or state block, then pop the values if successful.
 */
static int executeMkkInstruction(UnlambdaVM vm) {
  uint64_t arg = 0;

  if (readFromAddressStackTop(vm, 0, &arg)) {
    /** Error code set by readFromAddressStackTop() */
    return -1;    
  }

  CodeBlock* f = allocateCodeBlock(vm, "MKK", 12);
  if (!f) {
    return -1;
  }

  f->code[0] = PCALL_INSTRUCTION;
  f->code[1] = POP_INSTRUCTION;
  f->code[2] = PUSH_INSTRUCTION;
  *(uint64_t*)(f->code + 3) = arg;
  f->code[11] = RET_INSTRUCTION;

  /** Just read 8 bytes, so this should always succeed */
  /** TODO: Create a replaceStackTop() operation to replace POP + PUSH */
  assert(!popFromAddressStack(vm, NULL));
  assert(!pushToAddressStack(vm, vmmAddressForPtr(vm->memory, f->code)));

  ++(vm->pc);
  return 0;
}

static int executeMks0Instruction(UnlambdaVM vm) {
  uint64_t arg = 0;

  if (readFromAddressStackTop(vm, 0, &arg)) {
    return -1;    
  }
  
  CodeBlock* f = allocateCodeBlock(vm, "MKS0", 12);
  if (!f) {
    return -1;
  }
  f->code[0] = PCALL_INSTRUCTION;
  f->code[1] = PUSH_INSTRUCTION;
  *(uint64_t*)(f->code + 2) = arg;
  f->code[10] = MKS1_INSTRUCTION;
  f->code[11] = RET_INSTRUCTION;

  /** Just read 8 bytes, so these should always succeed */
  assert(!popFromAddressStack(vm, NULL));
  assert(!pushToAddressStack(vm, vmmAddressForPtr(vm->memory, f->code)));

  ++(vm->pc);
  return 0;
}

static int executeMks1Instruction(UnlambdaVM vm) {
  uint64_t u = 0, v = 0;

  if (readFromAddressStackTop(vm, 0, &u)
        || readFromAddressStackTop(vm, 1, &v)) {
    return -1;
  }

  CodeBlock* f = allocateCodeBlock(vm, "MKS1", 25);
  if (!f) {
    return -1;
  }
  f->code[0] = PCALL_INSTRUCTION; /** Evaluate w = z() */
  f->code[1] = DUP_INSTRUCTION;   /** Duplicate w */
  f->code[2] = PUSH_INSTRUCTION;  /** Push v */
  *(uint64_t*)(f->code + 3) = v;
  f->code[11] = MKS2_INSTRUCTION; /** Create q = lambda.v(w) */
  f->code[12] = SWAP_INSTRUCTION; /** Swap q and w */
  f->code[13] = PUSH_INSTRUCTION; /** Push u */
  *(uint64_t*)(f->code + 14) = u;
  f->code[22] = PCALL_INSTRUCTION; /** Compute a = u(w) */
  f->code[23] = PCALL_INSTRUCTION; /** Compute a(q) */
  f->code[24] = RET_INSTRUCTION;

  /** Just read 16 bytes, so popping16 and pushing 8 should succeed */
  const uint64_t codeAddr = vmmAddressForPtr(vm->memory, f->code);
  assert(!popFromAddressStack(vm, NULL));
  assert(!popFromAddressStack(vm, NULL));
  assert(!pushToAddressStack(vm, codeAddr));

  ++(vm->pc);
  return 0;
}

static int executeMks2Instruction(UnlambdaVM vm) {
  uint64_t u = 0, v = 0;

  if (readFromAddressStackTop(vm, 0, &u)
        || readFromAddressStackTop(vm, 1, &v)) {
    return -1;
  }

  CodeBlock* f = allocateCodeBlock(vm, "MKS2", 20);
  if (!f) {
    return -1;
  }
  f->code[0] = PUSH_INSTRUCTION;
  *(uint64_t*)(f->code + 1) = v;
  f->code[9] = PUSH_INSTRUCTION;
  *(uint64_t*)(f->code + 10) = u;
  f->code[18] = PCALL_INSTRUCTION;
  f->code[19] = RET_INSTRUCTION;

  assert(!popFromAddressStack(vm, NULL));
  assert(!popFromAddressStack(vm, NULL));
  assert(!pushToAddressStack(vm, vmmAddressForPtr(vm->memory, f->code)));
  
  ++(vm->pc);
  return 0;
}

static int executeMkdInstruction(UnlambdaVM vm) {
  uint64_t arg = 0;

  if (readFromAddressStackTop(vm, 0, &arg)) {
    return -1;
  }

  CodeBlock* f = allocateCodeBlock(vm, "MKD", 15);
  if (!f) {
    return -1;
  }
  f->code[0] = PUSH_INSTRUCTION;
  *(uint64_t*)(f->code + 1) = arg;
  f->code[9] = PCALL_INSTRUCTION;
  f->code[10] = SWAP_INSTRUCTION;
  f->code[11] = PCALL_INSTRUCTION;
  f->code[12] = SWAP_INSTRUCTION;
  f->code[13] = PCALL_INSTRUCTION;
  f->code[14] = RET_INSTRUCTION;

  // Just read 8 bytes, so this should succeed
  assert(!popFromAddressStack(vm, NULL));
  assert(!pushToAddressStack(vm, vmmAddressForPtr(vm->memory, f->code)));
  
  ++(vm->pc);
  return 0;
}

static int executeMkcInstruction(UnlambdaVM vm) {
  uint64_t savedState = 0;

  if (readFromAddressStackTop(vm, 0, &savedState)) {
    return -1;
  }

  CodeBlock* f = allocateCodeBlock(vm, "MKC", 13);
  if (!f) {
    return -1;
  }

  f->code[0] = PCALL_INSTRUCTION;
  f->code[1] = PUSH_INSTRUCTION;
  *(uint64_t*)(f->code + 2) = savedState;
  f->code[10] = RESTORE_INSTRUCTION;
  f->code[11] = 1;
  f->code[12] = RET_INSTRUCTION;

  // Just read 8 bytes, so this should succeed
  assert(!popFromAddressStack(vm, NULL));
  assert(!pushToAddressStack(vm, vmmAddressForPtr(vm->memory, f->code)));
  
  ++(vm->pc);
  return 0;
}

static int executeSaveInstruction(UnlambdaVM vm) {
  uint8_t* const ppc = ptrToVmPC(vm);
  const uint8_t skip = ppc[1];  /** # of addresses to skip on address stack */

  if (stackSize(vm->addressStack) < (8 * (uint64_t)skip)) {
    setVmStatus(vm, VmAddressStackUnderflowError, "Address stack underflow");
    return -1;
  }

  /** Make sure there is space on the address stack for the pointer to
   *  the saved state block.
   */
  if (stackSize(vm->addressStack) == stackMaxSize(vm->addressStack)) {
    setVmStatus(vm, VmAddressStackOverflowError, "Address stack overflow");
    return -1;
  }
  
  VmStateBlock* const state = allocateVmStateBlock(
      vm, "SAVE", stackSize(vm->callStack) / 16,
      (stackSize(vm->addressStack) / 8) - skip
  );
  if (!state) {
    return -1;
  }

  uint8_t* const addressStackStart =
      state->stacks + stackSize(vm->callStack);

  memcpy((void*)state->stacks, (const void*)bottomOfStack(vm->callStack),
	 stackSize(vm->callStack));
  memcpy((void*)addressStackStart, (const void*)bottomOfStack(vm->addressStack),
	 stackSize(vm->addressStack) - (8 * skip));

  // Address of state block's data goes on stack.  
  const uint64_t stateAddr = vmmAddressForPtr(vm->memory, (uint8_t*)state)
                               + sizeof(HeapBlock);
  /** Checked for enough space on the address stack earlier, so this call
   *    should succeed.
   */
  assert(!pushToAddressStack(vm, stateAddr));

  vm->pc += 2;
  return 0;
}

static int executeRestoreInstruction(UnlambdaVM vm) {
  uint8_t* const ppc = ptrToVmPC(vm);
  /** Number of entries on the address stack to save and push on top of
   *  the restored address stack
   */
  const uint8_t save = ppc[1];
  const uint64_t bytesToSave = 8 * (uint64_t)save;

  /** Pop the address of the saved state block from the stack */
  uint64_t savedStateAddr = 0;

  if (popFromAddressStack(vm, &savedStateAddr)) {
    return -1;
  }

  /** Get a pointer to the VmStateBlock on the heap */
  VmStateBlock* vmState = (VmStateBlock*)(
      ptrToVmmAddress(vm->memory, savedStateAddr - sizeof(HeapBlock))
  );

  if (!vmState) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Cannot read from address 0x%" PRIx64,
	     savedStateAddr - sizeof(HeapBlock));
    /** Put the popped address back */
    assert(!pushToAddressStack(vm, savedStateAddr));
    setVmStatus(vm, VmIllegalAddressError, msg);
    return -1;
  }

  if (getVmmBlockType((HeapBlock*)vmState) != VmmStateBlockType) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Block at address 0x%" PRIx64 " is not a VmStateBlock.  It "
	     "has type %u", savedStateAddr,
	     (unsigned int)getVmmBlockType((HeapBlock*)vmState));
    /** Put back the popped address */
    assert(!pushToAddressStack(vm, savedStateAddr));
    setVmStatus(vm, VmFatalError, msg);
    return -1;
  }

  /** Save "save" addresses on the top of the address stack to push later */
  if (stackSize(vm->addressStack) < bytesToSave) {
    assert(!pushToAddressStack(vm, savedStateAddr));
    setVmStatus(vm, VmAddressStackUnderflowError, "Address stack underflow");
    return -1;
  }
  
  uint8_t* savedData = (uint8_t*)malloc(bytesToSave);

  if (save && !savedData) {
    setVmStatus(vm, VmFatalError,
		"Could not allocate buffer for stack top for the RESTORE "
		"instruction");
    return -1;
  }

  memcpy((void*)savedData,
	 (const void*)(topOfStack(vm->addressStack) - bytesToSave),
	 bytesToSave);

  /** Ensure the address stack can hold the restored stack plus any data
   *  from the current stack that goes on top */
  if ((8 * vmState->addressStackSize + bytesToSave)
        > stackMaxSize(vm->addressStack)) {
    assert(!pushToAddressStack(vm, savedStateAddr));
    setVmStatus(vm, VmAddressStackOverflowError, "Address stack overflow");
    free((void*)savedData);
    return -1;
  }
  
  /** Restore the call and address stacks */

  if (setStack(vm->callStack, vmState->stacks, 16 * vmState->callStackSize)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Could not restore call stack (%s)",
	     getStackStatusMsg(vm->callStack));
    setVmStatus(vm, VmFatalError, msg);
    free((void*)savedData);
    return -1;
  }

  uint8_t* const addressStackStart =
      vmState->stacks + 16 * vmState->callStackSize;
  if (setStack(vm->addressStack, addressStackStart,
		8 * vmState->addressStackSize)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Could not restore address stack (%s)",
	     getStackStatusMsg(vm->addressStack));
    setVmStatus(vm, VmFatalError, msg);
    free((void*)savedData);
    return -1;    
  }

  if (save) {
    /** Push the saved address stack top onto the address stack */
    if (pushStack(vm->addressStack, savedData, bytesToSave)) {
      const int status = getStackStatus(vm->addressStack);
      if (status == StackOverflowError) {
	setVmStatus(vm, VmAddressStackOverflowError, "Address stack overflow");
      } else {
	assert(status == StackMemoryAllocationFailedError);
	setVmStatus(vm, VmFatalError,
		    "Could not allocate more memory for the address stack");
      }
      free((void*)savedData);
      return -1;
    }
  }

  free((void*)savedData);

  vm->pc += 2;
  return 0;
}

static int executePrintInstruction(UnlambdaVM vm) {
  uint8_t* const ppc = ptrToVmPC(vm);
  printf("%c", (char)ppc[1]);
  vm->pc += 2;
  return 0;
}

static int pushAddressToVmStack(UnlambdaVM vm, Stack s, uint64_t addr,
				int stackOverflowErrorCode,
				const char* stackOverflowErrorMsg,
				const char* outOfMemoryErrorMsg) {
  if (pushStack(s, &addr, sizeof(addr))) {
    const int status = getStackStatus(s);

    if (status == StackOverflowError) {
      setVmStatus(vm, stackOverflowErrorCode, stackOverflowErrorMsg);
    } else {
      assert(status == StackMemoryAllocationFailedError);
      setVmStatus(vm, VmFatalError, outOfMemoryErrorMsg);
    }
    return -1;
  }
  return 0;
}

static int popAddressFromVmStack(UnlambdaVM vm, Stack s, uint64_t* addr,
				 int stackUnderflowErrorCode,
				 const char* stackUnderflowErrorMsg) {
  if (popStack(s, addr, sizeof(uint64_t))) {
    assert(getStackStatus(s) == StackUnderflowError);
    setVmStatus(vm, stackUnderflowErrorCode, stackUnderflowErrorMsg);
    return -1;
  }
  return 0;
}

static int pushToAddressStack(UnlambdaVM vm, uint64_t addr) {
  return pushAddressToVmStack(
      vm, vm->addressStack, addr, VmAddressStackOverflowError,
      "Address stack overflow",
      "Cannot allocate more memory for the address stack"
  );
}

static int popFromAddressStack(UnlambdaVM vm, uint64_t* addr) {
  return popAddressFromVmStack(vm, vm->addressStack, addr,
			       VmAddressStackUnderflowError,
			       "Address stack underflow");
}

static int pushToCallStack(UnlambdaVM vm, uint64_t addr) {
  return pushAddressToVmStack(vm, vm->callStack, addr, VmCallStackOverflowError,
			      "Call stack overflow",
			      "Cannot allocate more memory for the call stack");
}

static int popFromCallStack(UnlambdaVM vm, uint64_t* addr) {
  return popAddressFromVmStack(vm, vm->callStack, addr,
			       VmCallStackUnderflowError,
			       "Call stack underflow");
}

static int readFromAddressStackTop(UnlambdaVM vm, uint64_t depth,
				   uint64_t* value) {
  Stack addressStack = getVmAddressStack(vm);
  const uint64_t offset = (depth + 1) * 8;
  if ((offset < (depth + 1)) || (offset > stackSize(addressStack))) {
    setVmStatus(vm, VmAddressStackUnderflowError, "Address stack underflow");
    return -1;
  }

  *value = *(uint64_t*)(topOfStack(addressStack) - offset);
  return 0;
}

static CodeBlock* allocateCodeBlock(UnlambdaVM vm, const char* instruction,
				    uint64_t size) {
 CodeBlock* f = allocateVmmCodeBlock(vm->memory, size);
  if (!f) {
    if (collectUnreachableVmmBlocks(vm->memory, vm->callStack, vm->addressStack,
				    vm->gcErrorHandler, NULL)) {
      /** If collection fails, the heap is corrupt, so indicate we could
       *  not allocate the code block on the heap
       */
      reportBlockAllocationFailure(vm, instruction, size, "GC failed");
      return NULL;
    }

    f = allocateVmmCodeBlock(vm->memory, size);
    while ((!f) && (currentVmmSize(vm->memory) < maxVmmSize(vm->memory))) {
      if (increaseVmmSize(vm->memory)) {
	reportBlockAllocationFailure(vm, instruction, size,
				     getVmmStatusMsg(vm->memory));
	return NULL;
      }
      f = allocateVmmCodeBlock(vm->memory, size);
    }

    if (!f) {
      reportBlockAllocationFailure(vm, instruction, size,
				   "Maximum memory size exceeded");
      return NULL;
    }
  }
  return f;
}

static VmStateBlock* allocateVmStateBlock(UnlambdaVM vm,
					  const char* instruction,
					  uint32_t callStackSize,
					  uint32_t addressStackSize) {
  VmStateBlock* b = allocateVmmStateBlock(vm->memory, callStackSize,
					  addressStackSize);
  const uint64_t size = 16 * callStackSize + 8 * addressStackSize + 16;
  
  if (!b) {
    if (collectUnreachableVmmBlocks(vm->memory, vm->callStack, vm->addressStack,
				    vm->gcErrorHandler, NULL)) {
      /** If collection fails, the heap is corrupt, so indicate we could
       *  not allocate the code block on the heap
       */
      reportBlockAllocationFailure(vm, instruction, size, "GC failed");
      return NULL;
    }

    b = allocateVmmStateBlock(vm->memory, callStackSize, addressStackSize);
    while ((!b) && (currentVmmSize(vm->memory) < maxVmmSize(vm->memory))) {
      if (increaseVmmSize(vm->memory)) {
	reportBlockAllocationFailure(vm, instruction, size,
				     getVmmStatusMsg(vm->memory));
	return NULL;
      }
      b = allocateVmmStateBlock(vm->memory, callStackSize, addressStackSize);
    }

    if (!b) {
      reportBlockAllocationFailure(vm, instruction, size,
				   "Maximum memory size exceeded");
      return NULL;
    }
  }
  return b;
}

static void reportBlockAllocationFailure(UnlambdaVM vm,
					 const char* instruction,
					 uint64_t size,
					 const char* details) {
  char msg[200];
  snprintf(msg, sizeof(msg),
	   "Could not allocate block of size %" PRIu64 " for %s (%s)",
	   size, instruction, details);
  setVmStatus(vm, VmOutOfMemoryError, msg);
}

static void handleGcError(VmMemory memory, uint64_t address, HeapBlock* block,
			  const char* details, void* unused) {
  char msg[1000];
  snprintf(msg, sizeof(msg),
	   "**GC ERROR at address 0x%" PRIX64 " (block size=0x%" PRIu64
	   ", type = %u, mark = %d): %s\n", address, getVmmBlockSize(block),
	   getVmmBlockType(block), vmmBlockIsMarked(block), details);
  printf("%s", msg);
}

