/* Copyright 2016 Joao Carreira */

#ifndef _TIMER_FUNCTION_H_
#define _TIMER_FUNCTION_H_

#include <string>
#include <chrono>

namespace sirius {

class TimerFunction {
public:
    TimerFunction(const std::string& name, bool print = false)
        : name_ (name),
        t0(Time::now()),
        print_(print) { }

    ~TimerFunction();

    uint64_t getUsElapsed() const;

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
