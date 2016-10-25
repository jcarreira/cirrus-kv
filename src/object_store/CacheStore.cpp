/* Copyright 2016 Joao Carreira */

#include "src/object_store/CacheStore.h"

namespace sirius {

CacheStore::CacheStore() :
    ObjectStore() {
    objects_.set_empty_key(ObjectName());
}
    
Object CacheStore::get(ObjectName name) {
    google::dense_hash_map<ObjectName, Object>::iterator it;
    it = objects_.find(name);
    if (it == objects_.end()) {
        return 0;
    } else {
        return it->second;
    }
}

bool CacheStore::put(Object obj, ObjectName name) {
    objects_[name] = obj;
    return true;
}

} //  namespace sirius
