#ifndef SRC_CACHE_MANAGER_LRADDEDEVICTIONPOLICY_H_
#define SRC_CACHE_MANAGER_LRADDEDEVICTIONPOLICY_H_

#include <stdint.h>
#include <vector>
#include <deque>
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
    explicit LRAddedEvictionPolicy(uint64_t max_num_objs);

    std::vector<ObjectID> get(ObjectID oid) override;

    std::vector<ObjectID> put(ObjectID oid) override;

    std::vector<ObjectID> prefetch(ObjectID oid) override;
    
    void remove(ObjectID oid) override;
 private:
     std::vector<ObjectID> process_oid(ObjectID oid);
   
     /**
      * The maximum capacity of the cache. Will never be exceeded. Set
      * at time of instantiation.
      */
     uint64_t max_size;

     /**
      * Queue to store the order that objects were inserted.
      */
     std::deque<ObjectID> object_deque;

     /**
      * Set to keep track of which objects are present.
      */
     std::set<ObjectID> object_set;
};

}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_LRADDEDEVICTIONPOLICY_H_
