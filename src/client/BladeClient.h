#ifndef SRC_CLIENT_BLADECLIENT_H_
#define SRC_CLIENT_BLADECLIENT_H_

#include <string>
#include <memory>

namespace cirrus {

using ObjectID = uint64_t;

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level.
  */
class BladeClient {
 public:
    virtual void connect(const std::string& address,
                         const std::string& port) = 0;

    virtual bool write_sync(ObjectID id, const void* data, uint64_t size) = 0;

    virtual bool read_sync(ObjectID id, void* data, uint64_t size) = 0;

    virtual bool remove(ObjectID id) = 0;

    virtual ClientFuture write_async(ObjectID oid, const void* data,
        uint64_t size) = 0;

    virtual ClientFuture read_async(ObjectID oid, void* data,
        uint64_t size) = 0;

    class ClientFuture {
     public:
        ClientFuture(std::shared_ptr<bool> result,
               std::shared_ptr<bool> result_available,
               std::shared_ptr<cirrus::PosixSemaphore> sem,
               std::shared_ptr<cirrus::ErrorCodes> error_code);

        void wait();

        bool try_wait();

        bool get();
     private:
        /** Pointer to the result. */
        std::shared_ptr<bool> result;

        /** Boolean monitoring result state. */
        std::shared_ptr<bool> result_available;

        /** Sem for the result. */
        std::shared_ptr<cirrus::PosixSemaphore> sem;

        /** Any errors thrown. */
        std::shared_ptr<cirrus::ErrorCodes> error_code;
    };
};

}  // namespace cirrus

#endif  // SRC_CLIENT_BLADECLIENT_H_
