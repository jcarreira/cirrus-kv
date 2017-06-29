#ifndef SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_
#define SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_

#include "cache_manager/LRAddedEvictionPolicy.h"
#include <stdint.h>
#include <vector>
#include <queue>
#include <set>

namespace cirrus {

/**
 * Constructor for the eviction policy.
 */
LRAddedEvictionPolicy::LRAddedEvictionPolicy(uint64_t max_size) :
    max_size(max_size) {}


/**
 * Get method for the eviction policy. If the cache is at capacity when get
 * is called, the oldest item is returned to be removed from the cache.
 */
std::vector<ObjectID> LRAddedEvictionPolicy::get(ObjectID oid) {
    return process_oid(oid);
}

/**
 * Put method for the eviction policy. Returns empty list as a put will never
 * require additional cache space.
 */
std::vector<ObjectID> LRAddedEvictionPolicy::put(ObjectID /* oid */) {
    return std::vector<ObjectID>();
}

/**
 * Prefetch method for the eviction policy. If the cache is at capacity when get
 * is called, the oldest item is returned to be removed from the cache.
 */
std::vector<ObjectID> LRAddedEvictionPolicy::prefetch(ObjectID oid) {
    return process_oid(oid);
}

/**
 * Method that processes an item being added to the cache. If the cache
 * is at capacity when it is called, the oldest item is returned to be
 * removed from the cache.
 */
std::vector<ObjectID> LRAddedEvictionPolicy::process_oid(ObjectID oid) {
    auto it = object_set.find(oid);
    if (it == object_set.end()) {
        object_set.insert(oid);
        object_queue.push(oid);
        if (object_queue.size() > max_size) {
            ObjectID to_remove = object_queue.front();
            object_queue.pop();
            std::vector<ObjectID> return_vec = {to_remove};
            return return_vec;
        } else {
            return std::vector<ObjectID>();
        }
    } else {
        return std::vector<ObjectID>();
    }
}
}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_LRUEVICTIONPOLICY_H_
