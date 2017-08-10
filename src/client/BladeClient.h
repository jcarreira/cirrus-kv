#ifndef SRC_CLIENT_BLADECLIENT_H_
#define SRC_CLIENT_BLADECLIENT_H_

#include <string>
#include <memory>

#include "common/Future.h"
#include "common/Serializer.h"
#include "common/Synchronization.h"
#include "common/Exception.h"
#include "utils/Log.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level.
  */
template<class T>
class BladeClient {
 public:
    class ClientFuture {
     public:
        ClientFuture(std::shared_ptr<bool> result,
               std::shared_ptr<bool> result_available,
               std::shared_ptr<cirrus::Lock> sem,
               std::shared_ptr<cirrus::ErrorCodes> error_code);
        ClientFuture() {}
        void wait();

        bool try_wait();

        bool get();
     private:
        /** Pointer to the result. */
        std::shared_ptr<bool> result;

        /** Boolean monitoring result state. */
        std::shared_ptr<bool> result_available;

        /** Sem for the result. */
        std::shared_ptr<cirrus::Lock> sem;

        /** Any errors thrown. */
        std::shared_ptr<cirrus::ErrorCodes> error_code;
    };

    virtual ~BladeClient() = default;

    virtual void connect(const std::string& address,
                         const std::string& port) = 0;

    virtual bool write_sync(ObjectID id,  const T& obj,
        const Serializer<T>& serializer) = 0;

    virtual bool read_sync(ObjectID id, void* data, uint64_t size) = 0;

    virtual bool remove(ObjectID id) = 0;

    virtual BladeClient::ClientFuture write_async(ObjectID oid,
        const T& obj,
        const Serializer<T>& serializer) = 0;

    virtual BladeClient::ClientFuture read_async(ObjectID oid, void* data,
        uint64_t size) = 0;
};

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
template<class T>
BladeClient<T>::ClientFuture::ClientFuture(std::shared_ptr<bool> result,
               std::shared_ptr<bool> result_available,
               std::shared_ptr<cirrus::Lock> sem,
               std::shared_ptr<cirrus::ErrorCodes> error_code):
    result(result), result_available(result_available),
    sem(sem), error_code(error_code) {}

/**
 * Waits until the result the future is monitoring is available.
 */
template<class T>
void BladeClient<T>::ClientFuture::wait() {
    while (!*result_available) {
        LOG<INFO>("Result not available, waiting.");
        sem->wait();
    }
}

/**
 * Checks the status of the result.
 * @return Returns true if the result is available, false otherwise.
 */
template<class T>
bool BladeClient<T>::ClientFuture::try_wait() {
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
template<class T>
bool BladeClient<T>::ClientFuture::get() {
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

#endif  // SRC_CLIENT_BLADECLIENT_H_
