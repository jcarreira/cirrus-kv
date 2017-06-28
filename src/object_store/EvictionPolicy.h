#ifndef SRC_OBJECT_STORE_EVICTIONPOLICY_H_
#define SRC_OBJECT_STORE_EVICTIONPOLICY_H_

#include <cstddef>

#include "object_store/FullCacheStore.h"


namespace cirrus {

class EvictionPolicy {
 public:
    explicit EvictionPolicy(size_t max_num_objs);
    virtual ~EvictionPolicy() = default;

    virtual bool evictIfNeeded(FullCacheStore *fc) = 0;

 protected:
    size_t max_num_objs_; /** Max number of objects in the store */
};

}  // namespace cirrus

#endif  // SRC_OBJECT_STORE_EVICTIONPOLICY_H_
