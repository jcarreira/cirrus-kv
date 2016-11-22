/* Copyright 2016 Joao Carreira */

#ifndef _EVICTION_POLICY_H_
#define _EVICTION_POLICY_H_

#include "src/object_store/FullCacheStore.h"

#include <cstddef>

namespace sirius {

class EvictionPolicy {
public:
    explicit EvictionPolicy(size_t max_num_objs);
    virtual ~EvictionPolicy() = default;

    virtual bool evictIfNeeded(FullCacheStore& fc) = 0;

protected:
    size_t max_num_objs_;
};

} // namespace sirius

#endif
