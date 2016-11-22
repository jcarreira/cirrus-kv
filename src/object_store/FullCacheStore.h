/* Copyright 2016 Joao Carreira */

#ifndef _CACHE_STORE_H_
#define _CACHE_STORE_H_

#include "src/object_store/ObjectStore.h"
#include <semaphore.h>

#include "third_party/libcuckoo/src/cuckoohash_map.hh"   
#include "third_party/libcuckoo/src/city_hasher.hh"

#include <vector>

#include <iostream>
#include <cstring>
#include "src/object_store/FullCacheStore.h"
#include "src/utils/utils.h"

//#define GOOGLE
#define CUCKOO

#ifdef GOOGLE
#include <google/dense_hash_map>
#endif

static const uint64_t GB = 1024*1024*1024;
static const size_t SIZE = 10000000; // FIX 

namespace sirius {

class FullCacheStore : public ObjectStore {
public:
    FullCacheStore()
       : ObjectStore()
    {
#ifdef GOOGLE
        objects_.set_empty_key(ObjectID());
        sem_init(&hash_sem, 0, 1);
#elif !defined(CUCKOO)
        // ugh
        objects_ = reinterpret_cast<void**>(new size_t[SIZE]);
        std::memset(objects_, 0, SIZE * sizeof(void*));
#endif

        // we need a serious allocation mechanism here
        // right now we use this big hack
        alloc_mem = new char[2 * GB];

        if (!alloc_mem)
            DIE("Error allocating cache memory");
    }

    ~FullCacheStore() {
#if !defined(CUCKOO) && !defined(GOOGLE)
        if (objects_)
            delete[] objects_;
#endif
        if (alloc_mem)
            delete[] alloc_mem;
    }
        
    inline Object get(const ObjectID& name) const {
        return objects_[name];
    }

    inline bool put(Object obj, uint64_t size, ObjectID name) {
        void *mem = 0;
        
#ifdef GOOGLE 
        sem_wait(&hash_sem);
        google::dense_hash_map<ObjectID, Object>::iterator it;
        it = objects_.find(name);
        if (it == objects_.end()) {
#elif defined(CUCKOO)
        bool found = objects_.find(name, mem);
        if (!found) {
#else
        if (!(mem = objects_[name])) {
#endif
            mem = reinterpret_cast<void*>(
                    reinterpret_cast<uint64_t>(alloc_mem) + mem_last_index);
            mem_last_index += size;
            objects_[name] = mem;
        } else {
#ifdef GOOGLE
            mem = it->second;
#else
#endif
        }
       
#ifdef GOOGLE 
        sem_post(&hash_sem);
#endif
        
        std::memcpy(mem, obj, size);
        
        return true;
    }

    void printStats() const noexcept {
#if defined(GOOGLE) || defined(CUCKOO)
        std::cout << "Cache size: " 
            << objects_.size() << std::endl;
#endif

#ifdef GOOGLE
        std::cout << "Max size: " 
            << objects_.max_size() << std::endl;
        std::cout << "Max bucket count: " 
            << objects_.max_bucket_count() << std::endl;
#endif
    }

    size_t getNumObjs() const noexcept {
#if defined(GOOGLE)  || defined(CUCKOO)
        return objects_.size();
#else
#error "We don't support this right now"
#endif

    }

    // we assume cache is not empty
    // we just drop 'first' object
    bool dropRandomObj() {
#ifdef GOOGLE
        objects_.erase(objects_.begin());
#elif defined(CUCKOO)
        auto tbl = objects_.lock_table();
        auto it = tbl.begin();
        objects_.erase(it->first);
#else
#error "We dont support this"
#endif
        return true;
    }
    
    bool dropLRUObj() {
        return false;
    }

private:
    char* alloc_mem;
    uint64_t mem_last_index = 0;
#ifdef GOOGLE
    google::dense_hash_map<ObjectID, Object> objects_;
#elif defined(CUCKOO)
    cuckoohash_map<ObjectID, Object, CityHasher<ObjectID> > objects_;
#else
    // assume names are values >= 0
    void** objects_;
#endif
    sem_t hash_sem;
};

}
#endif // _CACHE_STORE_H_
