#ifndef SRC_CACHE_MANAGER_PREFETCHPOLICY_H_
#define SRC_CACHE_MANAGER_PREFETCHPOLICY_H_

#include <vector>

namespace cirrus {

using ObjectID = uint64_t;

/**
 * A class that defines the interface for prefetch policies. The get method
 * inside the class is called when its counterpart is called inside the
 * cache manager. A list of ObjectIDs to prefetch is then return
 * If these IDs are not in the cache, an error will be thrown.
 */
template<class T>
class EvictionPolicy {
 public:
    EvictionPolicy() = default;
    /**
     * Counterpart to the get method. Returns a list of the ObjectIDs to be
     * prefetched from the cache.
     * @param id the objectID that was obtained.
     * @param obj a const reference to the object retrieved, potentially
     * useful in determining the next objects to prefetch.
     * @return a vector of ObjectIDs to prefetch.
     */
    virtual std::vector<ObjectID> get(const ObjectID& id, const T& obj) = 0;
};

}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_PREFETCHPOLICY_H_
