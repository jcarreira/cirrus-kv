#ifndef SRC_CLIENT_BLADECLIENT_H_
#define SRC_CLIENT_BLADECLIENT_H_

#include <string>
#include <memory>
#include <utility>

#include "common/Synchronization.h"
#include "common/Exception.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level.
  */
class BladeClient {
 public:
    class ClientFuture {
     public:
        ClientFuture(std::shared_ptr<bool> result,
               std::shared_ptr<bool> result_available,
               std::shared_ptr<cirrus::Lock> sem,
               std::shared_ptr<cirrus::ErrorCodes> error_code,
               std::shared_ptr<std::shared_ptr<char>> data_ptr,
               std::shared_ptr<uint64_t> data_size);
        ClientFuture() {}
        void wait();

        bool try_wait();

        bool get();

        std::pair<std::shared_ptr<char>, unsigned int> getDataPair();

     private:
         /** Pointer to the result. */
         std::shared_ptr<bool> result;
         /** Boolean monitoring result state. */
         std::shared_ptr<bool> result_available;
         /** lock for the result. */
         std::shared_ptr<cirrus::Lock> sem;
          /** Any errors thrown. */
         std::shared_ptr<cirrus::ErrorCodes> error_code;
         /** Pointer to a pointer to any mem for a read, if any. */
         std::shared_ptr<std::shared_ptr<char>> data_ptr;
         /** Size of the memory block for a read. */
         std::shared_ptr<uint64_t> data_size;
    };

    virtual ~BladeClient() = default;

    virtual void connect(const std::string& address,
                         const std::string& port) = 0;

    virtual bool write_sync(ObjectID id, const void* data, uint64_t size) = 0;

    virtual std::pair<std::shared_ptr<char>, unsigned int> read_sync(
        ObjectID id) = 0;

    virtual bool remove(ObjectID id) = 0;

    virtual BladeClient::ClientFuture write_async(ObjectID oid,
        const void* data,
        uint64_t size) = 0;

    virtual BladeClient::ClientFuture read_async(ObjectID oid) = 0;
};

}  // namespace cirrus

#endif  // SRC_CLIENT_BLADECLIENT_H_
