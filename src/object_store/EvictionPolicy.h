#ifndef _EVICTION_POLICY_H_
#define _EVICTION_POLICY_H_

#include "object_store/FullCacheStore.h"

#include <cstddef>

namespace cirrus {

class EvictionPolicy {
public:
    explicit EvictionPolicy(size_t max_num_objs);
    virtual ~EvictionPolicy() = default;

    virtual bool evictIfNeeded(FullCacheStore& fc) = 0;

protected:
    size_t max_num_objs_; /** Max number of objects in the store */
};

} // namespace cirrus

#endif
