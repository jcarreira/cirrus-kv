#include "src/utils/Time.h"
#include <iostream>

namespace cirrus {

TimerFunction::~TimerFunction() {
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

void TimerFunction::reset() {
    t0 = Time::now();
}

uint64_t TimerFunction::getUsElapsed() const {
    auto t1 = Time::now();
    us d_us = std::chrono::duration_cast<us>(t1 - t0);
    return d_us.count();
}

std::string getTimeNow() {
    //std::chrono::time_point<std::chrono::high_resolution_clock,
    //    std::chrono::milliseconds> start = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::now();
    auto t2 = std::chrono::time_point_cast<std::chrono::milliseconds>(t);

    auto res = t2.time_since_epoch().count();
    return to_string(res);
}

} // namespace cirrus
