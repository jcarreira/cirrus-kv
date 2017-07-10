#include <string>
#include <memory>
#include "common/Synchronization.h"
#include "common/Future.h"
#include "common/Exception.h"
#include "utils/Log.h"
namespace cirrus {

using ObjectID = uint64_t;

/**
  * Constructor for Future.
  */
Future::Future(std::shared_ptr<bool> result,
               std::shared_ptr<bool> result_available,
               std::shared_ptr<cirrus::PosixSemaphore> sem,
               std::shared_ptr<cirrus::ErrorCodes> error_code):
    result(result), result_available(result_available),
    sem(sem), error_code(error_code) {}

/**
  * Waits until the result the future is monitoring is available.
  */
void Future::wait() {
    // Wait on the semaphore as long as the result is not available
    while (!*result_available) {
        LOG<INFO>("Result not available, waiting.");
        sem->wait();
    }
}

/**
  * Checks the status of the result.
  * @return Returns true if the result is available, false otherwise.
  */
bool Future::try_wait() {
    if (!*result_available) {
        return sem->trywait();
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
    // Wait until result is available
    LOG<INFO>("Waiting for result.");
    wait();
    LOG<INFO>("Result is available.");
    LOG<INFO>("Error code is: ", *error_code);
    // Check the error code enum. Throw exception if one happened on server.
    switch (*error_code) {
      case cirrus::ErrorCodes::kOk: {
        break;
      }
      case cirrus::ErrorCodes::kException: {
        throw cirrus::Exception("Server threw generic exception.");
      }
      case cirrus::ErrorCodes::kServerMemoryErrorException: {
        throw cirrus::ServerMemoryErrorException("Server memory exhausted "
                                                 "during call to put.");
      }
      case cirrus::ErrorCodes::kNoSuchIDException: {
        throw cirrus::NoSuchIDException("Call to get was made for id that "
                                        "did not exist on server.");
      }
      default: {
        throw cirrus::Exception("Unrecognized error code during get().");
      }
    }
    return *result;
}

}  // namespace cirrus
