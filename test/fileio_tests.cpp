extern "C" {
#include <fileio.h>
}

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
  std::string makeTestFileName() {
    std::ostringstream name;
    name << "/tmp/fileio_tests_" << getpid() << ".txt";
    return name.str();
  }
}

TEST(fileio_tests, OpenNewFile) {
  std::string filename = makeTestFileName();
  const char* errMsg = NULL;

  const mode_t oldUmask = ::umask(0);

  int fd = openFile(filename.c_str(), O_CREAT|O_WRONLY, 0666, &errMsg);

  try {
    if (fd < 0) {
      ::free((void*)errMsg);
      FAIL() << "Failed to open file " << filename << " (" << ::strerror(errno)
	     << ")";
    }
    EXPECT_EQ(errMsg, (const char*)0);
    ::close(fd);
    fd = -1;

    struct stat fileStats;

    if (::stat(filename.c_str(), &fileStats)) {
      FAIL() << "Could not stat " << filename << " (" << ::strerror(errno)
	     << ")";
    }

    EXPECT_EQ((fileStats.st_mode & S_IFMT), S_IFREG);
    EXPECT_EQ((fileStats.st_mode & 07777), 0666);

    ::unlink(filename.c_str());
    ::umask(oldUmask);

  } catch(...) {
    if (fd >= 0) {
      ::close(fd);
    }
    ::unlink(filename.c_str());
    ::umask(oldUmask);
    throw;
  }
}

TEST(fileio_tests, OpenExistingFile) {
  const char TEST_FILE_TEXT[] = "This is a test.";
  std::string filename = makeTestFileName();
  const char* errMsg = NULL;
  
  {
    std::ofstream testFile(filename);
    testFile << TEST_FILE_TEXT;
  }

  int fd = openFile(filename.c_str(), O_RDONLY, 0, &errMsg);
  try {
    if (fd < 0) {
      ::free((void*)errMsg);
      FAIL() << "Failed to open file " << filename << " (" << ::strerror(errno)
	     << ")";
    }

    char buf[sizeof(TEST_FILE_TEXT)];
    ssize_t n = ::read(fd, buf, sizeof(TEST_FILE_TEXT) - 1);
    if (n < 0) {
      FAIL() << "Failed to read from " << filename << " (" << ::strerror(errno)
	     << ")";
    }
    buf[n] = 0;
    EXPECT_EQ(std::string(buf), std::string(TEST_FILE_TEXT));

    ::close(fd);
    ::unlink(filename.c_str());
  } catch(...) {
    ::close(fd);
    ::unlink(filename.c_str());
    throw;
  }
}

TEST(fileio_tests, ReadFromFile) {
  const char TEST_FILE_TEXT[] = "This is a test.";
  std::string filename = makeTestFileName();
  const char* errMsg = NULL;
  
  {
    std::ofstream testFile(filename);
    testFile << TEST_FILE_TEXT;
  }

  int fd = openFile(filename.c_str(), O_RDONLY, 0, &errMsg);
  try {
    if (fd < 0) {
      ::free((void*)errMsg);
      FAIL() << "Failed to open file " << filename << " (" << ::strerror(errno)
	     << ")";
    }

    char txt[sizeof(TEST_FILE_TEXT)];
    if (readFromFile(filename.c_str(), fd, (void*)txt,
		     sizeof(TEST_FILE_TEXT) - 1, &errMsg)) {
      std::string tmp(errMsg);
      free((void*)errMsg);
      FAIL() << "Failed to read from file " << filename << " (" << tmp << ")";
    }
    txt[sizeof(TEST_FILE_TEXT) - 1] = 0;

    EXPECT_EQ(std::string(txt), std::string(TEST_FILE_TEXT));

    ::close(fd);
    ::unlink(filename.c_str());
  } catch(...) {
    ::close(fd);
    ::unlink(filename.c_str());
    throw;
  }
}

TEST(fileio_tests, ReadMoreDataThanFileHas) {
  const char TEST_FILE_TEXT[] = "This is a test.";
  std::string filename = makeTestFileName();
  const char* errMsg = NULL;
  
  {
    std::ofstream testFile(filename);
    testFile << TEST_FILE_TEXT;
  }

  int fd = openFile(filename.c_str(), O_RDONLY, 0, &errMsg);
  try {
    if (fd < 0) {
      ::free((void*)errMsg);
      FAIL() << "Failed to open file " << filename << " (" << ::strerror(errno)
	     << ")";
    }

    char txt[2 * sizeof(TEST_FILE_TEXT)];
    int result = readFromFile(filename.c_str(), fd, (void*)txt,
			      2 * sizeof(TEST_FILE_TEXT), &errMsg);
    ASSERT_NE(result, 0);

    std::ostringstream truth;
    truth << "Error reading from " << filename << ": Attempted to read "
	  << 2 * sizeof(TEST_FILE_TEXT) << " bytes, but read only "
	  << (sizeof(TEST_FILE_TEXT) - 1) << " bytes";
    EXPECT_EQ(std::string(errMsg), truth.str());

    ::free((void*)errMsg);
    ::close(fd);
    ::unlink(filename.c_str());
  } catch(...) {
    ::close(fd);
    ::unlink(filename.c_str());
    throw;
  }
}

TEST(fileio_tests, WriteToFile) {
  const char TEST_FILE_TEXT[] = "This is a test.";
  std::string filename = makeTestFileName();
  const char* errMsg = NULL;

  int fd = openFile(filename.c_str(), O_CREAT|O_EXCL|O_WRONLY, 0666, &errMsg);
  if (fd < 0) {
      ::free((void*)errMsg);
      FAIL() << "Failed to create file " << filename
	     << " (" << ::strerror(errno) << ")";
  }

  if (writeToFile(filename.c_str(), fd, (const void*)TEST_FILE_TEXT,
		  sizeof(TEST_FILE_TEXT) - 1, &errMsg)) {
    std::string tmp(errMsg);
    ::free((void*)errMsg);
    ::close(fd);
    ::unlink(filename.c_str());
    FAIL() << "Failed to write to file " << filename << " (" << tmp  << ")";
  }

  ::close(fd);

  fd = openFile(filename.c_str(), O_RDONLY, 0, &errMsg);
  if (fd < 0) {
    ::free((void*)errMsg);
    FAIL() << "Failed to open file " << filename << " for verification ("
	   << strerror(errno) << ")";
  }

  char txt[2 * sizeof(TEST_FILE_TEXT) + 1];
  ssize_t n = ::read(fd, (void*)txt, 2 * sizeof(TEST_FILE_TEXT));
  if (n < 0) {
    ::close(fd);
    ::unlink(filename.c_str());
    FAIL() << "Failed to read from file " << filename << " ("
	   << ::strerror(errno) << ")";
  }
  txt[n] = 0;
  EXPECT_EQ(std::string(txt), std::string(TEST_FILE_TEXT));

  ::close(fd);
  ::unlink(filename.c_str());
}

