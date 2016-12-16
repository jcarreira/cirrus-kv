#ifndef _TIME_H_
#define _TIME_H_

#include <chrono>
#include <string>
#include "src/utils/StringUtils.h"

namespace sirius {

std::string getTimeNow();

class TimerFunction {
public:
    TimerFunction(const std::string& name, bool print = false)
        : name_ (name),
        t0(Time::now()),
        print_(print) { }

    ~TimerFunction();

    uint64_t getUsElapsed() const;

    using Time = std::chrono::high_resolution_clock;
    using us = std::chrono::microseconds; 
    using ms = std::chrono::milliseconds; 

private:
    std::string name_;
    Time::time_point t0;
    bool print_;
};

} // namespace sirius

#endif // _TIME_H_
