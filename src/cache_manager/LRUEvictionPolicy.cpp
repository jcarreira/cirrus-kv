#include "cache_manager/LRUEvictionPolicy.h"
#include <stdint.h>
#include <vector>
#include <algorithm>

#include "common/Exception.h"

namespace cirrus {

/**
 * Constructor for the eviction policy.
 */
LRUEvictionPolicy::LRUEvictionPolicy(uint64_t max_size) :
    max_size(max_size) {}


/**
 * Get method for the eviction policy. If the cache is at capacity when get
 * is called, the oldest item is returned to be removed from the cache.
 */
std::vector<ObjectID> LRUEvictionPolicy::get(ObjectID oid) {
    return process_oid(oid);
}

/**
 * Put method for the eviction policy. Returns empty list as a put will never
 * require additional cache space.
 */
std::vector<ObjectID> LRUEvictionPolicy::put(ObjectID /* oid */) {
    return std::vector<ObjectID>();
}

/**
 * Prefetch method for the eviction policy. If the cache is at capacity when get
 * is called, the oldest item is returned to be removed from the cache.
 */
std::vector<ObjectID> LRUEvictionPolicy::prefetch(ObjectID oid) {
    return process_oid(oid);
}

/**
 * Remove method for the eviction policy. Simply records internally that the
 * object is no longer in the cache.
 */
void LRUEvictionPolicy::remove(ObjectID oid) {
    auto it = object_map.find(oid);
    if (it != object_map.end()) {
        // remove item from set
        object_list.erase(object_map[oid]);
        object_map.erase(it);
    }
}
/**
 * Method that processes an item being added to the cache. If the cache
 * is at capacity when it is called, the least recently used item is removed.
 */
std::vector<ObjectID> LRUEvictionPolicy::process_oid(ObjectID oid) {
    auto it = object_map.find(oid);
    if (it == object_map.end()) {
        object_map[oid] = object_list.insert(object_list.begin(), oid);
        if (object_map.size() > max_size) {
            // remove oldest from deque
            ObjectID to_remove = object_list.back();
            object_list.pop_back();

            auto map_iterator = object_map.find(to_remove);
            object_map.erase(map_iterator);

            std::vector<ObjectID> return_vec = {to_remove};
            return return_vec;
        } else {
            return std::vector<ObjectID>();
        }
    } else {
        // Move an object back to the front if it has been accessed.
        object_list.splice(object_list.begin(), object_list, it->second);
        return std::vector<ObjectID>();
    }
}
}  // namespace cirrus
