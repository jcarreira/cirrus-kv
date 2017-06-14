/* Copyright 2016 Joao Carreira */

#ifndef _CACHEMANAGER_H_
#define _CACHEMANAGER_H_

#include "src/object_store/FullBladeObjectStore.h"
#include <map>

namespace cirrus {
using ObjectID = uint64_t;  // Will this work? should we include it?

  /**
    * A class that manages the cache and interfaces with the store.
    */
template<class T>
class CacheManager {
  public:
    /**
      * Constructor for the CacheManager class.
      * @param store a pointer to the ObjectStore that the CacheManager will
      * interact with. This is where all objects will be stored and retrieved
      * from.
      * @param cache_size the maximum number of objects that the cache should
      * hold.
      */
    CacheManager(cirrus::ostore::FullBladeObjectStoreTempl<T> *store,
                    uint64_t cache_size) :
                            store(store), max_size(cache_size){}
    T get(ObjectID oid);
    void put(ObjectID oid, T obj);
    void prefetch(ObjectID oid);

  private:
    cirrus::ostore::FullBladeObjectStoreTempl<T> *store;
    struct cache_entry {
      T obj;
    };    
    std::map<ObjectID, struct cache_entry> cache;
    uint64_t max_size;
};

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
  * the object being used in order to reduce latency.
  * @param oid the ObjectID that the object you wish to retrieve is stored
  * under.
  */
template<class T>
void CacheManager<T>::prefetch(ObjectID oid) {
    // set up local cache entry
    // Store object in the cache
    struct cache_entry& entry = cache[oid];
    entry.obj = store->get(oid);
}


}  // cirrus

#endif // _CACHEMANAGER_H_
