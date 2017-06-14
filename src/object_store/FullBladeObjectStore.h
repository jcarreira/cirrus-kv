/* Copyright 2016 Joao Carreira */

#ifndef _FULLBLADE_OBJECT_STORE_H_
#define _FULLBLADE_OBJECT_STORE_H_

#include <string>
#include <iostream>

#include "src/object_store/ObjectStore.h"
#include "src/client/BladeClient.h"
#include "src/utils/utils.h"
#include "src/utils/Time.h"
#include "src/utils/logging.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"
#include "third_party/libcuckoo/src/city_hasher.hh"

namespace cirrus {
namespace ostore {

/**
  * @brief A class that stores a size and allocation record.
  */
class BladeLocation {
public:
    BladeLocation(uint64_t sz, const AllocationRecord& ar) :
        size(sz), allocRec(ar) {}
    explicit BladeLocation(uint64_t sz = 0) :
        size(sz) {}

    uint64_t size;
    AllocationRecord allocRec;
};

/**
  * This store keeps objects in blades at all times
  * method get copies objects from blade to local nodes
  * method put copies object from local dram to remote blade
  */
template<class T = void>
class FullBladeObjectStoreTempl : public ObjectStore {
public:
    FullBladeObjectStoreTempl(const std::string& bladeIP, const std::string& port);

    Object get(const ObjectID&) const override;
    bool get(ObjectID, T*) const;
    std::function<bool(bool)> get_async(ObjectID, T*) const;

    bool getHandle(ObjectID, BladeLocation& bl) const;

    bool getHandle(const ObjectID&, BladeLocation&) const;

    bool put(Object, uint64_t, ObjectID, RDMAMem* mem = nullptr);
    std::function<bool(bool)> put_async(Object, uint64_t, ObjectID);
    virtual void printStats() const noexcept override;

    bool remove(ObjectID);

private:
    bool readToLocal(BladeLocation loc, void*) const;
    std::shared_ptr<FutureBladeOp> readToLocalAsync(BladeLocation loc,
            void* ptr) const;

    bool writeRemote(Object obj, BladeLocation loc, RDMAMem* mem = nullptr);
    std::shared_ptr<FutureBladeOp> writeRemoteAsync(Object obj,
            BladeLocation loc);
    bool insertObjectLocation(ObjectID id,
            uint64_t size, const AllocationRecord& allocRec);

    // hash to map oid and location
    // if oid is not found, object is not in store
    cuckoohash_map<ObjectID, BladeLocation, CityHasher<ObjectID> > objects_;
    mutable BladeClient client;
};

/**
  * @brief Instatiates a FullBladeObjectStoreTempl.
  * @param bladeIP the ip address of the blade to connect to.
  * @param port the port to connect to.
  * @see BladeClient.connect()
  */
template<class T>
FullBladeObjectStoreTempl<T>::FullBladeObjectStoreTempl(const std::string& bladeIP,
        const std::string& port) :
    ObjectStore() {
    client.connect(bladeIP, port);
}


/**
  * @brief Not implemented as this store does not cache locally.
  */
template<class T>
Object FullBladeObjectStoreTempl<T>::get(const ObjectID& id) const {
    // We do not implement this because
    // this store does not cache locally
    throw std::runtime_error("Not implemented");
    return 0;
}

/**
  * @brief Copies object from blade to local node.
  * @param id the ObjectID of the object to transfer.
  * @param ptr a pointer to where the object should be transferred to.
  */
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

/**
  * @brief Returns the BladeLocation that contains a given ObjectID.
  * @param id the ObjectID to find
  * @param a reference to a BladeLocation that will be set.
  * @return True if object exits, false otherwise.
  */
template<class T>
bool FullBladeObjectStoreTempl<T>::getHandle(const ObjectID& id, BladeLocation& loc) const {
    if (objects_.find(id, loc)) {
        return true;
    } else {
        return false;
    }
}


/**
  * @brief Asynchronously copies object from remote blade to local DRAM.
  * @param id the ObjectID of the object being retrieved.
  * @param ptr a pointer to the location where the object should be copied.
  * @return Returns an std::function<bool(bool)>, which in this case will be
  * a future that allows the user to see the status of the get. If called with
  * false as the argument, the function will wait until the operation is
  * complete. If true is passed as the argument, it will return immediately
  * with the status of the get. If object does not exist, will return
  * null pointer.
  * @see FutureBladeOp
  * @see readToLocalAsync()
  */
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

/**
  * @brief Copies object from local dram to remote blade.
  * Operates in a synchronous fashion.
  * @param id the ObjectID that obj should be stored under.
  * @param obj the object to store on the remote blade.
  * @param size the size of the obj being transferred
  * @param mem a pointer to an RDMAMem
  * @return Returns the success of the call to writeRemote()
  * @see writeRemote()
  */
template<class T>
bool FullBladeObjectStoreTempl<T>::put(Object obj, uint64_t size,
        ObjectID id, RDMAMem* mem) {
    BladeLocation loc;

    if (objects_.find(id, loc)) {
        return writeRemote(obj, loc, mem);
    } else {
        // we could merge this into a single message (?)
        cirrus::AllocationRecord allocRec;
        {
            TimerFunction tf("FullBladeObjectStoreTempl::put allocate", true);
            allocRec = client.allocate(size);
        }
        insertObjectLocation(id, size, allocRec);
        LOG<INFO>("FullBladeObjectStoreTempl::writeRemote after alloc");
        return writeRemote(obj, BladeLocation(size, allocRec), mem);
    }
}

/**
  * @brief Asynchronously copies object from local dram to remote blade.
  * @param id the ObjectID that obj should be stored under.
  * @param obj the object to store on the remote blade.
  * @param size the size of the obj being transferred
  * @param mem a pointer to an RDMAMem
  * @return Returns an std::function<bool(bool)>, which in this case will be
  * a future that allows the user to see the status of the put. If called with
  * false as the argument, the function will wait until the operation is
  * complete. If true is passed as the argument, it will return immediately
  * with the status of the put.
  * @see FutureBladeOp
  * @see writeRemoteAsync()
  */
template<class T>
std::function<bool(bool)>
FullBladeObjectStoreTempl<T>::put_async(Object obj, uint64_t size, ObjectID id) {
    BladeLocation loc;

    if (!objects_.find(id, loc)) {
        cirrus::AllocationRecord allocRec = client.allocate(size);
        insertObjectLocation(id, size, allocRec);
        loc = BladeLocation(size, allocRec);
    }

    auto future = writeRemoteAsync(obj, loc);

    //TimerFunction tf("create lambda", true);
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

/**
  * @brief Deallocates space occupied by object in remote blade.
  * @param id the ObjectID of the object to be removed from remote memory.
  * @return Returns true.
  */
template<class T>
bool FullBladeObjectStoreTempl<T>::remove(ObjectID id) {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        client.deallocate(loc.allocRec);
    } else {
        throw std::runtime_error("Error. Trying to do inexistent object");
    }
}

template<class T>
bool FullBladeObjectStoreTempl<T>::readToLocal(BladeLocation loc, void* ptr) const {
    RDMAMem mem(ptr, loc.size);
    client.read_sync(loc.allocRec, 0, loc.size, ptr, &mem);
    return true;
}

template<class T>
std::shared_ptr<FutureBladeOp> FullBladeObjectStoreTempl<T>::readToLocalAsync(
        BladeLocation loc, void* ptr) const {
    auto future = client.read_async(loc.allocRec, 0, loc.size, ptr);
    return future;
}

template<class T>
bool FullBladeObjectStoreTempl<T>::writeRemote(Object obj,
        BladeLocation loc, RDMAMem* mem) {

//    LOG<INFO>("FullBladeObjectStoreTempl::writeRemote");
    RDMAMem mm(obj, loc.size);

    {
        //TimerFunction tf("writeRemote", true);
        //LOG<INFO>("FullBladeObjectStoreTempl:: writing sync");
        client.write_sync(loc.allocRec, 0, loc.size, obj,
                nullptr == mem ? &mm : mem);
    }
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
