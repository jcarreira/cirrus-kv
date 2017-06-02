/* Copyright 2016 Joao Carreira */

#ifndef _CACHEMANAGER_H_
#define _CACHEMANAGER_H_

#include "src/object_store/FullBladeObjectStore.h"
#include <map>


/*
 * A class that manages the cache and interfaces with the store
 *
 *
 */

namespace cirrus {

class CacheManager {
  public:

    struct cache_entry {
      bool fetched_async;
      std::function<bool(bool)> future;
      int* ptr;
    };
    CacheManager(cirrus::ostore::FullBladeObjectStoreTempl<> *store) :
                            store(store){}
    int& get(int oid);
    bool put(int oid, int val);
    void prefetch(int oid);

  private:
    cirrus::ostore::FullBladeObjectStoreTempl<> *store;
    std::map<int, struct cache_entry> cache;

};


}

#endif // _CACHEMANAGER_H_
