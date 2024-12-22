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

#endif
