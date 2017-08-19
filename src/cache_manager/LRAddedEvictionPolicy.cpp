#include "cache_manager/LRAddedEvictionPolicy.h"
#include <stdint.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <set>

#include "common/Exception.h"

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
 * Remove method for the eviction policy. Simply records internally that the
 * object is no longer in the cache.
 */
void LRAddedEvictionPolicy::remove(ObjectID oid) {
    auto it = object_set.find(oid);
    if (it != object_set.end()) {
        // remove item from set
        object_set.erase(it);
        // remove item from deque
        // This is a linear search and is thus O(n), which will severely impact
        // performance when the store is very full.
        auto deque_iterator = std::find(object_deque.begin(),
                                    object_deque.end(),
                                    oid);
        object_deque.erase(deque_iterator);
    }
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
        object_deque.push_back(oid);
        if (object_deque.size() > max_size) {
            // remove oldest from deque
            ObjectID to_remove = object_deque.front();

            // remove item from deque
            object_deque.pop_front();
            // remove oldest from set
            auto set_iterator = object_set.find(to_remove);
            object_set.erase(set_iterator);

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
