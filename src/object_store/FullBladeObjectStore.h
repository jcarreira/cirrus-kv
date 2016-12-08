/* Copyright 2016 Joao Carreira */

#ifndef _FULLBLADE_OBJECT_STORE_H_
#define _FULLBLADE_OBJECT_STORE_H_

#include <string>

#include "src/object_store/ObjectStore.h"
#include "src/client/BladeClient.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"   
#include "third_party/libcuckoo/src/city_hasher.hh"

/*
 * This store keeps objects in blades at all times
 * method get copies objects from blade to local nodes
 * method put copies object from local dram to remote blade
 */

namespace sirius {

class BladeLocation {
public:
    BladeLocation(uint64_t sz, const AllocRec& ar) :
        size(sz), allocRec(ar) {}
    explicit BladeLocation(uint64_t sz = 0) :
        size(sz) {}

    uint64_t size;
    AllocRec allocRec;
};

class FullBladeObjectStore : public ObjectStore {
public:
    FullBladeObjectStore(const std::string& bladeIP, const std::string& port);

    Object get(const ObjectID&) const;
    bool get(ObjectID, void*) const;
    std::function<bool(bool)> get_async(ObjectID, void*) const;

    bool put(Object, uint64_t, ObjectID);
    std::function<bool(bool)> put_async(Object, uint64_t, ObjectID);
    virtual void printStats() const noexcept;

private:
    bool readToLocal(BladeLocation loc, void*) const;
    std::shared_ptr<FutureBladeOp> readToLocalAsync(BladeLocation loc,
            void* ptr) const;

    bool writeRemote(Object obj, BladeLocation loc);
    std::shared_ptr<FutureBladeOp> writeRemoteAsync(Object obj,
            BladeLocation loc);
    bool insertObjectLocation(ObjectID id,
            uint64_t size, const AllocRec& allocRec);

    // hash to map oid and location
    // if oid is not found, object is not in store
    cuckoohash_map<ObjectID, BladeLocation, CityHasher<ObjectID> > objects_;
    mutable BladeClient client;
};

}

#endif // _FULLBLADE_OBJECT_STORE_H_

