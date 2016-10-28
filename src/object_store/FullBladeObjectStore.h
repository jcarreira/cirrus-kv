/* Copyright 2016 Joao Carreira */

#ifndef _FULLBLADE_OBJECT_STORE_H_
#define _FULLBLADE_OBJECT_STORE_H_

#include <string>

#include "src/object_store/ObjectStore.h"
#include "src/client/BladeClient.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"   
#include "third_party/libcuckoo/src/city_hasher.hh"

namespace sirius {

class BladeLocation {
public:
    BladeLocation(uint64_t sz, const AllocRec& ar) :
        size(sz), allocRec(ar) {}
    BladeLocation(){}

    uint64_t size;
    AllocRec allocRec;
};

class FullBladeObjectStore : public ObjectStore {
public:
    FullBladeObjectStore(const std::string& bladeIP, const std::string& port);

    Object get(ObjectID);
    bool put(Object, uint64_t, ObjectID);
    virtual void printStats();

private:
    Object readToLocal(ObjectID id, BladeLocation loc);
    bool writeRemote(Object obj, BladeLocation loc);
    bool insertObjectLocation(ObjectID id,
            uint64_t size, const AllocRec& allocRec);

    cuckoohash_map<ObjectID, BladeLocation, CityHasher<ObjectID> > objects_;
    BladeClient client;
};

}

#endif // _FULLBLADE_OBJECT_STORE_H_

