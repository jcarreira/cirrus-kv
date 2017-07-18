#include <iostream>
#include <string>
#include "object_store/RDMAObjectStore.h"
#include "object_store/RandomEvictionPolicy.h"
#include "utils/utils.h"

namespace cirrus {
    
template<class T>
RDMAObjectStore<T>::RDMAObjectStore(const std::string& blade_addr,
        const std::string& port,
        EvictionPolicy* ev = new RandomEvictionPolicy()) :
    ObjectStore(),
    ep(ev) {
        client.connect(blade_addr, port);
}

template<class T>
Object RDMAObjectStore<T>::get(ObjectID name) {
    Object obj = cache_.get(name);
    if (obj) {
        return obj;
    } else {
        std::cout << "Object not found. name: " << name << std::endl;
        DIE("DIE");
        return 0;
    }
}

bool RDMAObjectStore::put(Object obj, uint64_t size, ObjectID name) {
    ep->evictIfNeeded(cache_);
    return cache_.put(obj, size, name);
}

template<class T>
typename RDMAObjectStore<T>::ObjectStoreGetFuture ObjectStore<T>::get_async(
    const ObjectID& id) {
    throw std::runtime_error("Function not implemented.");
}

template<class T>
typename RDMAObjectStore<T>::ObjectStorePutFuture ObjectStore<T>::put_async(
    const ObjectID& id, const T& obj) {
    throw std::runtime_error("Function not implemented.");
}

template<class T>
void RDMAObjectStore<T>::put_bulk(ObjectID /* start */,
    ObjectID /* last */, T* /* data */) {
    throw std::runtime_error("Function not implemented.");
}

template<class T>
void RDMAObjectStore<T>::get_bulk(ObjectID /* start */,
    ObjectID /* last */, T* /* data */) {
    throw std::runtime_error("Function not implemented.");
}


void RDMAObjectStore::printStats() {
    std::cout << "ObjectStore stats" << std::endl;
    std::cout << "Cache stats:" << std::endl;
    cache_.printStats();
}

}  //  namespace cirrus

