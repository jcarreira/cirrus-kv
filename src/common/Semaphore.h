/* Copyright 2016 Joao Carreira */

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace sirius {

class Semaphore {
public:
    Semaphore()
        : count_()
    {}

    void notify() {
        boost::mutex::scoped_lock lock(mutex_);
        ++count_;
        condition_.notify_one();
    }

    void wait() {
        boost::mutex::scoped_lock lock(mutex_);
        while(!count_)
            condition_.wait(lock);
        --count_;
    }

private:
    boost::mutex mutex_;
    boost::condition_variable condition_;
    unsigned long count_;
};

}

#endif // _SEMAPHORE_H_
