#ifndef SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_
#define SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_

#include <vector>
#include <queue>
#include <set>
#include "cache_manager/EvictionPolicy.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
 * A class that inherits from EvictionPolicy. When it needs to evict an item,
 * evicts the single oldest item.
 */
class LRAddedEvictionPolicy : public EvictionPolicy {
 public:
    explicit EvictionPolicy(size_t max_num_objs) override;

    std::vector<ObjectID> get(ObjectID oid) override;

    std::vector<ObjectID> put(ObjectID oid, T obj) override;

    std::vector<ObjectID> prefetch(ObjectID oid) override;

 private:
     /**
       * The maximum capacity of the cache. Will never be exceeded. Set
       * at time of instantiation.
       */
     uint64_t max_size;

     /**
       * Queue to store the order that objects were inserted.
       */
     std::queue<ObjectID> object_queue;

     /**
      * Set to keep track of which objects are present.
      */
     std::set<ObjectID> object_set;
};

}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_
