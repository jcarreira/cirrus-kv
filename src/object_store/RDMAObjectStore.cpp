/* Copyright 2016 Joao Carreira */

#include "src/object_store/RDMAObjectStore.h"
#include "src/utils/utils.h"

namespace sirius {

RDMAObjectStore::RDMAObjectStore() :
    ObjectStore() {

}

Object RDMAObjectStore::get(ObjectName name) {
    Object obj = cache_.get(name);
    if (obj) {
        return obj;
    } else {
        DIE("Object not found");
        return 0;
    }
}

bool RDMAObjectStore::put(Object obj, ObjectName name) {
    return cache_.put(obj, name);
}

} //  namespace sirius

