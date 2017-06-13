/* Copyright 2016 Joao Carreira */

#include "src/object_store/FullBladeObjectStore.h"
#include "src/cache_manager/CacheManager.h"

namespace cirrus {

/**
  * A function that returns an object stored under a given ObjectID.
  * If no object is stored under that ObjectID, throws a cirrus::Exception.
  * @param oid the ObjectID that the object you wish to retrieve is stored
  * under.
  * @return a copy of the object stored on the server.
  */
T CacheManager<T>::get(int oid) {
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
        entry.obj = store.get(oid);
        return *entry.obj;
    }
}

/**
  * A function that stores an object obj in the remote store under ObjectID oid.
  * Calls the put() method in FullBladeObjectstore.h
  * @param oid the ObjectID to store under.
  * @param obj the object to store on the server
  * @see FullBladeObjectstore
  */
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
void CacheManager<T>::prefetch(ObjectID oid) {
    // set up local cache entry
    // Store object in the cache
    struct cache_entry& entry = cache[oid];
    cache_entry.obj = store->get(oid);
}

}
