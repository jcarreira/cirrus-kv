/* Copyright 2016 Joao Carreira */

#ifndef _FULLBLADE_OBJECT_STORE_H_
#define _FULLBLADE_OBJECT_STORE_H_

#include <string>
#include <iostream>

#include "src/object_store/ObjectStore.h"
#include "src/client/BladeClient.h"
#include "src/utils/utils.h"
#include "src/utils/logging.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"   
#include "third_party/libcuckoo/src/city_hasher.hh"

/*
 * This store keeps objects in blades at all times
 * method get copies objects from blade to local nodes
 * method put copies object from local dram to remote blade
 */

namespace sirius {
namespace ostore {

class BladeLocation {
public:
    BladeLocation(uint64_t sz, const AllocationRecord& ar) :
        size(sz), allocRec(ar) {}
    explicit BladeLocation(uint64_t sz = 0) :
        size(sz) {}

    uint64_t size;
    AllocationRecord allocRec;
};

template<class T = void>
class FullBladeObjectStoreTempl : public ObjectStore {
public:
    FullBladeObjectStoreTempl(const std::string& bladeIP, const std::string& port);

    Object get(const ObjectID&) const;
    bool get(ObjectID, T*) const;
    std::function<bool(bool)> get_async(ObjectID, T*) const;

    bool getHandle(const ObjectID&, BladeLocation&) const;

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
            uint64_t size, const AllocationRecord& allocRec);

    // hash to map oid and location
    // if oid is not found, object is not in store
    cuckoohash_map<ObjectID, BladeLocation, CityHasher<ObjectID> > objects_;
    mutable BladeClient client;
};

template<class T>
FullBladeObjectStoreTempl<T>::FullBladeObjectStoreTempl(const std::string& bladeIP,
        const std::string& port) :
    ObjectStore() {
    client.connect(bladeIP, port);
}

template<class T>
Object FullBladeObjectStoreTempl<T>::get(const ObjectID& id) const {
    // We do not implement this because
    // this store does not cache locally
    throw std::runtime_error("Not implemented");
    return 0;
}

template<class T>
bool FullBladeObjectStoreTempl<T>::get(ObjectID id, T* ptr) const {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        readToLocal(loc, reinterpret_cast<void*>(ptr));
        return true;
    } else {
        // object is not in store
        return false;
    }
}

template<class T>
bool FullBladeObjectStoreTempl<T>::getHandle(const ObjectID& id, BladeLocation& loc) const {
    if (objects_.find(id, loc)) {
        return true;
    } else {
        return false;
    }
}

template<class T>
std::function<bool(bool)>
FullBladeObjectStoreTempl<T>::get_async(ObjectID id, T* ptr) const {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        auto future = readToLocalAsync(loc, reinterpret_cast<void*>(ptr));

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

template<class T>
bool FullBladeObjectStoreTempl<T>::put(Object obj, uint64_t size, ObjectID id) {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        return writeRemote(obj, loc);
    } else {
        // we could merge this into a single message (?)
        sirius::AllocationRecord allocRec = client.allocate(size);
        insertObjectLocation(id, size, allocRec);
        return writeRemote(obj, BladeLocation(size, allocRec));
    }
}

template<class T>
std::function<bool(bool)>
FullBladeObjectStoreTempl<T>::put_async(Object obj, uint64_t size, ObjectID id) {
    BladeLocation loc;

    if (!objects_.find(id, loc)) {
        sirius::AllocationRecord allocRec = client.allocate(size);
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

template<class T>
bool FullBladeObjectStoreTempl<T>::readToLocal(BladeLocation loc, void* ptr) const {
    client.read_sync(loc.allocRec, 0, loc.size, ptr);
    return true;
}

template<class T>
std::shared_ptr<FutureBladeOp> FullBladeObjectStoreTempl<T>::readToLocalAsync(
        BladeLocation loc, void* ptr) const {
    auto future = client.read_async(loc.allocRec, 0, loc.size, ptr);
    return future;
}

template<class T>
bool FullBladeObjectStoreTempl<T>::writeRemote(Object obj, BladeLocation loc) {
    client.write_sync(loc.allocRec, 0, loc.size, obj);
    return true;
}

template<class T>
std::shared_ptr<FutureBladeOp> FullBladeObjectStoreTempl<T>::writeRemoteAsync(
        Object obj, BladeLocation loc) {
    auto future = client.write_async(loc.allocRec, 0, loc.size, obj);
    return future;
}

template<class T>
bool FullBladeObjectStoreTempl<T>::insertObjectLocation(ObjectID id,
        uint64_t size, const AllocationRecord& allocRec) {
    objects_[id] = BladeLocation(size, allocRec);

    return true;
}

template<class T>
void FullBladeObjectStoreTempl<T>::printStats() const noexcept {
}

}
}

#endif // _FULLBLADE_OBJECT_STORE_H_

