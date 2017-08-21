#ifndef SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_
#define SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_

#include <stdint.h>
#include <unordered_map>
#include <list>
#include <vector>

#include "cache_manager/EvictionPolicy.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
 * A class that inherits from EvictionPolicy. When it needs to evict an item,
 * evicts least recently used item.
 */
class LRUEvictionPolicy : public EvictionPolicy {
 public:
    explicit LRUEvictionPolicy(uint64_t max_num_objs);

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
      * Unordered map to track iterators to inside of list.
      */
     std::unordered_map<ObjectID, std::list<ObjectID>::iterator> object_map;

     /**
      * List to keep track of ordering.
      */
     std::list<ObjectID> object_list;
};

}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_
