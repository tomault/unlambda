#include "argparse.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CmdLineArgParserImpl_ {
  char** argv;
  int argc;
  int current;
  int statusCode;
  const char* statusMsg;
} CmdLineArgParserImpl;

static const char OK_MSG[] = "OK";
static const char DEFAULT_ERR_MSG[] = "ERROR";

const int NoMoreCmdLineArgsError = -1;
const int InvalidCmdLineArgError = -2;

CmdLineArgParser createCmdLineArgParser(int argc, char** argv) {
  CmdLineArgParser parser = malloc(sizeof(CmdLineArgParserImpl));

  if (parser) {
    parser->argv = argv;
    parser->argc = argc;
    parser->current = 0;
    parser->statusCode = 0;
    parser->statusMsg = OK_MSG;
  }

  return parser;
}

void destroyCmdLineArgParser(CmdLineArgParser parser) {
  clearCmdLineArgParserStatus(parser);
  free((void*)parser);
}

int getCmdLineArgParserStatus(CmdLineArgParser parser) {
  return parser->statusCode;
}

const char* getCmdLineArgParserStatusMsg(CmdLineArgParser parser) {
  return parser->statusMsg;
}

static int shouldDeallocateStatusMsg(CmdLineArgParser parser) {
  return parser->statusMsg && (parser->statusMsg != OK_MSG)
           && (parser->statusMsg != DEFAULT_ERR_MSG);
}

void clearCmdLineArgParserStatus(CmdLineArgParser parser) {
  if (shouldDeallocateStatusMsg(parser)) {
    free((void*)parser->statusMsg);
  }
  parser->statusCode = 0;
  parser->statusMsg = OK_MSG;
}

static void setCmdLineArgParserStatus(CmdLineArgParser parser, int code,
				      const char* msg) {
  if (shouldDeallocateStatusMsg(parser)) {
    free((void*)parser->statusMsg);
  }

  parser->statusCode = code;
  if (!msg || (msg == OK_MSG) || (msg == DEFAULT_ERR_MSG)) {
    parser->statusMsg = msg;
  } else {
    size_t msgLen = strlen(msg);
    parser->statusMsg = malloc(msgLen + 1);
    if (parser->statusMsg) {
      memcpy((char*)parser->statusMsg, msg, msgLen);
      ((char*)parser->statusMsg)[msgLen] = 0;
    } else {
      parser->statusMsg = DEFAULT_ERR_MSG;
    }
  }
}

static void setNoMoreArguments(CmdLineArgParser parser) {
  setCmdLineArgParserStatus(parser, NoMoreCmdLineArgsError,
			    "No more arguments");
}

int hasMoreCmdLineArgs(CmdLineArgParser parser) {
  return (parser->current + 1) < parser->argc;
}

const char* currentCmdLineArg(CmdLineArgParser parser) {
  return (parser->current && (parser->current < parser->argc))
           ? parser->argv[parser->current]
           : NULL;
}

const char* nextCmdLineArg(CmdLineArgParser parser) {
  clearCmdLineArgParserStatus(parser);
  if (!hasMoreCmdLineArgs(parser)) {
    parser->current = parser->argc;
    setNoMoreArguments(parser);
    return NULL;
  } else {
    return parser->argv[++parser->current];
  }
}

const char* nextCmdLineArgInSet(CmdLineArgParser parser,
				const char** options, uint32_t numOptions) {
  const char* next = nextCmdLineArg(parser);
  if (next) {
    for (size_t i = 0; i < numOptions; ++i) {
      if (!strcmp(next, options[i])) {
	return next;
      }
    }

    char msg[2048];
    char optionsList[1024];
    uint32_t current = 0;
    uint32_t remaining = 1023;
    for (size_t i = 0; (i < numOptions) && (remaining > 1); ++i) {
      if (i) {
	snprintf(optionsList + current, remaining, ", ");
	current += 2;
	remaining -= 2;
      }
      size_t optionLen = strlen(options[i]) + 2;
      snprintf(optionsList + current, remaining, "\"%s\"", options[i]);
      if (remaining < optionLen) {
	remaining = 0;
	current = sizeof(optionsList) - 1;
      } else {
	remaining -= optionLen;
	current += optionLen;
      }
    }

    snprintf(msg, sizeof(msg), "Value is \"%s\", but it should be one of: %s",
	     next, optionsList);
    setCmdLineArgParserStatus(parser, InvalidCmdLineArgError, msg);
    next = NULL;
  }
  return next;
}

uint8_t nextCmdLineArgAsUInt8(CmdLineArgParser parser) {
  uint64_t v = nextCmdLineArgAsUInt64(parser);
  if (!getCmdLineArgParserStatus(parser) && (v > 255)) {
    setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
			      "Value must be a nonnegative integer < 256");
    v = 0;
  }
  return v;
}

uint16_t nextCmdLineArgAsUInt16(CmdLineArgParser parser) {
  uint64_t v = nextCmdLineArgAsUInt64(parser);
  if (!getCmdLineArgParserStatus(parser) && (v > 65535)) {
    setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
			      "Value must be a nonnegative integer < 65536");
    v = 0;
  }
  return v;
}

uint32_t nextCmdLineArgAsUInt32(CmdLineArgParser parser) {
  uint64_t v = nextCmdLineArgAsUInt64(parser);
  if (!getCmdLineArgParserStatus(parser) && (v > 0xFFFFFFFF)) {
    setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
			      "Value must be a nonnegative integer "
			      "< 4294967296");
    v = 0;
  }
  return v;
}

static const uint64_t MAX_UINT64_DIV_10 = (uint64_t)0xFFFFFFFFFFFFFFFF / 10;
static const uint64_t MAX_UINT64_MOD_10 = (uint64_t)0xFFFFFFFFFFFFFFFF % 10;

static const char* parseUInt64(CmdLineArgParser parser, const char* start,
			       uint64_t* value) {
  const char* p = start;
  uint64_t v = 0;

  while (*p && isdigit(*p)) {
    uint64_t d = (*p) - '0';
    if ((v > MAX_UINT64_DIV_10)
	  || ((v == MAX_UINT64_DIV_10) && (d > MAX_UINT64_MOD_10))) {
      setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
				"Value is too large");
      return NULL;
    }
    v = v * 10 + d;
    ++p;
  }

  *value = v;
  return p;
}

uint64_t nextCmdLineArgAsUInt64(CmdLineArgParser parser) {
  const char* next = nextCmdLineArg(parser);
  if (!next) {
    return 0;
  } else {
    uint64_t v = 0;
    const char* end = parseUInt64(parser, next, &v);
    if (end && *end) {
      setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
				"Value must be a nonnegative integer");
      return 0;
    }
    return v;
  }
}

static void setUnknownSizeSuffix(CmdLineArgParser parser, const char* suffix) {
  char msg[200];
  snprintf(msg, sizeof(msg), "Unknown size suffix \"%s\"", suffix);
  setCmdLineArgParserStatus(parser, InvalidCmdLineArgError, msg);
}

uint64_t nextCmdLineArgAsMemorySize(CmdLineArgParser parser) {
  const char* next = nextCmdLineArg(parser);
  if (!next) {
    return 0;
  } else {
    uint64_t v = 0;
    const char* p = parseUInt64(parser, next, &v);
    if (p) {
      char suffix = *p++;
      if (suffix && *p) {
	setUnknownSizeSuffix(parser, p - 1);
	return 0;
      }
      switch (suffix) {
        case 0:
	  /** No suffix, so value is in bytes */
	  break;

        case 'k':
        case 'K':
	  /** Value is in kilobytes */
	  if (v >= ((uint64_t)1 << 54)) {
	    setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
				      "Value is too large");
	    return 0;
	  }
	  v *= 1024;
	  break;

        case 'm':
        case 'M':
	  /** Value is in megabytes */
	  if (v >= ((uint64_t)1 << 44)) {
	    setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
				      "Value is too large");
	    return 0;
	  }
	  v *= 1024 * 1024;
	  break;

        case 'g':
        case 'G':
	  /** Value is in gigabytes */
	  if (v >= ((uint64_t)1 << 34)) {
	    setCmdLineArgParserStatus(parser, InvalidCmdLineArgError,
				      "Value is too large");
	    return 0;
	  }
	  v *= 1024 * 1024 * 1024;
	  break;

        default:
	  setUnknownSizeSuffix(parser, p - 1);
	  v = 0;
	  break;
      }
    }
    return v;
  }
}
