#ifndef SRC_CACHE_MANAGER_CACHEMANAGER_H_
#define SRC_CACHE_MANAGER_CACHEMANAGER_H_

#include <map>
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>
#include "cache_manager/EvictionPolicy.h"
#include "cache_manager/PrefetchPolicy.h"
#include "object_store/FullBladeObjectStore.h"
#include "common/Exception.h"
#include "utils/logging.h"

namespace cirrus {
using ObjectID = uint64_t;

  /**
    * A class that manages the cache and interfaces with the store.
    */
template<class T>
class CacheManager {
 public:
    /**
     * This enum defines different prefetch modes for the cache manager.
     * The default is no prefetching.
     */
    enum PrefetchMode {
        kNone = 0, /**< The cache manager will not automatically prefetch. */
        kOrdered, /**< The cache manager will prefetch a few items ahead. */
        kCustom /**< The cache will prefetch in a user defined fashion. */
    };
    CacheManager(cirrus::ostore::FullBladeObjectStoreTempl<T> *store,
                 cirrus::EvictionPolicy *eviction_policy,
                 uint64_t cache_size);
    T get(ObjectID oid);
    void put(ObjectID oid, T obj);
    void prefetch(ObjectID oid);
    void remove(ObjectID oid);
    void setMode(PrefetchMode mode,
            cirrus::PrefetchPolicy<T> *policy = nullptr);
    void setMode(PrefetchMode mode, ObjectID first, ObjectID last,
            uint64_t read_ahead);

 private:
    /**
     * A prefetch policy that does not perform any prefetching.
     */
    class OffPolicy :public cirrus::PrefetchPolicy<T> {
     public:
        /**
         * Return an empty vector to indicate no prefetching.
         */
        std::vector<ObjectID> get(const ObjectID& id, const T& obj) override {
            return std::vector<ObjectID>();
        }
    };

    /**
     * A prefetch policy that fetches the next k items when one is fetched.
     * Store cannot be modified while this policy is in use.
     */
    class OrderedPolicy :public cirrus::PrefetchPolicy<T> {
     public:
        std::vector<ObjectID> get(const ObjectID& id,
            const T& /* obj */) override {
            if (id < first || id > last) {
                throw cirrus::Exception("Attempting to get id outside of "
                        "continuous range present at time of prefetch  mode "
                        "specification.");
            }
            std::vector<ObjectID> to_return;
            to_return.reserve(read_ahead);
            for (int i = 1; i <= read_ahead; i++) {
                // Math to make sure that prefetching loops back around
                // Formula is:
                // val = ((oid + i) - first) % (last - first + 1)) + first
                ObjectID tentative_fetch = id + i;
                ObjectID shifted = tentative_fetch - first;
                ObjectID modded = shifted % (last - first + 1);
                to_return.push_back(modded + first);
            }
            return to_return;
        }
        /**
         * Sets the range that this policy will use.
         * @param first_ first objectID in a continuous range
         * @param last_ last objectID that will be used
         */
        void setRange(ObjectID first_, ObjectID last_) {
            first = first_;
            last = last_;
        }

        /**
         * Sets the readahead for this prefetch policy.
         * @param read_ahead_ how many items ahead the cache should prefetch.
         */
        void setReadAhead(uint64_t read_ahead_) {
            read_ahead = read_ahead_;
        }

     private:
        /** How many objects to prefetch ahead. */
        uint64_t read_ahead = 5;
        /** First continuous oid. */
        ObjectID first;
        /** Last continuous oid. */
        ObjectID last;
    };

    void evict_vector(const std::vector<ObjectID>& to_remove);
    void evict(ObjectID oid);

    /**
      * A pointer to a store that contains the same type of object as the
      * cache. This is the store that the cache manager interfaces with in order
      * to access the remote store.
      */
    cirrus::ostore::FullBladeObjectStoreTempl<T> *store;

    /**
      * Struct that is stored within the cache. Contains a copy of an object
      * of the type that the cache is storing.
      */
    struct cache_entry {
      T obj; /**< Object that will be retrieved by a get() operation. */
    };

    /**
      * The map that serves as the actual cache. Maps ObjectIDs to cache
      * entries.
      */
    std::map<ObjectID, struct cache_entry> cache;

    /**
      * The maximum capacity of the cache. Will never be exceeded. Set
      * at time of instantiation.
      */
    uint64_t max_size;

    /**
     * EvictionPolicy used for all calls to the eviction policy. Call made
     * before each operation that the cache manager makes.
     */
    cirrus::EvictionPolicy *eviction_policy;

    /**
     * PrefetchPolicy used to determine prefetch operations.
     */
    cirrus::PrefetchPolicy<T> *prefetch_policy;
    // Policies to use if specified
    /** An instance of an off policy. */
    OffPolicy off_policy;
    /** An instance of an ordered policy. */
    OrderedPolicy ordered_policy;
};


/**
  * Constructor for the CacheManager class. Any object added to the cache
  * needs to have a default constructor. Prefetching is off by default.
  * @param store a pointer to the ObjectStore that the CacheManager will
  * interact with. This is where all objects will be stored and retrieved
  * from.
  * @param cache_size the maximum number of objects that the cache should
  * hold. Must be at least one.
  */
template<class T>
CacheManager<T>::CacheManager(
                           cirrus::ostore::FullBladeObjectStoreTempl<T> *store,
                           cirrus::EvictionPolicy *eviction_policy,
                           uint64_t cache_size) :
                           store(store), eviction_policy(eviction_policy),
                           max_size(cache_size) {
    if (cache_size < 1) {
        throw cirrus::CacheCapacityException(
              "Cache capacity must be at least one.");
    }
    prefetch_policy = &off_policy;
}


/**
  * A function that returns an object stored under a given ObjectID.
  * If no object is stored under that ObjectID, throws a cirrus::Exception.
  * @param oid the ObjectID that the object you wish to retrieve is stored
  * under.
  * @return a copy of the object stored on the server.
  */
template<class T>
T CacheManager<T>::get(ObjectID oid) {
    std::vector<ObjectID> to_remove = eviction_policy->get(oid);
    evict_vector(to_remove);
    // check if entry exists for the oid in cache
    // if entry exists, return if it is there, otherwise wait
    // return pointer
    struct cache_entry *entry;
    if (cache.find(oid) != cache.end()) {
        entry = &(cache.find(oid)->second);
    } else {
        // set up entry, pull synchronously
        // Do we save to the cache in this case?
        if (cache.size() == max_size) {
          throw cirrus::CacheCapacityException("Get operation would put cache "
                                             "over capacity.");
        }
        entry = &cache[oid];
        entry->obj = store->get(oid);
    }

    std::vector<ObjectID> to_prefetch = prefetch_policy->get(oid, entry->obj);
    for (auto const& id_to_prefetch : to_prefetch) {
        prefetch(id_to_prefetch);
    }
    return entry->obj;
}

/**
  * A function that stores an object obj in the remote store under ObjectID oid.
  * Calls the put() method in FullBladeObjectstore.h
  * @param oid the ObjectID to store under.
  * @param obj the object to store on the server
  * @see FullBladeObjectstore
  */
template<class T>
void CacheManager<T>::put(ObjectID oid, T obj) {
    std::vector<ObjectID> to_remove = eviction_policy->put(oid);
    evict_vector(to_remove);
    // Push the object to the store under the given id
    store->put(oid, obj);
}

/**
  * A function that fetches an object from the remote server and stores it in
  * the cache, but does not return the object. Should be called in advance of
  * the object being used in order to reduce latency. If object is already
  * in the cache, no call will be made to the remote server.
  * @param oid the ObjectID that the object you wish to retrieve is stored
  * under.
  */
template<class T>
void CacheManager<T>::prefetch(ObjectID oid) {
    std::vector<ObjectID> to_remove = eviction_policy->prefetch(oid);
    evict_vector(to_remove);
    LOG<INFO>("Prefetching oid: ", oid);
    if (cache.find(oid) == cache.end()) {
      struct cache_entry& entry = cache[oid];
      entry.obj = store->get(oid);
    }
}

/**
 * Removes the cache entry corresponding to oid from the cache.
 * Throws an error if it is not present.
 * @param oid the ObjectID corresponding to the object to remove.
 */
template<class T>
void CacheManager<T>::remove(ObjectID oid) {
    // Call remove on store first. Exception will be thrown if oid nonexistent.
    store->remove(oid);
    auto it = cache.find(oid);
    if (it != cache.end()) {
        cache.erase(it);
    }
    eviction_policy->remove(oid);
}

/**
 * Removes the cache entry corresponding to oid from the cache.
 * Throws an error if it is not present.
 * @param oid the ObjectID corresponding to the object to remove.
 */
template<class T>
void CacheManager<T>::evict(ObjectID oid) {
    auto it = cache.find(oid);
    if (it == cache.end()) {
        throw cirrus::Exception("Attempted to remove item not in cache.");
    } else {
        cache.erase(it);
    }
}

/**
 * Removes the cache entry corresponding to each oid in the vector passed in.
 * @param to_remove an std::vector of ids corresponding to the objects
 * to remove.
 */
template<class T>
void CacheManager<T>::evict_vector(const std::vector<ObjectID>& to_remove) {
    for (auto const& oid : to_remove) {
        evict(oid);
    }
}

/**
 * Sets the prefetching mode for the cache. The default is no prefetching.
 * Note: Ordered prefetching can only be used if all objectIDs are sequential.
 * Using ordered prefetching when IDs are not sequential will result in errors.
 * Note: Cache/Store contents should not be modified (no put or remove) while
 * the ordered iterator is in use as this could cause issues. Disable the
 * iterator before making changes.
 */
template<class T>
void CacheManager<T>::setMode(CacheManager::PrefetchMode mode,
    cirrus::PrefetchPolicy<T> *policy) {
    // Set the mode
    switch (mode) {
      case CacheManager::PrefetchMode::kNone: {
        // Set policy to the off policy
        prefetch_policy = &off_policy;
        break;
      }
      case CacheManager::PrefetchMode::kOrdered: {
        throw cirrus::Exception("Ordered prefetching "
                "specified without a range and read ahead.");
      }
      case CacheManager::PrefetchMode::kCustom: {
        if (policy == nullptr) {
            throw cirrus::Exception("Custom mode specified without a policy.");
        }
        prefetch_policy = policy;
        break;
      }
      default: {
        throw cirrus::Exception("Unrecognized prefetch mode during setMode().");
      }
    }
}

/**
 * Variant of setMode that is only used for switching to Ordered mode.
 * @param mode the mode to switch to. This should be kOrdered.
 * @param first the first continuous objectID in a range that prefetching will
 * loop over.
 * @param last the last continuous objectID in the above range.
 * @param read_ahead how many items ahead the cache should prefetch.
 */
template<class T>
void CacheManager<T>::setMode(CacheManager::PrefetchMode mode,
    ObjectID first, ObjectID last, uint64_t read_ahead) {
    switch (mode) {
      case CacheManager::PrefetchMode::kOrdered: {
        if (first > last) {
            throw cirrus::Exception("Last oid must be >= first");
        }
        ordered_policy.setRange(first, last);
        ordered_policy.setReadAhead(read_ahead);
        prefetch_policy = &ordered_policy;
        break;
      }
      default: {
        throw cirrus::Exception("First/last/read_ahead arguments passed for "
                "nonordered policy.");
      }
    }
}
}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_CACHEMANAGER_H_
