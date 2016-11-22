/* Copyright 2016 Joao Carreira */

#ifndef _RDMA_OBJECT_STORE_H_
#define _RDMA_OBJECT_STORE_H_

#include "src/object_store/ObjectStore.h"
#include "src/object_store/FullCacheStore.h"
#include "src/object_store/EvictionPolicy.h"
#include "src/client/BladeClient.h"

namespace sirius {

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

}

#endif // _RDMA_OBJECT_STORE_H_

