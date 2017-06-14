/* Copyright 2016 Joao Carreira */

#ifndef _FULLBLADE_OBJECT_STORE_H_
#define _FULLBLADE_OBJECT_STORE_H_

#include <string>
#include <iostream>
#include <utility>

#include "src/object_store/ObjectStore.h"
#include "src/client/BladeClient.h"
#include "src/utils/utils.h"
#include "src/utils/Time.h"
#include "src/utils/logging.h"
#include "src/common/Exception.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"
#include "third_party/libcuckoo/src/city_hasher.hh"

namespace cirrus {
namespace ostore {

/**
  * A class that stores a size and allocation record.
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
template<class T>
class FullBladeObjectStoreTempl : public ObjectStore<T> {
public:
    FullBladeObjectStoreTempl(const std::string& bladeIP,
                            const std::string& port,
            std::function<std::pair<std::unique_ptr<char[]>, unsigned int>(const T&)> serializer,
            std::function<T(void*,unsigned int)> deserializer);

    T get(const ObjectID& id) const override;
    bool put(const ObjectID& id, const T& obj) override;

    std::function<bool(bool)> get_async(ObjectID, T*) const;
    std::function<bool(bool)> put_async(Object, uint64_t, ObjectID);
    virtual void printStats() const noexcept override;

    bool remove(ObjectID) override;

private:
    bool readToLocal(BladeLocation loc, void*) const;
    std::shared_ptr<FutureBladeOp> readToLocalAsync(BladeLocation loc,
            void* ptr) const;

    bool writeRemote(Object obj, BladeLocation loc, RDMAMem* mem = nullptr);
    std::shared_ptr<FutureBladeOp> writeRemoteAsync(Object obj,
            BladeLocation loc);
    bool insertObjectLocation(ObjectID id,
            uint64_t size, const AllocationRecord& allocRec);

    /**
      * hash to map oid and location.
      * if oid is not found, object is not in store.
      */
    cuckoohash_map<ObjectID, BladeLocation, CityHasher<ObjectID> > objects_;
    mutable BladeClient client;

    /** The size of serialized objects. This is obtained from the return
      * value of the serializer() function. We assume that all serialized
      * objects have the same length. */
    uint64_t serialized_size;

    /**
      * A function that takes an object and serializes it. Returns a pointer
      * to the buffer containing the serialized object as well as the size of
      * the buffer.
      */
    std::function<std::pair<std::unique_ptr<char[]>,
                                unsigned int>(const T&)> serializer;

    /**
      * A function that reads the buffer passed in and deserializes it,
      * returning an object constructed from the information in the buffer.
      */
    std::function<T(void*, unsigned int)> deserializer;
};

/**
  * Constructor for new object stores.
  * @param bladeIP the ip of the remote server.
  * @param port the port to use to communicate with the remote server
  * @param serializer A function that takes an object and serializes it.
  * Returns a pointer to the buffer containing the serialized object as
  * well as the size of the buffer. All serialized objects should be the
  * same length.
  * @param deserializer A function that reads the buffer passed in and
  * deserializes it, returning an object constructed from the information
  * in the buffer.
  */
template<class T>
FullBladeObjectStoreTempl<T>::FullBladeObjectStoreTempl(const std::string& bladeIP,
        const std::string& port,
        std::function<std::pair<std::unique_ptr<char[]>, unsigned int>(const T&)> serializer,
        std::function<T(void*, unsigned int)> deserializer) :
    ObjectStore<T>(), serializer(serializer), deserializer(deserializer) {
    client.connect(bladeIP, port);
}

/**
  * A function that retrieves the object at a specified object id.
  * @param id the ObjectID that the object should be stored under.
  * @return the object stored at id.
  */
template<class T>
T FullBladeObjectStoreTempl<T>::get(const ObjectID& id) const {
    BladeLocation loc;
    if (objects_.find(id, loc)) {
        /* This is safe as we will only reach here if a previous put has
           occured, thus setting the value of serialized_size. */

        /* This allocation provides a buffer to read the serialized object
           into. */
        void* ptr = ::operator new (serialized_size);

        // Read into the section of memory you just allocated
        readToLocal(loc, ptr);

        // Deserialize the memory at ptr and return an object
        T retval = deserializer(ptr, serialized_size);

        // Free the memory we stored the serialized object in.
        ::operator delete (ptr);
        return retval;
    } else {
        throw cirrus::Exception("Requested ObjectID does not exist remotely.");
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
  * A function that puts a given object at a specified object id.
  * @param id the ObjectID that the object should be stored under.
  * @param obj the object to be stored.
  * @return the success of the put.
  */
template<class T>
bool FullBladeObjectStoreTempl<T>::put(const ObjectID& id, const T& obj) {
    BladeLocation loc;

    // Approach: serialize object passed in, push it to id
    // serialized_size is saved in the class, it is the size of pushed objects

    std::pair<std::unique_ptr<char[]>, unsigned int> serializer_out =
                                                        serializer(obj);
    std::unique_ptr<char[]> serial_ptr = std::move(serializer_out.first);
    serialized_size = serializer_out.second;

    bool retval;
    if (objects_.find(id, loc)) {
        retval = writeRemote(serial_ptr.get(), loc, nullptr);
    } else {
        // we could merge this into a single message (?)
        cirrus::AllocationRecord allocRec;
        {
            TimerFunction tf("FullBladeObjectStoreTempl::put allocate", true);
            allocRec = client.allocate(serialized_size);
        }
        insertObjectLocation(id, serialized_size, allocRec);
        LOG<INFO>("FullBladeObjectStoreTempl::writeRemote after alloc");
        retval = writeRemote(serial_ptr.get(),
                          BladeLocation(serialized_size, allocRec),
                          nullptr);
    }
    return retval;

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
