#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <string>
#include <exception>

namespace cirrus {

class Exception : public std::exception {
public:
    explicit Exception(std::string msg): msg(msg) {}
    const char* what() const throw() {
        return msg.c_str();
    }
private:
    std::string msg;
};

class ServerMemoryErrorException : public cirrus::Exception {
public:
    explicit ServerMemoryErrorException(std::string msg):
        cirrus::Exception(msg) {}
};

}  //namespace cirrus

#endif  // _EXCEPTION_H_
