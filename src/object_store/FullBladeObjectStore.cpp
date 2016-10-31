/* Copyright 2016 Joao Carreira */

#include <iostream>
#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/utils.h"

namespace sirius {

FullBladeObjectStore::FullBladeObjectStore(const std::string& bladeIP,
        const std::string& port) :
    ObjectStore() {
    client.connect(bladeIP, port);
}

Object FullBladeObjectStore::get(ObjectID id) {
    // We do not implement this because
    // this store does not cache locally
    return 0;
}

void FullBladeObjectStore::get(ObjectID id, void*& ptr) {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        readToLocal(loc, ptr);
    } else {
        // object is not in store
        ptr = 0;
    }
}

bool FullBladeObjectStore::put(Object obj, uint64_t size, ObjectID id) {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        return writeRemote(obj, loc);
    } else {
        // we could merge this into a single message (?)
        sirius::AllocRec allocRec = client.allocate(size);
        insertObjectLocation(id, size, allocRec);
        return writeRemote(obj, BladeLocation(size, allocRec));
    }
}

bool FullBladeObjectStore::readToLocal(BladeLocation loc, void* ptr) {
    client.read_sync(loc.allocRec, 0, loc.size, ptr);
    return true;
}

bool FullBladeObjectStore::writeRemote(Object obj, BladeLocation loc) {
    client.write_sync(loc.allocRec, 0, loc.size, obj);
    return true;
}

bool FullBladeObjectStore::insertObjectLocation(ObjectID id,
        uint64_t size, const AllocRec& allocRec) {
    objects_[id] = BladeLocation(size, allocRec);

    return true;
}

void FullBladeObjectStore::printStats() {
}

}  //  namespace sirius

