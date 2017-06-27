#ifndef SRC_CACHE_MANAGER_CACHEMANAGER_H_
#define SRC_CACHE_MANAGER_CACHEMANAGER_H_

#include <map>
#include "object_store/FullBladeObjectStore.h"
#include "common/Exception.h"

namespace cirrus {
using ObjectID = uint64_t;

  /**
    * A class that manages the cache and interfaces with the store.
    */
template<class T>
class CacheManager {
 public:
    CacheManager(cirrus::ostore::FullBladeObjectStoreTempl<T> *store,
                    uint64_t cache_size);
    T get(ObjectID oid);
    void put(ObjectID oid, T obj);
    void prefetch(ObjectID oid);

 private:
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
                           uint64_t cache_size) :
                           store(store), max_size(cache_size) {
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
    // check if entry exists for the oid in cache
    // if entry exists, return if it is there, otherwise wait
    // return pointer
    if (cache.find(oid) != cache.end()) {
        struct cache_entry& entry = cache.find(oid)->second;
        return entry.obj;

    } else {
        // set up entry, pull synchronously
        // Do we save to the cache in this case?
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
    if (cache.find(oid) == cache.end()) {
      struct cache_entry& entry = cache[oid];
      entry.obj = store->get(oid);
    }
}


}  // namespace cirrus

#endif  // SRC_CACHE_MANAGER_CACHEMANAGER_H_
