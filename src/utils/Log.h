/* Copyright 2016 Joao Carreira */

#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <utility>
#include "src/utils/Time.h"
#include "src/common/Synchronization.h"

namespace sirius {

struct INFO {
    static const int value = 5;
    constexpr static const char* prefix = "[INFO]";
};
struct ERROR {
    static const int value = 10;
    constexpr static const char* prefix = "[ERROR]";
};

static const int THRESHOLD = 5;

#if __GNUC__ >= 7
struct FLUSH { };
struct NO_FLUSH { };
struct TIME { };
struct NO_TIME { };
    
template<typename T, typename K = NO_FLUSH, typename TT = TIME, typename ... Params>
#else
template<typename T, typename ... Params>
#endif
bool LOG(Params&& ... param) {

    if (T::value < THRESHOLD)
        return true;

    std::cout << T::prefix
        << " -";

#if __GNUC__ >= 7
    if constexpr (std::is_same(TT, TIME)) {
        std::cout << getTimeNow();
    }
#else
    std::cout << getTimeNow();
#endif

    auto f = [](const auto& arg) {
        std::cout << " " << arg;
    };

 __attribute__((unused))
    int dummy[] = { 0, ( (void) f(std::forward<Params>(param)), 0) ... }; 

#if __GNUC__ >= 7
    if constexpr (std::is_same(K, FLUSH)) {
        std::cout << std::endl;
    }
#else
    std::cout << std::endl;
#endif
//    sl.signal();

    return true; // success
}

} // sirius

#endif // _LOG_H_
