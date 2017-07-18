#include "client/BladeClient.h"

#include <string>
#include <memory>

#include "common/Synchronization.h"
#include "common/Exception.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
 * Constructor for ClientFuture.
 * @param result a std::shared_ptr that points to a boolean indicating
 * whether the operation was successful.
 * @param result_available a std::shared_ptr that points to a boolean
 * indicating whether the operation has completed and the result is available.
 * @param sem a std::shared_ptr to a cirrus::Lock that the future can wait on
 * for the result to become available.
 * @param error_code a std::shared_ptr to a cirrus::ErrorCodes that indicates
 * either success on the server or any errors that occured during the operation.
 */
BladeClient::ClientFuture::ClientFuture(std::shared_ptr<bool> result,
               std::shared_ptr<bool> result_available,
               std::shared_ptr<cirrus::Lock> sem,
               std::shared_ptr<cirrus::ErrorCodes> error_code):
    result(result), result_available(result_available),
    sem(sem), error_code(error_code) {}

/**
 * Waits until the result the future is monitoring is available.
 */
void BladeClient::ClientFuture::wait() {
    if (!*result_available) {
        sem->wait();
    }
}

/**
 * Checks the status of the result.
 * @return Returns true if the result is available, false otherwise.
 */
bool BladeClient::ClientFuture::try_wait() {
    if (!*result_available) {
        bool sem_success = sem->trywait();
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
bool BladeClient::ClientFuture::get() {
    wait();

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
