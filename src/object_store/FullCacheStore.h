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
//#define CUCKOO

#ifdef GOOGLE
#include <google/dense_hash_map>
#endif

static const uint64_t GB = 1024*1024*1024;

namespace sirius {

class FullCacheStore {
public:
    FullCacheStore()
    //   : ObjectStore()
    {
#ifdef GOOGLE
        objects_.set_empty_key(ObjectID());
        sem_init(&hash_sem, 0, 1);
#elif !defined(CUCKOO)
        objects_ = new void*[10000000];
        std::memset(objects_, 0, 10000000 * sizeof(void*));
#endif
        alloc_mem = new char[2 * GB];
    }
        
    inline Object get(const ObjectID& name) {
        return objects_[name];
    }
        
//    //    std::cout << "Getting name: " << name
//    //        << std::endl;
//
//        Object ret;
//#ifdef GOOGLE
//        google::dense_hash_map<ObjectID, Object>::iterator it;
//        sem_wait(&hash_sem);
//        it = objects_.find(name);
//        if (it == objects_.end()) {
//            sem_post(&hash_sem);
//#elif defined(CUCKOO)
//        bool found = objects_.find(name, ret);
//        if (!found) {
//#else
//        if ((ret = objects_[name]) == 0) {
//#endif
//            return 0;
//        } else {
//#ifdef GOOGLE
//            ret = it->second;
//            sem_post(&hash_sem);
//#endif
//            return ret;
//        }
//    }

    inline bool put(Object obj, uint64_t size, ObjectID name) {
        // right now we use C++ allocator
        // later I may use my own thing
        void *mem = 0;
        
    //    std::cout << "Putting name: " << name
    //        << std::endl;
       
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
            //mem = new char[size];
            mem = reinterpret_cast<void*>(
                    reinterpret_cast<uint64_t>(alloc_mem) + mem_last_index);
            mem_last_index += size;
            if (!mem)
                DIE("Error allocating cache memory");
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

    void printStats() {
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

private:
    void* alloc_mem;
    uint64_t mem_last_index = 0;
#ifdef GOOGLE
    google::dense_hash_map<ObjectID, Object> objects_;
#elif CUCKOO
    cuckoohash_map<ObjectID, Object, CityHasher<ObjectID> > objects_;
#else
    // assume names are integers
//    std::vector<Object> objects_;
    void** objects_;
#endif
    sem_t hash_sem;

    uint64_t CACHE_SIZE = 10000000;
};

}
#endif // _CACHE_STORE_H_
