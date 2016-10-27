/* Copyright 2016 Joao Carreira */

#include <iostream>
#include "src/object_store/RDMAObjectStore.h"
#include "src/utils/utils.h"
//#include "src/utils/easylogging++.h"

namespace sirius {

RDMAObjectStore::RDMAObjectStore() :
    ObjectStore() {

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
    return cache_.put(obj, size, name);
}

void RDMAObjectStore::printStats() {
    std::cout << "ObjectStore stats" << std::endl;
    std::cout << "Cache stats:" << std::endl;
    cache_.printStats();
}

} //  namespace sirius

