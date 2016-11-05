/* Copyright 2016 Joao Carreira */

#include "src/utils/TimerFunction.h"
#include <iostream>

namespace sirius {

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

uint64_t TimerFunction::getUsElapsed() const {
    auto t1 = Time::now();
    us d_us = std::chrono::duration_cast<us>(t1 - t0);
    return d_us.count();
}

}  // namespace sirius
