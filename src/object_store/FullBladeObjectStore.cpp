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
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        return readToLocal(id, loc);
    } else {
        // object is not in store
        return reinterpret_cast<Object>(0);
    }
}

bool FullBladeObjectStore::put(Object obj, uint64_t size, ObjectID id) {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        return writeRemote(obj, loc);
    } else {
        sirius::AllocRec allocRec = client.allocate(size);
        insertObjectLocation(id, size, allocRec);
        return false;
    }
}

Object FullBladeObjectStore::readToLocal(ObjectID id, BladeLocation loc) {
    // FIX: we want to put this into an LRU cache
    id = id;
    //
    Object ptr = new char[loc.size];
    if (!ptr) {
        DIE("Error allocating memory");
    }

    client.read_sync(loc.allocRec, 0, loc.size, ptr);

    return ptr;
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

