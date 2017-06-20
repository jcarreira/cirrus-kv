#include <iostream>
#include <string>
#include "object_store/RDMAObjectStore.h"
#include "object_store/RandomEvictionPolicy.h"
#include "utils/utils.h"

namespace cirrus {

RDMAObjectStore::RDMAObjectStore(const std::string& blade_addr,
        const std::string& port,
        EvictionPolicy* ev = new RandomEvictionPolicy()) :
    ObjectStore(),
    ep(ev) {
        client.connect(blade_addr, port);
}

Object RDMAObjectStore::get(ObjectID name) {
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

void RDMAObjectStore::printStats() {
    std::cout << "ObjectStore stats" << std::endl;
    std::cout << "Cache stats:" << std::endl;
    cache_.printStats();
}

}  //  namespace cirrus

