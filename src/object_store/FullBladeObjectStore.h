#ifndef SRC_OBJECT_STORE_FULLBLADEOBJECTSTORE_H_
#define SRC_OBJECT_STORE_FULLBLADEOBJECTSTORE_H_

#include <string>
#include <iostream>
#include <utility>

#include "object_store/ObjectStore.h"
#include "client/BladeClient.h"
#include "utils/utils.h"
#include "utils/Time.h"
#include "utils/logging.h"
#include "common/Exception.h"
#include "common/Future.h"

#include "third_party/libcuckoo/src/cuckoohash_map.hh"
#include "third_party/libcuckoo/src/city_hasher.hh"

namespace cirrus {
namespace ostore {

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
                              BladeClient *client,
                              std::function<std::pair<std::unique_ptr<char[]>,
                              unsigned int>(const T&)> serializer,
                              std::function<T(void*, unsigned int)>
                              deserializer);

    T get(const ObjectID& id) const override;
    bool put(const ObjectID& id, const T& obj) override;
    bool remove(ObjectID) override;

    // std::function<bool(bool)> get_async(ObjectID, T*) const;
    // std::function<bool(bool)> put_async(Object, uint64_t, ObjectID);

    void printStats() const noexcept override;


 private:
    /**
      * The client that the store uses to achieve all interaction with the
      * remote store.
      */
    BladeClient *client;

    /** The size of serialized objects. This is obtained from the return
      * value of the serializer() function. We assume that all serialized
      * objects have the same length.
      */
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
FullBladeObjectStoreTempl<T>::FullBladeObjectStoreTempl(
        const std::string& bladeIP,
        const std::string& port,
        BladeClient* client,
        std::function<std::pair<std::unique_ptr<char[]>,
        unsigned int>(const T&)> serializer,
        std::function<T(void*, unsigned int)> deserializer) :
    ObjectStore<T>(), client(client),
    serializer(serializer), deserializer(deserializer) {
    client->connect(bladeIP, port);
}

/**
  * A function that retrieves the object at a specified object id.
  * @param id the ObjectID that the object should be stored under.
  * @return the object stored at id.
  */
template<class T>
T FullBladeObjectStoreTempl<T>::get(const ObjectID& id) const {
    /* This is safe as we will only reach here if a previous put has
       occured, thus setting the value of serialized_size. */
    if (serialized_size == 0) {
        // TODO: throw error message if get before put
    }
    /* This allocation provides a buffer to read the serialized object
       into. */
    void* ptr = ::operator new (serialized_size);

    // Read into the section of memory you just allocated
    client->read_sync(id, ptr, serialized_size);

    // Deserialize the memory at ptr and return an object
    T retval = deserializer(ptr, serialized_size);

    // Free the memory we stored the serialized object in.
    ::operator delete (ptr);
    return retval;
}


/**
  * Asynchronously copies object from remote blade to local DRAM.
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
// template<class T>
// std::function<bool(bool)>
// FullBladeObjectStoreTempl<T>::get_async(ObjectID id, T* ptr) const {
//
//     // Build future. Returns the object
//     /* This is safe as we will only reach here if a previous put has
//        occured, thus setting the value of serialized_size. */
//     if (serialized_size == 0) {
//         // TODO: throw error message if get before put
//     }
//     /* This allocation provides a buffer to read the serialized object
//        into. */
//     void* ptr = ::operator new (serialized_size);
//
//     // Read into the section of memory you just allocated
//     auto future = client->read_async(id, ptr, serialized_size);
//
//     // Future will have to deserialize this
//     // Deserialize the memory at ptr and return an object
//     T retval = deserializer(ptr, serialized_size);
//
//     // Free the memory we stored the serialized object in.
//     ::operator delete (ptr);
//     return retval;
// }

/**
  * A function that puts a given object at a specified object id.
  * @param id the ObjectID that the object should be stored under.
  * @param obj the object to be stored.
  * @return the success of the put.
  */
template<class T>
bool FullBladeObjectStoreTempl<T>::put(const ObjectID& id, const T& obj) {
    // Approach: serialize object passed in, push it to id
    // serialized_size is saved in the class, it is the size of pushed objects
    std::pair<std::unique_ptr<char[]>, unsigned int> serializer_out =
                                                        serializer(obj);
    std::unique_ptr<char[]> serial_ptr = std::move(serializer_out.first);
    serialized_size = serializer_out.second;

    return client->write_sync(id, serial_ptr.get(), serialized_size);
}

/**
  * Asynchronously copies object from local dram to remote blade.
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
// template<class T>
// std::function<bool(bool)>
// FullBladeObjectStoreTempl<T>::put_async(const ObjectID& id, const T& obj) {
//
//     std::pair<std::unique_ptr<char[]>, unsigned int> serializer_out =
//                                                         serializer(obj);
//     std::unique_ptr<char[]> serial_ptr = std::move(serializer_out.first);
//     serialized_size = serializer_out.second;
//
//     auto future = client->write_async (id, serial_ptr.get(), serialized_size);
//     // TODO: Build future to return to higher levels. Get will return bool
//     return fun;
// }

/**
  * Deallocates space occupied by object in remote blade.
  * @param id the ObjectID of the object to be removed from remote memory.
  * @return Returns true.
  */
template<class T>
bool FullBladeObjectStoreTempl<T>::remove(ObjectID id) {
    return client->remove(id);
}

template<class T>
void FullBladeObjectStoreTempl<T>::printStats() const noexcept {
}

}  // namespace ostore
}  // namespace cirrus

#endif  // SRC_OBJECT_STORE_FULLBLADEOBJECTSTORE_H_
