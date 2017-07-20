#ifndef SRC_OBJECT_STORE_RDMAOBJECTSTORE_H_
#define SRC_OBJECT_STORE_RDMAOBJECTSTORE_H_

#include <string>

#include "object_store/ObjectStore.h"
#include "object_store/FullCacheStore.h"
#include "object_store/EvictionPolicy.h"
#include "client/BladeClient.h"

namespace cirrus {

template<class T>
class RDMAObjectStore : public ObjectStore<T> {
 public:
    RDMAObjectStore(const std::string& blade_addr,
        const std::string& port, EvictionPolicy* ev);
    virtual ~RDMAObjectStore() = default;

    Object get(ObjectID) override;
    bool put(Object, uint64_t, ObjectID) override;
    ObjectStoreGetFuture get_async(const ObjectID& id) override;
    ObjectStorePutFuture put_async(const ObjectID& id, const T& obj) override;

    void get_bulk(ObjectID start, ObjectID last, T* data) override;
    void put_bulk(ObjectID start, ObjectID last, T* data) override;
    virtual void printStats() override;

 private:
    std::unique_ptr<EvictionPolicy> ep;
    // cache of objects
    mutable FullCacheStore cache_;
    BladeClient client;
};

}  // namespace cirrus

#endif  // SRC_OBJECT_STORE_RDMAOBJECTSTORE_H_
