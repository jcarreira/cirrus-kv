/* Copyright 2016 Joao Carreira */

#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <utility>

namespace sirius {

struct INFO {
    static const int value = 5;
};
struct ERROR {
    static const int value = 10;
};

template<typename T, typename ... Params>
bool LOG(Params&& ... param) {
    auto f = [](const auto& arg) {
        std::cout << " " << arg;
    };

 __attribute__((unused))
    int dummy[] = { 0, ( (void) f(std::forward<Params>(param)), 0) ... }; 

    std::cout << std::endl;

    return true; // success
}

//class Log : public std::ostream {
//    std::ostream& operator<<(const std::string& s) {
//        std::cerr << s << std::flush;
//    }
//};

} // sirius

#endif // _LOG_H_
