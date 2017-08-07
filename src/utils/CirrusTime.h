#ifndef SRC_UTILS_CIRRUSTIME_H_
#define SRC_UTILS_CIRRUSTIME_H_

#include <chrono>
#include <string>
#include "utils/StringUtils.h"

namespace cirrus {

std::string getTimeNow();

class TimerFunction {
 public:
    explicit TimerFunction(const std::string& name = "", bool print = false)
        : name_(name),
        t0(Time::now()),
        print_(print) { }

    ~TimerFunction();

    void reset();

    uint64_t getNsElapsed() const;
    double getUsElapsed() const;

    using Time = std::chrono::high_resolution_clock;
    using ns = std::chrono::nanoseconds;
    using us = std::chrono::microseconds;
    using ms = std::chrono::milliseconds;

 private:
    std::string name_;
    Time::time_point t0;
    bool print_;
};

}  // namespace cirrus

#endif  // SRC_UTILS_CIRRUSTIME_H_
