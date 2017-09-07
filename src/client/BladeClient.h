#ifndef SRC_CLIENT_BLADECLIENT_H_
#define SRC_CLIENT_BLADECLIENT_H_

#include <string>
#include <memory>
#include <utility>
#include <vector>

#include "common/Serializer.h"
#include "common/Synchronization.h"
#include "common/Exception.h"
#include "utils/Log.h"

namespace cirrus {

using ObjectID = uint64_t;

struct FutureData {
    FutureData(
            bool result = false,
            bool result_available = false,
            cirrus::ErrorCodes error_code = cirrus::ErrorCodes(),
            std::shared_ptr<const char> data_ptr = nullptr,
            uint64_t data_size = 0);

     /** Pointer to the result. */
     bool result;
     /** Boolean monitoring result state. */
     bool result_available;
     /** lock for the result. */
     std::shared_ptr<cirrus::Lock> sem;
     /** Any errors thrown. */
     cirrus::ErrorCodes error_code;
     /** Pointer to any mem for a read, if any. */
     std::shared_ptr<const char> data_ptr;
     /** Size of the memory block for a read. */
     uint64_t data_size;
};

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level.
  */
class BladeClient {
 public:
    class ClientFuture {
     public:
        explicit ClientFuture(std::shared_ptr<FutureData> fd);
        ClientFuture() = default;
        void wait();

        bool try_wait();

        bool get();

        std::pair<std::shared_ptr<const char>, unsigned int> getDataPair();

     private:
         std::shared_ptr<FutureData> fd;
    };

    virtual ~BladeClient() = default;

    virtual void connect(const std::string& address,
                         const std::string& port) = 0;
    virtual bool write_sync(ObjectID id,  const WriteUnit& w) = 0;

    virtual std::pair<std::shared_ptr<const char>, unsigned int> read_sync(
        ObjectID id) = 0;
    virtual std::pair<std::shared_ptr<const char>, unsigned int> read_sync_bulk(
        const std::vector<ObjectID>& ids) = 0;

    virtual bool remove(ObjectID id) = 0;

    virtual BladeClient::ClientFuture write_async(ObjectID oid,
        const WriteUnit& w) = 0;

    virtual BladeClient::ClientFuture read_async(ObjectID oid) = 0;
    virtual BladeClient::ClientFuture read_async_bulk(
                                                std::vector<ObjectID> oids) = 0;
};

}  // namespace cirrus

#endif  // SRC_CLIENT_BLADECLIENT_H_
