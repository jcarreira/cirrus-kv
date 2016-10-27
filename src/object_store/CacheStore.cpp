/* Copyright 2016 Joao Carreira */

#include <iostream>
#include <cstring>
#include "src/object_store/CacheStore.h"
#include "src/utils/utils.h"

namespace sirius {

CacheStore::CacheStore() :
    ObjectStore() {
#ifdef GOOGLE
    objects_.set_empty_key(ObjectID());
    sem_init(&hash_sem, 0, 1);
#elif !defined(CUCKOO)
//    objects_.resize(10000000);
    objects_ = new void*[10000000];
    std::memset(objects_, 0, 10000000 * sizeof(void*));
    //std::fill(objects_.begin(), objects_.end(), static_cast<void*>(0));
#endif
}
    
Object CacheStore::get(ObjectID name) {
    
//    std::cout << "Getting name: " << name
//        << std::endl;

    Object ret;
#ifdef GOOGLE
    google::dense_hash_map<ObjectID, Object>::iterator it;
    sem_wait(&hash_sem);
    it = objects_.find(name);
    if (it == objects_.end()) {
        sem_post(&hash_sem);
#elif defined(CUCKOO)
    bool found = objects_.find(name, ret);
    if (!found) {
#else
    if ((ret = objects_[name]) == 0) {
#endif
        return 0;
    } else {
#ifdef GOOGLE
        ret = it->second;
        sem_post(&hash_sem);
#endif
        return ret;
    }
}

bool CacheStore::put(Object obj, uint64_t size, ObjectID name) {
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
        mem = new char[size];
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

void CacheStore::printStats() {
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

} //  namespace sirius
