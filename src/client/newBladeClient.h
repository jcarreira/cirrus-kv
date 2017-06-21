#ifndef SRC_CLIENT_BLADECLIENT_H_
#define SRC_CLIENT_BLADECLIENT_H_

#include <string>

namespace cirrus {

using ObjectID = uint64_t;

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level.
  */
class newBladeClient {
 public:
    virtual void connect(std::string address, std::string port) = 0;

    virtual bool write_sync(ObjectID id, void* data, uint64_t size) = 0;

    virtual bool read_sync(ObjectID id, void* data, uint64_t size) = 0;

    virtual bool remove(ObjectID id) = 0;

    // std::future<bool> write_sync(ObjectID id, void* data, uint64_t size);
    
    // std::future<bool> read_sync(ObjectID id, void* data, uint64_t size);
};

}  // namespace cirrus

#endif  // SRC_CLIENT_BLADECLIENT_H_