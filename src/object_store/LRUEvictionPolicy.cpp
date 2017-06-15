#include "object_store/LRUEvictionPolicy.h"
#include "object_store/FullCacheStore.h"

namespace cirrus {

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

}  // namespace cirrus
