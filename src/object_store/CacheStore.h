/* Copyright 2016 Joao Carreira */

#ifndef _CACHE_STORE_H_
#define _CACHE_STORE_H_

#include "src/object_store/ObjectStore.h"
#include <google/dense_hash_map>

namespace sirius {

class CacheStore : public ObjectStore {
public:
    CacheStore();

    Object get(ObjectName);
    bool put(Object, ObjectName);

private:
    google::dense_hash_map<ObjectName, Object> objects_;
};
}

#endif // _CACHE_STORE_H_

