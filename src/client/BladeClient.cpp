#include "client/BladeClient.h"

#include <string>
#include <memory>

#include "common/Synchronization.h"
#include "common/Exception.h"
#include "utils/Log.h"

namespace cirrus {

using ObjectID = uint64_t;

FutureData::FutureData(
            bool result,
            bool result_available,
            cirrus::ErrorCodes error_code,
            std::shared_ptr<const char> data_ptr,
            uint64_t data_size) :
    result(result), result_available(result_available),
    error_code(error_code), data_ptr(data_ptr), data_size(data_size) {
    sem = std::make_shared<cirrus::SpinLock>();
}

/**
 * Constructor for ClientFuture.
 * @param result a std::shared_ptr that points to a boolean indicating
 * whether the operation was successful.
 * @param result_available a shared pointer to a boolean indicating whether
 * the result is available yet.
 * @param sem a shared_ptr to a cirrus::Lock, used to wait for the
 * result.
 * @param error_code a std::shared_ptr to a cirrus::ErrorCodes that indicates
 * either success on the server or any errors that occured during the operation.
 * @param data_ptr a shared_ptr to a shared_ptr that points to the buffer
 * used to receive an object from the remote store, if any.
 * @param data_size the length of the buffer used to receive data from the
 * store.
 */
BladeClient::ClientFuture::ClientFuture(std::shared_ptr<FutureData> fd) :
    fd(fd) {}

/**
 * Waits until the result the future is monitoring is available.
 */
void BladeClient::ClientFuture::wait() {
    while (!fd->result_available) {
        LOG<INFO>("Result not available, waiting.");
        fd->sem->wait();
    }
}

/**
 * Checks the status of the result.
 * @return Returns true if the result is available, false otherwise.
 */
bool BladeClient::ClientFuture::try_wait() {
    if (!fd->result_available) {
        return fd->sem->trywait();
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
    LOG<INFO>("Waiting for result.");
    wait();
    LOG<INFO>("Result is available.");
    LOG<INFO>("Error code is: ", fd->error_code);

    // Check the error code enum. Throw exception if one happened on server.
    switch (fd->error_code) {
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
    return fd->result;
}

/**
 * Returns a std::pair with a pointer to buffer that serialized object was read
 * into and length of buffer. Will throw an exception if called on a Future
 * that was not used for a read.
 */
std::pair<std::shared_ptr<const char>, unsigned int>
BladeClient::ClientFuture::getDataPair() {
    // Wait until result is available and throw exception if necessary
    get();
    if (fd->data_ptr.get() == nullptr) {
        throw cirrus::Exception("getData called on a non read future");
    }
    return std::make_pair(fd->data_ptr, fd->data_size);
}


}  // namespace cirrus
