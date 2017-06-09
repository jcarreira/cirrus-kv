/* Copyright 2016 Joao Carreira */

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <string>
#include <exception>

namespace cirrus {

class Exception : public std::exception {
    public:
        Exception(std::string msg): msg(msg) {}
        const char* what() const throw() {
          return msg.c_str();
        }
    private:
        std::string msg;
};

class ServerMemoryErrorException : public cirrus::Exception {
    public:
        ServerMemoryErrorException(std::string msg): 
            cirrus::Exception(msg) {}
};

}

#endif // _EXCEPTION_H_
