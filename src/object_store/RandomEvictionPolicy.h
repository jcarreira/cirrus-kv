#ifndef SRC_OBJECT_STORE_RANDOMEVICTIONPOLICY_H_
#define SRC_OBJECT_STORE_RANDOMEVICTIONPOLICY_H_

#include "object_store/EvictionPolicy.h"

namespace cirrus {

class RandomEvictionPolicy : public EvictionPolicy {
 public:
    explicit RandomEvictionPolicy(size_t num_objs = DEFAULT_SIZE);
    virtual ~RandomEvictionPolicy() = default;

    virtual bool evictIfNeeded(FullCacheStore *fc);

 private:
    static const size_t DEFAULT_SIZE = 1000;
};

}  // namespace cirrus

#endif  // SRC_OBJECT_STORE_RANDOMEVICTIONPOLICY_H_
