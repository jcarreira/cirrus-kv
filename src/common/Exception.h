/* Copyright 2016 Joao Carreira */

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

namespace cirrus {

class Exception: public std::exception {
    public:
        Exception(std::string msg): msg(msg) {}
        const char* what() {
          return msg.c_str();
        }
    private:
        std::string msg;
};


}

#endif // _EXCEPTION_H_
