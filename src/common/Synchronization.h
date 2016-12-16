/* Copyright 2016 Joao Carreira */

#ifndef _SYNCHRONIZATION_H_
#define _SYNCHRONIZATION_H_

#include <error.h>
#include <semaphore.h>
#include <atomic>
#include <stdexcept>
#include <errno.h>
//#include "src/utils/logging.h"
#include "src/common/Decls.h"

namespace sirius {

class Lock {
public:
    Lock() {
    }

    virtual ~Lock() {
    }

    virtual void wait() = 0;

    virtual void signal() = 0;

    virtual void signal(int count) = 0;

    // return true if lock has succeeded    
    virtual bool trywait() = 0;
private:
    DISALLOW_COPY_AND_ASSIGN(Lock);
};

class PosixSemaphore : public Lock {
public:
    PosixSemaphore(int initialCount = 0) : Lock() {
        sem_init(&m_sema, 0, initialCount);
    }

    virtual ~PosixSemaphore() {
        sem_destroy(&m_sema);
    }

    void wait() override final {
        int rc;
        do {
            rc = sem_wait(&m_sema);
        }
        while (rc == -1 && errno == EINTR);
    }

    void signal() override final {
        sem_post(&m_sema);
    }

    void signal(int count) override final {
        while (count-- > 0) {
            sem_post(&m_sema);
        }
    }
    
    bool trywait() override final {
        int ret = sem_trywait(&m_sema);
        if (ret == -1 && errno != EAGAIN) {
            throw std::runtime_error("trywait error");
        }
        return ret != -1; // true for success
    }
private:
    sem_t m_sema;
};

class SpinLock : public Lock {
public:
    SpinLock() : 
        Lock()
    { }

    virtual ~SpinLock() = default;

    void wait() override final {
        while (lock.test_and_set(std::memory_order_acquire))
            ;
    }
    
    bool trywait() override final {
        return lock.test_and_set(std::memory_order_acquire) == 0;
    }
    
    void signal(__attribute__((unused)) int count) override final {
        throw std::runtime_error("Not implemented");
    }

    void signal() override final {
        lock.clear(std::memory_order_release);
    }

private:
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
};

}

#endif // _SYNCHRONIZATION_H_
