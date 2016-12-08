/* Copyright 2016 Joao Carreira */

#include <iostream>
#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/utils.h"
#include "src/utils/logging.h"

namespace sirius {

FullBladeObjectStore::FullBladeObjectStore(const std::string& bladeIP,
        const std::string& port) :
    ObjectStore() {
    client.connect(bladeIP, port);
}

Object FullBladeObjectStore::get(const ObjectID& id) const {
    // We do not implement this because
    // this store does not cache locally
    throw std::runtime_error("Not implemented");
    return 0;
}

bool FullBladeObjectStore::get(ObjectID id, void* ptr) const {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        readToLocal(loc, ptr);
        return true;
    } else {
        // object is not in store
        return false;
    }
}

std::function<bool(bool)>
FullBladeObjectStore::get_async(ObjectID id, void* ptr) const {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        auto future = readToLocalAsync(loc, ptr);

        auto fun = [future](bool try_wait) -> bool {
            if (try_wait) {
                return future->try_wait();
            } else {
                future->wait();
                return true;
            }
        };
        return fun;
    } else {
        // object is not in store
        return std::function<bool(bool)>();
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

std::function<bool(bool)>
FullBladeObjectStore::put_async(Object obj, uint64_t size, ObjectID id) {
    BladeLocation loc;

    if (!objects_.find(id, loc)) {
        sirius::AllocRec allocRec = client.allocate(size);
        insertObjectLocation(id, size, allocRec);
        loc = BladeLocation(size, allocRec);
    }

    auto future = writeRemoteAsync(obj, loc);

    auto fun = [future](bool try_wait) -> bool {
        if (try_wait) {
            return future->try_wait();
        } else {
            future->wait();
            return true;
        }
    };
    return fun;
}

bool FullBladeObjectStore::readToLocal(BladeLocation loc, void* ptr) const {
    client.read_sync(loc.allocRec, 0, loc.size, ptr);
    return true;
}

std::shared_ptr<FutureBladeOp> FullBladeObjectStore::readToLocalAsync(
        BladeLocation loc, void* ptr) const {
    auto future = client.read_async(loc.allocRec, 0, loc.size, ptr);
    return future;
}

bool FullBladeObjectStore::writeRemote(Object obj, BladeLocation loc) {
    client.write_sync(loc.allocRec, 0, loc.size, obj);
    return true;
}

std::shared_ptr<FutureBladeOp> FullBladeObjectStore::writeRemoteAsync(
        Object obj, BladeLocation loc) {
    auto future = client.write_async(loc.allocRec, 0, loc.size, obj);
    return future;
}

bool FullBladeObjectStore::insertObjectLocation(ObjectID id,
        uint64_t size, const AllocRec& allocRec) {
    objects_[id] = BladeLocation(size, allocRec);

    return true;
}

void FullBladeObjectStore::printStats() const noexcept {
}

}  //  namespace sirius

