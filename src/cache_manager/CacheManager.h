#ifndef SRC_CACHE_MANAGER_CACHEMANAGER_H_
#define SRC_CACHE_MANAGER_CACHEMANAGER_H_

#include <map>
#include <vector>
#include <functional>
#include <algorithm>

#include "object_store/ObjectStore.h"
#include "cache_manager/EvictionPolicy.h"
#include "object_store/FullBladeObjectStore.h"
#include "common/Exception.h"
#include "utils/Log.h"

namespace cirrus {
using ObjectID = uint64_t;

/**
 * A class that manages the cache and interfaces with the store.
 */
template<class T>
class CacheManager {
 public:
    CacheManager(cirrus::ostore::FullBladeObjectStoreTempl<T> *store,
                 cirrus::EvictionPolicy *policy,
                 uint64_t cache_size);
    T get(ObjectID oid);
    void put(ObjectID oid, T obj);
    void prefetch(ObjectID oid);
    void remove(ObjectID oid);

 private:
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
    cirrus::EvictionPolicy *policy;
};


/**
  * Constructor for the CacheManager class. Any object added to the cache
  * needs to have a default constructor.
  * @param store a pointer to the ObjectStore that the CacheManager will
  * interact with. This is where all objects will be stored and retrieved
  * from.
  * @param cache_size the maximum number of objects that the cache should
  * hold. Must be at least one.
  */
template<class T>
CacheManager<T>::CacheManager(
                           cirrus::ostore::FullBladeObjectStoreTempl<T> *store,
                           cirrus::EvictionPolicy *policy,
                           uint64_t cache_size) :
                           store(store), policy(policy), max_size(cache_size) {
    if (cache_size < 1) {
        throw cirrus::CacheCapacityException(
              "Cache capacity must be at least one.");
    }
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
    std::vector<ObjectID> to_remove = policy->get(oid);
    evict_vector(to_remove);
    // check if entry exists for the oid in cache
    LOG<INFO>("Cache get called on oid: ", oid);
    auto cache_iterator = cache.find(oid);
    if (cache_iterator != cache.end()) {
        LOG<INFO>("Entry exists for oid: ", oid);
        // entry exists
        // Call future's get method if necessary
        struct cache_entry& entry = cache_iterator->second;
        if (!entry.cached) {
            LOG<INFO>("oid was prefetched");
            // TODO(Tyler): Should we return the result of the get directly
            // and avoid a potential extra copy? Tradeoff is copy now vs
            // copy in the future in case of repeated access.
            entry.obj = entry.future.get();
            entry.cached = true;
        }
        return entry.obj;

    } else {
        // entry does not exist.
        // set up entry, pull synchronously
        // TODO(Tyler): Do we save to the cache in this case?
        if (cache.size() == max_size) {
          throw cirrus::CacheCapacityException("Get operation would put cache "
                                             "over capacity.");
        }
        struct cache_entry& entry = cache[oid];
        entry.obj = store->get(oid);
        return entry.obj;
    }
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
    std::vector<ObjectID> to_remove = policy->put(oid);
    evict_vector(to_remove);
    // Push the object to the store under the given id
    // TODO(Tyler): Should we switch this to an async op for greater
    // performance potentially? what to do with the futures?
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
    // Check if it exists locally before prefetching
    std::vector<ObjectID> to_remove = policy->prefetch(oid);
    evict_vector(to_remove);
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
    evict(oid);
    policy->remove(oid);
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

}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_CACHEMANAGER_H_
