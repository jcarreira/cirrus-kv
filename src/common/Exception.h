#ifndef SRC_COMMON_EXCEPTION_H_
#define SRC_COMMON_EXCEPTION_H_

#include <string>
#include <exception>

namespace cirrus {

// TODO(Tyler): Add in enum for exceptions

enum ErrorCodes {
  kOk = 0,
  kException,
  kServerMemoryErrorException,
  kNoSuchIDException,
};

/**
  * A class for cirrus generated exceptions.
  */
class Exception : public std::exception {
 public:
    explicit Exception(std::string msg): msg(msg) {}
    const char* what() const throw() {
        return msg.c_str();
    }
 private:
    std::string msg;
};

/**
  * An exception generated when the remote server experience an issue
  * during memory allocation. This could be due to the server running out
  * of memory.
  */
class ServerMemoryErrorException : public cirrus::Exception {
 public:
    explicit ServerMemoryErrorException(std::string msg):
        cirrus::Exception(msg) {}
};

/**
  * An exception generated when the local cache is completely filled and
  * the user attempts to perform a get operation, either directly or through
  * the use of an iterator. Also thrown if user sets max size of the
  * cache to be zero.
  */
class CacheCapacityException : public cirrus::Exception {
 public:
    explicit CacheCapacityException(std::string msg):
        cirrus::Exception(msg) {}
};

/**
  * An exception generated when the user requests a get operation on an
  * ObjectID that was either never pushed to the remote store or was removed
  * from the remote store.
  */
class NoSuchIDException : public cirrus::Exception {
 public:
    explicit NoSuchIDException(std::string msg):
        cirrus::Exception(msg) {}
};

/**
  * An exception generated when the client or server fail to make a connection
  * with the other.
  */
class ConnectionException : public cirrus::Exception {
 public:
    explicit ConnectionException(std::string msg):
        cirrus::Exception(msg) {}
};

/**
  * An exception generated when a read on the server fails.
  */
class ServerReadException : public cirrus::Exception {
 public:
    explicit ServerReadException(std::string msg):
        cirrus::Exception(msg) {}
};

}  // namespace cirrus

#endif  // SRC_COMMON_EXCEPTION_H_
