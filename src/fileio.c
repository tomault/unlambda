#include "fileio.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int openFile(const char* filename, int flags, int mode, const char** errMsg) {
  int fd = open(filename, flags, mode);
  if (fd < 0) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Error opening %s: %s", filename,
	     strerror(errno));
    *errMsg = strdup(msg);
    return -1;
  }

  return fd;
}

int readFromFile(const char* filename, int fd, void* buffer, size_t n,
		 const char** errMsg) {
  *errMsg = NULL;
  
  ssize_t nRead = read(fd, buffer, n);
  if (nRead < 0) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Error reading from %s: %s", filename,
	     strerror(errno));
    *errMsg = strdup(msg);
    return -1;
  } else if (nRead != n) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Error reading from %s: Attempted to read %zu bytes, but "
	     "read only %zd bytes", filename, n, nRead);
    *errMsg = strdup(msg);
    return -1;
  }

  return 0;
}

int writeToFile(const char* filename, int fd, const void* buffer, size_t n,
		const char** errMsg) {
  *errMsg = NULL;
  
  ssize_t nWritten = write(fd, buffer, n);
  if (nWritten < 0) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Error writing to %s: %s", filename,
	     strerror(errno));
    *errMsg = strdup(msg);
    return -1;
  } else if (nWritten != n) {
    char msg[200];
    snprintf(msg, sizeof(msg),
	     "Error writing to %s: Attempted to write %zu bytes, but "
	     "wrote only %zd bytes", filename, n, nWritten);
    *errMsg = strdup(msg);
    return -1;
  }

  return 0;
}


