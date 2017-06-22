#ifndef SRC_CLIENT_BLADECLIENT_H_
#define SRC_CLIENT_BLADECLIENT_H_

#include <string>
#include "common/future.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level.
  */
class BladeClient {
 public:
    virtual void connect(std::string address, std::string port) = 0;

    virtual bool write_sync(ObjectID id, void* data, uint64_t size) = 0;

    virtual bool read_sync(ObjectID id, void* data, uint64_t size) = 0;

    virtual bool remove(ObjectID id) = 0;

    cirrus::Future write_async(ObjectID oid, void* data, uint64_t size);

    cirrus::Future read_async(ObjectID oid, void* data, uint64_t size);
};

}  // namespace cirrus

#endif  // SRC_CLIENT_BLADECLIENT_H_
