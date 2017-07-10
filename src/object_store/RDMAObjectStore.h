#ifndef SRC_OBJECT_STORE_RDMAOBJECTSTORE_H_
#define SRC_OBJECT_STORE_RDMAOBJECTSTORE_H_

#include <string>

#include "object_store/ObjectStore.h"
#include "object_store/FullCacheStore.h"
#include "object_store/EvictionPolicy.h"
#include "client/BladeClient.h"

namespace cirrus {

class RDMAObjectStore : public ObjectStore {
 public:
    RDMAObjectStore(const std::string& blade_addr,
        const std::string& port, EvictionPolicy* ev);
    virtual ~RDMAObjectStore() = default;

    Object get(ObjectID);
    bool put(Object, uint64_t, ObjectID);
    virtual void printStats();

 private:
    std::unique_ptr<EvictionPolicy> ep;
    // cache of objects
    mutable FullCacheStore cache_;
    BladeClient client;
};

}  // namespace cirrus

#endif  // SRC_OBJECT_STORE_RDMAOBJECTSTORE_H_
