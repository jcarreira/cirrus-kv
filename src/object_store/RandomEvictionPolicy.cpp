#include "object_store/RandomEvictionPolicy.h"
#include "object_store/FullCacheStore.h"

namespace cirrus {

RandomEvictionPolicy::RandomEvictionPolicy(size_t num_objs) :
    EvictionPolicy(num_objs) {
}

bool RandomEvictionPolicy::evictIfNeeded(FullCacheStore& fc) {
    if (fc.getNumObjs() > max_num_objs_) {
        fc.dropRandomObj();
        return true;
    }
    return false;
}

}  // namespace cirrus
