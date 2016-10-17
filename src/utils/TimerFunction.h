/* Copyright 2016 Joao Carreira */

#ifndef _TIMER_FUNCTION_H_
#define _TIMER_FUNCTION_H_

#include <string>
#include <chrono>

#include "src/utils/easylogging++.h"

namespace sirius {

class TimerFunction {
public:
    TimerFunction(const std::string& name, bool print = false)
        : name_ (name),
        t0(Time::now()),
        print_(print) { }

    ~TimerFunction() {
        if (!print_)
            return;

        auto t1 = Time::now();
        us d_us = std::chrono::duration_cast<us>(t1 - t0);
        ms d_ms = std::chrono::duration_cast<ms>(t1 - t0);

        std::cerr << name_ << ": " 
            << d_us.count() << "us "
            << d_ms.count() << "ms"
            << std::endl;
    }

    uint64_t getUsElapsed() const {
        auto t1 = Time::now();
        us d_us = std::chrono::duration_cast<us>(t1 - t0);
        return d_us.count();
    }

    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::microseconds us; 
    typedef std::chrono::milliseconds ms; 

private:
    std::string name_;
    Time::time_point t0;
    bool print_;
};

}

#endif // _TIMER_FUNCTION_H_
