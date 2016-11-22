/* Copyright 2016 Joao Carreira */

#ifndef _RANDOM_EVICTION_POLICY_H_
#define _RANDOM_EVICTION_POLICY_H_

#include "src/object_store/EvictionPolicy.h"

namespace sirius {

class RandomEvictionPolicy : public EvictionPolicy {
public:
    explicit RandomEvictionPolicy(size_t num_objs = DEFAULT_SIZE);
    virtual ~RandomEvictionPolicy() = default;

    virtual bool evictIfNeeded(FullCacheStore& fc);

private:
    static const size_t DEFAULT_SIZE = 1000;
};

} // namespace sirius

#endif // _RANDOM_EVICTION_POLICY_H_
