#include <string>
#include <memory>
#include "common/Synchronization.h"
#include "common/Future.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
  * Constructor for Future.
  */
Future::Future(std::shared_ptr<bool> result,
               std::shared_ptr<cirrus::PosixSemaphore> sem):
	result(result), sem(sem) {}

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
        return sem_success;
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
