#ifndef SRC_CACHE_MANAGER_CACHEMANAGER_H_
#define SRC_CACHE_MANAGER_CACHEMANAGER_H_

#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>


#include "object_store/ObjectStore.h"
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
    void removeBulk(ObjectID first, ObjectID last);
    void prefetchBulk(ObjectID first, ObjectID last);
    void get_bulk(ObjectID start, ObjectID last, T* data);
    void put_bulk(ObjectID start, ObjectID last, T* data);
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
        std::vector<ObjectID> get(const ObjectID& /* id */,
                const T& /* obj */) override {
            return std::vector<ObjectID>();
        }
    };

    /**
     * A prefetch policy that fetches the next k items when one is fetched.
     * Store cannot be modified while this policy is in use.
     */
    class OrderedPolicy :public cirrus::PrefetchPolicy<T> {
     public:
        /**
         * Counterpart to the CacheManager's get method. Returns a list of items
         * to prefetch.
         * @param id the ObjectID being retrieved.
         * @param obj unused
         * @return a vector of ObjectIDs to be prefetched.
         */
        std::vector<ObjectID> get(const ObjectID& id,
            const T& /* obj */) override {
            if (id < first || id > last) {
                throw cirrus::Exception("Attempting to get id outside of "
                        "continuous range present at time of prefetch  mode "
                        "specification.");
            }
            std::vector<ObjectID> to_return;
            to_return.reserve(read_ahead);
            for (uint64_t i = 1; i <= read_ahead; i++) {
                // Math to make sure that prefetching loops back around
                // Formula is:
                // val = ((oid + i) - first) % (last - first + 1)) + first
                ObjectID tentative_fetch = id + i;
                ObjectID shifted = tentative_fetch - first;
                ObjectID modded = shifted % (last - first + 1);
                ObjectID to_prefetch = modded + first;
                // If attempting to prefetch the id that was just retrieved,
                // then a full circle has been completed, so no further
                // prefetching is necessary
                if (to_prefetch == id) {
                    break;
                }
                to_return.push_back(to_prefetch);
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
     * Struct that is stored within the cache. Contains a copy of an object
     * of the type that the cache is storing.
     */
    struct cache_entry {
        /** Boolean indicating whether this item is in the cache. */
        bool cached = true;
        /** Object that will be retrieved by a get() operation. */
        T obj;
        /** Future indicating status of operation */
        typename cirrus::ObjectStore<T>::ObjectStoreGetFuture future;
    };

    /**
     * A pointer to a store that contains the same type of object as the
     * cache. This is the store that the cache manager interfaces with in order
     * to access the remote store.
     */
    cirrus::ObjectStore<T> *store;

    /**
     * The map that serves as the actual cache. Maps ObjectIDs to cache
     * entries.
     */
    std::unordered_map<ObjectID, struct cache_entry> cache;

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
                           store(store),
                           max_size(cache_size),
                           eviction_policy(eviction_policy) {
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
    struct cache_entry *entry;

    LOG<INFO>("Cache get called on oid: ", oid);
    auto cache_iterator = cache.find(oid);
    if (cache_iterator != cache.end()) {
        LOG<INFO>("Entry exists for oid: ", oid);
        // entry exists for the oid
        // Call future's get method if the object is not cached
        entry = &(cache_iterator->second);
        if (!entry->cached) {
            LOG<INFO>("oid was prefetched");
            // TODO(Tyler): Should we return the result of the get directly
            // and avoid a potential extra copy? Tradeoff is copy now vs
            // copy in the future in case of repeated access.
            entry->obj = entry->future.get();
            entry->cached = true;
        }
    } else {
        // entry does not exist.
        // set up entry, pull synchronously
        // TODO(Tyler): Do we save to the cache in this case?
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
  * Calls the put() method in FullBladeObjectstore.h and stores cached copy
  * of object locally.
  * @param oid the ObjectID to store under.
  * @param obj the object to store on the server
  * @see FullBladeObjectstore
  */
template<class T>
void CacheManager<T>::put(ObjectID oid, T obj) {
    std::vector<ObjectID> to_remove = eviction_policy->put(oid);
    evict_vector(to_remove);
    // Push the object to the store under the given id
    // TODO(Tyler): Should we switch this to an async op for greater
    // performance potentially? what to do with the futures?
    store->put(oid, obj);

    struct cache_entry *entry;
    LOG<INFO>("Cache put called on oid: ", oid);
    auto cache_iterator = cache.find(oid);
    if (cache_iterator != cache.end()) {
        LOG<INFO>("Entry exists for oid: ", oid);
        // entry exists for the oid
        entry = &(cache_iterator->second);
        // replace existing entry
        entry->obj = obj;
    } else {
        // entry does not exist.
        // set up entry and fill it
        if (cache.size() == max_size) {
          throw cirrus::CacheCapacityException("Put operation would put cache "
                                             "over capacity.");
        }
        entry = &cache[oid];
        entry->obj = obj;
    }
}

/**
 * Gets many objects from the remote store at once. These items will be written
 * into the c style array pointed to by data.
 * @param start the first objectID that should be pulled from the store.
 * @param the last objectID that should be pulled from the store.
 * @param data a pointer to a c style array that will be filled from the
 * remote store.
 */
template<class T>
void CacheManager<T>::get_bulk(ObjectID start, ObjectID last, T* data) {
    if (last < start) {
        throw cirrus::Exception("Last objectID for get_bulk must be greater "
            "than start objectID.");
    }
    const int numObjects = last - start + 1;
    for (int i = 0; i < numObjects; i++) {
        data[i] = get(start + i);
    }
}

/**
 * Puts many objects to the remote store at once.
 * @param start the objectID that should be assigned to the first object
 * @param the objectID that should be assigned to the last object
 * @param data a pointer the first object in a c style array that will
 * be put to the remote store.
 */
template<class T>
void CacheManager<T>::put_bulk(ObjectID start, ObjectID last, T* data) {
    if (last < start) {
        throw cirrus::Exception("Last objectID for put_bulk must be greater "
            "than start objectID.");
    }
    const int numObjects = last - start + 1;
    for (int i = 0; i < numObjects; i++) {
        put(start + i, data[i]);
    }
}

/**
  * A function that fetches an object from the remote server and stores it in
  * the cache, but does not return the object. Should be called in advance of
  * the object being accessed in order to reduce latency. If object is already
  * in the cache, no call will be made to the remote server.
  * @param oid the ObjectID that the object you wish to retrieve is stored
  * under.
  */
template<class T>
void CacheManager<T>::prefetch(ObjectID oid) {
    std::vector<ObjectID> to_remove = eviction_policy->prefetch(oid);
    evict_vector(to_remove);
    LOG<INFO>("Prefetching oid: ", oid);
    // Check if it exists locally before calling the store method
    if (cache.find(oid) == cache.end()) {
        struct cache_entry& entry = cache[oid];
        entry.cached = false;
        entry.future = store->get_async(oid);
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
 * Prefetches a range of objects.
 * @param first the first ObjectID to prefetch
 * @param last the last ObjectID to prefetch
 */
template<class T>
void CacheManager<T>::prefetchBulk(ObjectID first, ObjectID last) {
    if (first > last) {
        throw cirrus::Exception("Last ObjectID to prefetch must be leq first.");
    }
    for (unsigned int oid = first; oid <= last; oid++) {
        prefetch(oid);
    }
}

/**
 * Removes a range of objects from the cache.
 * @param first the first continuous ObjectID to be removed from the cache
 * @param last the last ObjectID to be removed
 */
template<class T>
void CacheManager<T>::removeBulk(ObjectID first, ObjectID last) {
    if (first > last) {
        throw cirrus::Exception("Last ObjectID to remove must be leq first.");
    }
    for (unsigned int oid = first; oid <= last; oid++) {
        remove(oid);
    }
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
