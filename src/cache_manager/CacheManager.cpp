/* Copyright 2016 Joao Carreira */

#include "src/object_store/FullBladeObjectStore.h"
#include "src/cache_manager/CacheManager.h"

namespace cirrus {

int* CacheManager::get(int oid) {
  // check if entry exists for the oid in cache
  // if entry exists, return if it is there, otherwise wait
  // return pointer
  if (cache.find(oid) != cache.end()) {
    struct cache_entry *entry = &(*cache.find(oid)).second;

    if (entry->fetched_async) {
      entry->future(false);
    }
    return &entry->val;

  } else {
    //set up entry, pull synnchronously
    struct cache_entry entry;
    entry.fetched_async = false;
    store->get(oid, &entry.val);
    cache[oid] = entry;
    return &((*cache.find(oid)).second.val);
  }
}


bool CacheManager::put(int oid, int val) {
    // put in local cache, then push
    struct cache_entry entry;
    entry.fetched_async = false;
    entry.val = val;
    cache[oid] = entry;

    return store->put(&val, sizeof(int), oid);
  }

void CacheManager::prefetch(int oid) {
    //set up local copy
    //call async get
    struct cache_entry entry;
    entry.fetched_async = true;

    auto future = store->get_async(oid, &entry.val);

    entry.future = future;
    cache[oid] = entry;
}

}
