#ifndef SRC_OBJECT_STORE_OBJECTSTORE_H_
#define SRC_OBJECT_STORE_OBJECTSTORE_H_

#include <cstdint>
#include <vector>

#include "client/BladeClient.h"

namespace cirrus {

using ObjectID = uint64_t;

template<typename T>
class ObjectStore {
 public:
    /**
     * A future used for asynchronous put operations.
     */
    class ObjectStorePutFuture {
     public:
        explicit ObjectStorePutFuture(cirrus::BladeClient::ClientFuture
                                          client_future);
        ObjectStorePutFuture() {}
        void wait();

        bool try_wait();

        bool get();
     private:
        /**
         * A pointer to a BladeClient::ClientFuture that will be used for
         * all calls to wait and get.
         */
        cirrus::BladeClient::ClientFuture client_future;
    };

    /**
     * A future used for asynchronous get operations.
     */
    class ObjectStoreGetFuture {
     public:
        ObjectStoreGetFuture(cirrus::BladeClient::ClientFuture client_future,
                            std::shared_ptr<std::vector<char>> mem,
                            uint64_t serialized_size,
                            std::function<T(void*, unsigned int)> deserializer);
        ObjectStoreGetFuture() {}
        void wait();

        bool try_wait();

        T get();

     private:
        /**
         * A pointer to a BladeClient::ClientFuture that will be used for
         * all calls to wait and get.
         */
        cirrus::BladeClient::ClientFuture client_future;

        /**
         * A pointer to the memory that the BladeClient will write the
         * serialized object into.
         */
        std::shared_ptr<std::vector<char>> mem;

        /**
         * The size of serialized objects. This is obtained from the return
         * value of the serializer() function. We assume that all serialized
         * objects have the same length.
         */
         uint64_t serialized_size;

         // TODO(Tyler): Change to be a reference/pointer?
         /**
          * A function that reads the buffer passed in and deserializes it,
          * returning an object constructed from the information in the buffer.
          */
        std::function<T(void*, unsigned int)> deserializer;
    };

    ObjectStore() {}

    virtual T get(const ObjectID& id) const = 0;

    virtual bool put(const ObjectID& id, const T& obj) = 0;

    virtual void printStats() const noexcept = 0;

    virtual bool remove(ObjectID id) = 0;

    virtual ObjectStoreGetFuture get_async(const ObjectID& id) = 0;
    virtual ObjectStorePutFuture put_async(const ObjectID& id,
        const T& obj) = 0;

    virtual void get_bulk(ObjectID start, ObjectID last, T* data) = 0;
    virtual void put_bulk(ObjectID start, ObjectID last, T* data) = 0;
};


/**
 * Constructor for an ObjectStoreputFuture.
 * @param client_future the client_future that will be used for all
 * calls to wait, etc.
 */
template<class T>
ObjectStore<T>::ObjectStorePutFuture::ObjectStorePutFuture(
    cirrus::BladeClient::ClientFuture client_future) :
        client_future(client_future) {}

/**
 * Waits until the result of the put operation is available.
 * @return A boolean indicating whether the result is available.
 */
template<class T>
void ObjectStore<T>::ObjectStorePutFuture::wait() {
    return client_future.wait();
}

/**
 * Check whether the result of the put operation is available.
 * @return A boolean indicating whether the result of the put is available.
 * Returns true if it is available.
 */
template<class T>
bool ObjectStore<T>::ObjectStorePutFuture::try_wait() {
    return client_future.try_wait();
}

/**
 * Gets the result of the put operation. Similar to simply calling put_sync().
 * If there were any errors on the serverside during the get operation, a
 * corresponding error will be thrown when get() is called.
 * @return A boolean indicating the success of the operation.
 */
template<class T>
bool ObjectStore<T>::ObjectStorePutFuture::get() {
    return client_future.get();
}

/**
 * Constructor for an ObjectStoreGetFuture.
 * @param client_future the client_future that will be used for all
 * calls to wait, etc.
 * @param mem a shared pointer to the memory that the serialized object will
 * be read into by the BladeClient.
 * @param serialized_size the size of a serialized object.
 * @param deserializer a deserializer.
 */
template<class T>
ObjectStore<T>::ObjectStoreGetFuture::ObjectStoreGetFuture(
    cirrus::BladeClient::ClientFuture client_future,
    std::shared_ptr<std::vector<char>> mem,
    uint64_t serialized_size,
    std::function<T(void*, unsigned int)> deserializer) :
        client_future(client_future), mem(mem),
        serialized_size(serialized_size), deserializer(deserializer) {}

/**
 * Waits until the result of the Get operation is available.
 * @return A boolean indicating whether the result is available.
 */
template<class T>
void ObjectStore<T>::ObjectStoreGetFuture::wait() {
    return client_future.wait();
}

/**
 * Check whether the result of the Get operation is available.
 * @return A boolean indicating whether the result of the Get is available.
 * Returns true if it is available.
 */
template<class T>
bool ObjectStore<T>::ObjectStoreGetFuture::try_wait() {
    return client_future.try_wait();
}

/**
 * Gets the result of the Get operation. Similar to simply calling get_sync().
 * If there were any errors on the serverside during the get operation, a
 * corresponding error will be thrown when get() is called.
 * @return An object of type T that was returned from the remote server.
 */
template<class T>
T ObjectStore<T>::ObjectStoreGetFuture::get() {
    // Ensure that the result is available, and throw error if needed
    client_future.get();
    // Deserialize and return the memory.
    return deserializer(mem->data(), serialized_size);
}

}  // namespace cirrus

#endif  // SRC_OBJECT_STORE_OBJECTSTORE_H_
