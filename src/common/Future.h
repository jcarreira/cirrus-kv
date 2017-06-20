#ifndef SRC_COMMON_FUTURE_H_
#define SRC_CLIENT_BLADECLIENT_H_

#include <string>
#include "common/Synchronization.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level. Used to monitor
  * operations that have a true/false result.
  */
class Future {
 public:
    Future(std::shared_ptr<bool> result,
           std::shared_ptr<cirrus::PosixSemaphore> sem);

    void wait();

    bool try_wait();

    bool get();
 private:
    std::shared_ptr<bool> result; /**< Pointer to the result. */
    std::shared_ptr<cirrus::PosixSemaphore> sem; /**< Sem for the result. */
    bool result_available = false; /** Boolean monitoring result state. */
};

/**
  * Waits until the result the future is monitoring is available.
  */
void Future::wait() {
    if (!result_available) {
        sem->wait();
        result_available = true;
    }
}

/**
  * Checks the status of the result.
  * @return Returns true if the result is available, false otherwise.
  */
bool Future::try_wait() {
    if (!result_available) {
        bool sem_success = sem->trywait();
        result_available = sem_success;
        return result;
    } else {
        return true;
    }
}

/**
  * Returns the result of the asynchronous operation. If result is not
  * yet available, waits until it is ready.
  * @return Returns the result given by the asynchronous operation.
  */
bool Future::get() {
    if (!result_available) {
        sem->wait();
        result_available = true;
    }

    return *result;
}

}  // namespace cirrus

#endif  // SRC_CLIENT_BLADECLIENT_H_
