#ifndef __UNLAMBDA__ARGPARSE_H__
#define __UNLAMBDA__ARGPARSE_H__

#include <stdint.h>

typedef struct CmdLineArgParserImpl_* CmdLineArgParser;

CmdLineArgParser createCmdLineArgParser(int argc, char** argv);
void destroyCmdLineArgParser(CmdLineArgParser parser);

int getCmdLineArgParserStatus(CmdLineArgParser parser);
const char* getCmdLineArgParserStatusMsg(CmdLineArgParser parser);
void clearCmdLineArgParserStatus(CmdLineArgParser parser);

int hasMoreCmdLineArgs(CmdLineArgParser parser);
const char* currentCmdLineArg(CmdLineArgParser parser);

const char* nextCmdLineArg(CmdLineArgParser parser);
const char* nextCmdLineArgInSet(CmdLineArgParser parser,
				const char** options, uint32_t numOptions);
uint8_t nextCmdLineArgAsUInt8(CmdLineArgParser parser);
uint16_t nextCmdLineArgAsUInt16(CmdLineArgParser parser);
uint32_t nextCmdLineArgAsUInt32(CmdLineArgParser parser);
uint64_t nextCmdLineArgAsUInt64(CmdLineArgParser parser);
uint64_t nextCmdLineArgAsMemorySize(CmdLineArgParser parser);

#ifdef __cplusplus
const int NoMoreCmdLineArgsError = -1;
const int InvalidCmdLineArgError = -2;
#else
const int NoMoreCmdLineArgsError;
const int InvalidCmdLineArgError;
#endif

#endif
