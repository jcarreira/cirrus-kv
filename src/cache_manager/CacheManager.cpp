/* Copyright 2016 Joao Carreira */

#include "src/object_store/FullBladeObjectStore.h"
#include "src/cache_manager/CacheManager.h"

namespace cirrus {

int* CacheManager::get(int oid) {
    // check if entry exists for the oid in cache
    // if entry exists, return if it is there, otherwise wait
    // return pointer
    if (cache.find(oid) != cache.end()) {
        struct cache_entry& entry = cache.find(oid)->second;

        if (entry.fetched_async) {
            entry.future(false);
        }
        return &entry.val;

    } else {
        //set up entry, pull synchronously
        struct cache_entry& entry = cache[oid];
        entry.fetched_async = false;
        store->get(oid, &entry.val);
        return &entry.val;
    }
}


bool CacheManager::put(int oid, int val) {
    // push to remote
    return store->put(&val, sizeof(int), oid);
}

void CacheManager::prefetch(int oid) {
    //set up local copy
    //call async get
    //only pull if it is not already in the cache
    if (cache.find(oid) == cache.end()) {
        struct cache_entry& entry = cache[oid];
        entry.fetched_async = true;
        auto future = store->get_async(oid, &entry.val);
        entry.future = future;
    }
}

}
