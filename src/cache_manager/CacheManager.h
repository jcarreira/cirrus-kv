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
    CacheManager<T>(cirrus::ostore::FullBladeObjectStoreTempl<T> *store,
                    uint64_t cache_size) :
                            store(store), max_size(cache_size){}
    T get(ObjectID oid);
    void put(ObjectID oid, T obj);
    void prefetch(ObjectID oid);

  private:
    cirrus::ostore::FullBladeObjectStoreTempl<T> *store;
    std::map<ObjectID, struct cache_entry> cache;
    uint64_t max_size;
    struct cache_entry {
      T obj;
    };
};


}

#endif // _CACHEMANAGER_H_
