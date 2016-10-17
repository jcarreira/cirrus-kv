/* Copyright 2016 Joao Carreira */

#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>

namespace sirius {

class Log : public std::ostream {
    //std::ostream operator<<(int i) {
    //    return std::cerr << i;// << std::flush;
    //}
    std::ostream& operator<<(const std::string& s) {
        std::cerr << s << std::flush;
    }
};

} // sirius

#endif // _LOG_H_
