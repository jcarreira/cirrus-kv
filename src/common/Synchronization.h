#ifndef SRC_COMMON_SYNCHRONIZATION_H_
#define SRC_COMMON_SYNCHRONIZATION_H_

#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <atomic>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <ctime>
#include <random>
#include <thread>
#include "common/Decls.h"
namespace cirrus {

/**
  * A class providing a general outline of a lock. Purely virtual.
  */
class Lock {
 public:
    Lock() = default;
    virtual ~Lock() = default;

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
        sem_name = random_string();
        m_sema = sem_open(sem_name, O_CREAT, S_IRWXU, initialCount);
        if (m_sema == SEM_FAILED) {
            switch (errno) {
                case EACCES:
                    printf("EACCESS");
                    break;
                case EEXIST:
                    printf("EEXIST");
                    break;
                case EINVAL:
                    printf("EINVAL");
                    break;
                case EMFILE:
                    printf("EMFILE");
                    break;
                case ENAMETOOLONG:
                    printf("ENAMETOOLONG");
                    break;
                case ENFILE:
                    printf("ENFILE");
                    break;
                case ENOENT:
                    printf("ENOENT");
                    break;
                case ENOMEM:
                    printf("ENOMEM");
                    break;
            }
            throw std::runtime_error("Creation of new semaphore failed");
        }
    }

    virtual ~PosixSemaphore() {
        sem_close(m_sema);
        sem_unlink(sem_name);
        delete[] sem_name;
    }

    /**
      * Waits until entered into semaphore.
      */
    void wait() final {
        int rc = sem_wait(m_sema);
        while (rc == -1 && errno == EINTR) {
            rc = sem_wait(m_sema);
        }
    }

    /**
      * Posts to one waiter
      */
    void signal() final {
        sem_post(m_sema);
    }

    /**
      * Posts to a specified number of waiters
      * @param count number of waiters to wake
      */
    void signal(int count) final {
        while (count-- > 0) {
            sem_post(m_sema);
        }
    }

    /**
      * Attempts to lock the semaphore and returns its success.
      * @return True if the semaphore had a positive value and was decremented.
      */
    bool trywait() final {
        int ret = sem_trywait(m_sema);
        if (ret == -1 && errno != EAGAIN) {
            throw std::runtime_error("trywait error");
        }
        return ret != -1;  // true for success
    }

 private:
    /** underlying semaphore that operations are performed on. */
    sem_t *m_sema;
    char  *sem_name;
    /** Length of randomly names for semaphores. */
    int rand_string_length = 16;

    /**
     * Method to generate random strings for named semaphores.
     */

    // Code from goo.gl/2NS4ri
    char* random_string() {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);

        char* str = new char[rand_string_length + 1];
        // First character of semaphore name must be a slash
        str[0] = '/';
        str[rand_string_length] = '\0';
        // Seed RNG
        const auto time_seed = static_cast<size_t>(std::time(0));
        const auto clock_seed = static_cast<size_t>(std::clock());
        const size_t pid_seed =
                  std::hash<std::thread::id>()(std::this_thread::get_id());

        std::seed_seq seed_value { time_seed, clock_seed, pid_seed };
        std::mt19937 gen;
        gen.seed(seed_value);

        for (int i = 1; i < rand_string_length; i++) {
            str[i] = charset[gen() % max_index];
        }
        return str;
    }
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
        while (lock.test_and_set(std::memory_order_acquire))
            continue;
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

#endif  // SRC_COMMON_SYNCHRONIZATION_H_
