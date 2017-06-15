#ifndef _LRU_EVICTION_POLICY_H_
#define _LRU_EVICTION_POLICY_H_

#include "object_store/EvictionPolicy.h"

namespace cirrus {

class LRUEvictionPolicy : public EvictionPolicy {
public:
    explicit LRUEvictionPolicy(size_t num_objs = DEFAULT_SIZE);
    virtual ~LRUEvictionPolicy();

    virtual bool evictIfNeeded(FullCacheStore& fc);

private:
    static const size_t DEFAULT_SIZE = 1000;
};

} // namespace cirrus

#endif // _LRU_EVICTION_POLICY_H_
