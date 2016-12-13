/* Copyright 2016 Joao Carreira */

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <error.h>
#include <semaphore.h>

namespace sirius {

class Semaphore {
public:
    Semaphore(int initialCount = 0) {
        sem_init(&m_sema, 0, initialCount);
    }

    virtual ~Semaphore() {
        sem_destroy(&m_sema);
    }

    void wait() {
        int rc;
        do {
            rc = sem_wait(&m_sema);
        }
        while (rc == -1 && errno == EINTR);
    }

    void signal() {
        sem_post(&m_sema);
    }

    void signal(int count) {
        while (count-- > 0) {
            sem_post(&m_sema);
        }
    }
    
    bool trywait() {
        int ret = sem_trywait(&m_sema);
        if (ret == -1 && errno != EAGAIN)
            throw std::runtime_error("trywait error");
        return ret != -1; // true for success
    }
private:
    sem_t m_sema;

    Semaphore(const Semaphore& other) = delete;
    Semaphore& operator=(const Semaphore& other) = delete;

};

}

#endif // _SEMAPHORE_H_
