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

#include <vm.h>
#include <stddef.h>

typedef struct DebuggerImpl Debugger;

/** Create a new debugger for the given virtual machine */
Debugger createDebugger(UnlambdaVM vm);

/** Destroy a debugger and release the resources it uses */
void destroyDebugger(Debugger dbg);

/** Returns true if VM execution should temporarily halt and enter debug
 *  mode
 */
int shouldBreakExecution(Debugger dbg);

/** Read a command from the console */
const char* readDebugCommand(Debugger dbg, char* buffer, size_t bufferSize);

/** Parse and execute a debug command */
int parseAndExecuteDebugCommand(Debugger dbg, const char* command);

#endif

