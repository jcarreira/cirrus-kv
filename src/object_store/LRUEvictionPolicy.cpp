/* Copyright Joao Carreira 2016 */

#include "src/object_store/LRUEvictionPolicy.h"
#include "src/object_store/FullCacheStore.h"

namespace sirius {

LRUEvictionPolicy::LRUEvictionPolicy(size_t num_objs) :
    EvictionPolicy(num_objs) {
}

LRUEvictionPolicy::~LRUEvictionPolicy() {
}

bool LRUEvictionPolicy::evictIfNeeded(FullCacheStore& fc) {
    if (fc.getNumObjs() > max_num_objs_) {
        fc.dropLRUObj();
        return true;
    }
    return false;
}

}  // namespace sirius
