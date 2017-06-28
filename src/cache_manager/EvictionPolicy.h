#ifndef SRC_CACHE_MANAGER_EVICTIONPOLICY_H_
#define SRC_CACHE_MANAGER_EVICTIONPOLICY_H_

#include <vector>

namespace cirrus {

using ObjectID = uint64_t;

/**
 * A class that defines the interface for eviction policies. The methods
 * inside the class are called when their counterparts are called in the
 * cache manager. Each method returns a list of ObjectIDs to be removed.
 * If these IDs are not in the cache, an error will be thrown.
 */
class EvictionPolicy {
 public:
    EvictionPolicy() {}
    /**
     * Counterpart to the get method. Returns a list of the ObjectIDs to be
     * purged from the cache.
     */
    virtual std::vector<ObjectID> get(ObjectID oid) = 0;

    /**
     * Counterpart to the put method. Returns a vector of the ObjectIDs to be
     * purged from the cache.
     */
    virtual std::vector<ObjectID> put(ObjectID oid) = 0;

    /**
     * Counterpart to the prefetch method. Returns a vector of the ObjectIDs to be
     * purged from the cache.
     */
    virtual std::vector<ObjectID> prefetch(ObjectID oid) = 0;

 private:
     /**
       * The maximum capacity of the cache. Will never be exceeded. Set
       * at time of instantiation.
       */
     uint64_t max_size;
};

}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_EVICTIONPOLICY_H_
