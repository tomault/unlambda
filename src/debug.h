#ifndef __UNLAMBDA__DEBUG_H__
#define __UNLAMBDA__DEBUG_H__

/** Implementation of the virtual machine debugger that gets started when
 *  the VM executes a BRK instruction or has a fault of some kind.
 *
 *  Debugger commands:
 *    l [addr] [lines]  Dissassemble code starting at addr (default = current
 *                        PC or end of last "l" command)
 *    d <addr> [bytes]  Dump bytes starting at addr.  Default is 256 bytes
 *    w <addr> <byte-list>  Modify bytes at addr
 *    as [frame] [#]    Dump address stack starting from "frame"
 *    was <frame> <value>  Modify address stack frame
 *    cs [frame] [#]    Dump call stack starting from "frame"
 *    wcs <frame> <block-addr> <ret-addr>  Modify call stack frame
 *    b                 List current breakpoints
 *    ba <addr>         Add breakpoint at address
 *    bd <addr>         Remove breakpoint at address
 *    r [addr]          Run starting from [addr] (current PC)
 *    rr                Run until return
 *    s                 Single step into
 *    ss                Single step over
 *    hd [filename]     Dump the heap
 *    q                 Exit the VM
 *
 *  Values in angle brackets are required, while values in square brakets
 *  are optional
 *     
 *  An address can be a number, a symbol, a symbol+number or symbol-number.
 *  Frame numbers start at 0 for the top of the stack
 */

#include <brkpt.h>
#include <dbgcmd.h>
#include <vm.h>
#include <stddef.h>

typedef struct DebuggerImpl_* Debugger;

/** Create a new debugger for the given virtual machine */
Debugger createDebugger(UnlambdaVM vm, uint32_t maxBreakpoints);

/** Destroy a debugger and release the resources it uses */
void destroyDebugger(Debugger dbg);

int getDebuggerStatus(Debugger dbg);
const char* getDebuggerStatusMsg(Debugger dbg);
void clearDebuggerStatus(Debugger dbg);

UnlambdaVM getDebuggerVm(Debugger dbg);
BreakpointList getDebuggerPersistentBreakpoints(Debugger dbg);
BreakpointList getDebuggerTransientBreakpoints(Debugger dbg);

/** Clear all transient breakpoints */
int clearDebuggerTransientBreakpoints(Debugger dbg);

/** Returns true if VM execution should temporarily halt and enter debug
 *  mode
 */
int shouldBreakExecution(Debugger dbg);

/** Execute a debug command */
int executeDebugCommand(Debugger dbg, DebugCommand cmd);

#ifdef __cplusplus

const int DebuggerIllegalArgumentError = -1;
const int DebuggerInvalidCommandError = -2;
const int DebuggerCommandExecutionError = -3;
const int DebuggerResumeExecution = -4;
const int DebuggerQuitVm = -5;
const int DebuggerOperationFailedError = -6;

#else

const int DebuggerIllegalArgumentError;
const int DebuggerInvalidCommandError;
const int DebuggerCommandExecutionError;
const int DebuggerResumeExecution;
const int DebuggerQuitVm;
const int DebuggerOperationFailedError;

#endif

#endif

