#ifndef _SYNCHRONIZATION_H_
#define _SYNCHRONIZATION_H_

#include <errno.h>
#include <error.h>
#include <semaphore.h>
#include <atomic>
#include <stdexcept>
#include "common/Decls.h"

namespace cirrus {

/**
  * A class providing a general outline of a lock. Purely virtual.
  */
class Lock {
 public:
    Lock() {
    }

    virtual ~Lock() {
    }

    virtual void wait() = 0; /** A pure virtual member. */
    virtual void signal() = 0; /** A pure virtual member. */
    virtual void signal(int count) = 0; /** A pure virtual member. */

    /**
      * A pure virtual member.
      * @return true if lock has succeeded
      */
    virtual bool trywait() = 0;

 private:
    DISALLOW_COPY_AND_ASSIGN(Lock);
};

/**
  * A class that extends the Lock class. Makes use of sem_t and its
  * associated methods to fullfill the functions of the lock class.
  */
class PosixSemaphore : public Lock {
 public:
    explicit PosixSemaphore(int initialCount = 0) : Lock() {
        sem_init(&m_sema, 0, initialCount);
    }

    virtual ~PosixSemaphore() {
        sem_destroy(&m_sema);
    }

    /**
      * Waits until entered into semaphore.
      */
    void wait() final {
        int rc;
        do {
            rc = sem_wait(&m_sema);
        }
        while (rc == -1 && errno == EINTR);
    }

    /**
      * Posts to one waiter
      */
    void signal() final {
        sem_post(&m_sema);
    }

    /**
      * Posts to a specified number of waiters
      * @param count number of waiters to wake
      */
    void signal(int count) final {
        while (count-- > 0) {
            sem_post(&m_sema);
        }
    }

    /**
      * Attempts to lock the semaphore and returns its success.
      * @return True if the semaphore had a positive value and was decremented.
      */
    bool trywait() final {
        int ret = sem_trywait(&m_sema);
        if (ret == -1 && errno != EAGAIN) {
            throw std::runtime_error("trywait error");
        }
        return ret != -1;  // true for success
    }

 private:
    sem_t m_sema; /**< underlying semaphore that operations are performed on. */
};

/**
  * A lock that extends the Lock class. Utilizes spin waiting.
  */
class SpinLock : public Lock {
 public:
    SpinLock() :
        Lock()
    { }

    virtual ~SpinLock() = default;

    /**
      * This function busywaits until it obtains the lock.
      */
    void wait() final {
        while (lock.test_and_set(std::memory_order_acquire));
    }

    /**
      * This function attempts to obtain the lock once.
      * @return true if the lock was obtained, false otherwise.
      */
    bool trywait() final {
        return lock.test_and_set(std::memory_order_acquire) == 0;
    }

    void signal(__attribute__((unused)) int count) final {
        throw std::runtime_error("Not implemented");
    }

    void signal() final {
        lock.clear(std::memory_order_release);
    }

 private:
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
};

}  // namespace cirrus

#endif  // _SYNCHRONIZATION_H_
