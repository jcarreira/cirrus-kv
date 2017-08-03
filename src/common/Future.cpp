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
  * @param result A shared pointer to the boolean success of the operation
  * @param result_available a shared pointer to a boolean indicating whether
  * the result is available yet.
  * @param sem a shared_ptr to a cirrus::PosixSemaphore, used to wait for the
  * result.
  * @param data_ptr a shared_ptr to a shared_ptr that points to the buffer
  * used to receive an object from the remote store, if any.
  * @param data_size the length of the buffer used to receive data from the
  * store.
  */
Future::Future(std::shared_ptr<bool> result,
               std::shared_ptr<bool> result_available,
               std::shared_ptr<cirrus::PosixSemaphore> sem,
               std::shared_ptr<cirrus::ErrorCodes> error_code,
               std::shared_ptr<std::shared_ptr<char>> data_ptr,
               std::shared_ptr<uint64_t> data_size):
    result(result), result_available(result_available),
    sem(sem), error_code(error_code), data_ptr(data_ptr),
    data_size(data_size) {}

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

/**
 * Returns a std::pair with a pointer to buffer that serialized object was read
 * into and length of buffer. Will throw an exception if called on a Future
 * that was not used for a read.
 */
std::pair<std::shared_ptr<char>, unsigned int> Future::getDataPair() {
    // Wait until result is available and throw exception if necessary
    get();
    if (data_ptr.get() == nullptr) {
        throw cirrus::Exception("getData called on a non read future");
    }
    return std::make_pair(*data_ptr, *data_size);
}


}  // namespace cirrus
