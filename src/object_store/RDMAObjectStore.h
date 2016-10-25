/* Copyright 2016 Joao Carreira */

#ifndef _RDMA_OBJECT_STORE_H_
#define _RDMA_OBJECT_STORE_H_

#include "src/object_store/ObjectStore.h"
#include "src/object_store/CacheStore.h"

namespace sirius {

class RDMAObjectStore : public ObjectStore {
public:
    RDMAObjectStore();

    Object get(ObjectName);
    bool put(Object, ObjectName);

private:
    // cache of objects
    mutable CacheStore cache_;
};

}

#endif // _RDMA_OBJECT_STORE_H_

