#ifndef SRC_OBJECT_STORE_FULLBLADEOBJECTSTORE_H_
#define SRC_OBJECT_STORE_FULLBLADEOBJECTSTORE_H_

#include <string>
#include <iostream>
#include <utility>
#include <memory>

#include "object_store/ObjectStore.h"
#include "client/BladeClient.h"
#include "utils/utils.h"
#include "utils/Time.h"
#include "utils/logging.h"
#include "common/Exception.h"

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

    typename ObjectStore<T>::ObjectStoreGetFuture get_async(
            const ObjectID& id) override;
    typename ObjectStore<T>::ObjectStorePutFuture put_async(const ObjectID& id,
            const T& obj) override;

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

    // TODO(Tyler): Change these to be references or pointers?
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
        // TODO(Tyler): throw error message if get before put
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
  * @return Returns an ObjectStoreGetFuture;
  */
template<class T>
typename ObjectStore<T>::ObjectStoreGetFuture
FullBladeObjectStoreTempl<T>::get_async(const ObjectID& id) {
    /* This is safe as we will only reach here if a previous put has
       occured, thus setting the value of serialized_size. */
    if (serialized_size == 0) {
        // TODO(Tyler): throw error message if get before put
        // Alternatively, let the server handle it.
    }

    /* This allocation provides a buffer to read the serialized object
       into. */
    std::shared_ptr<std::vector<char>> ptr =
        std::make_shared<std::vector<char>>(std::vector<char>(serialized_size));
    
    // Read into the section of memory you just allocated
    auto client_future = client->read_async(id, ptr->data(), serialized_size);

    return typename ObjectStore<T>::ObjectStoreGetFuture(client_future, ptr,
            serialized_size, deserializer);
}

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

    // TODO(Tyler): This code in the body is duplicated in async. Pull it out?
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
  * @return Returns an ObjectStorePutFuture.
  */
template<class T>
typename ObjectStore<T>::ObjectStorePutFuture
FullBladeObjectStoreTempl<T>::put_async(const ObjectID& id, const T& obj) {
    std::pair<std::unique_ptr<char[]>, unsigned int> serializer_out =
                                                        serializer(obj);
    std::unique_ptr<char[]> serial_ptr = std::move(serializer_out.first);
    serialized_size = serializer_out.second;

    auto client_future = client->write_async(id,
                                           serial_ptr.get(),
                                           serialized_size);

    // Constructor takes a pointer to a client future
    return typename ObjectStore<T>::ObjectStorePutFuture(client_future);
}

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
