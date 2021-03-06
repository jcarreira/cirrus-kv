#ifndef TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_
#define TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_

#include <string.h>
#include <utility>
#include <memory>
#include <string>
#include "client/BladeClient.h"
#include "client/TCPClient.h"

#ifdef HAVE_LIBRDMACM
#include "client/RDMAClient.h"
#endif  // HAVE_LIBRDMACM

#include "common/Serializer.h"

namespace cirrus {

/**
  * Struct used in many of the tests.
  */
template<unsigned int SIZE>
struct Dummy {
 public:
    explicit Dummy(int id = 1492) : id(id) {}

 public:
    char data[SIZE];
    int id;
};

template<class T>
class serializer_simple : public cirrus::Serializer<T> {
 public:
    serializer_simple();
    uint64_t size(const T& object) const override;

    void serialize(const T& object, void *mem) const override;
};

template<typename T>
serializer_simple<T>::serializer_simple() {
}

template<typename T>
uint64_t serializer_simple<T>::size(const T& object) const {
    return sizeof(object);
}

template<typename T>
void serializer_simple<T>::serialize(const T& object, void* mem) const {
    std::memcpy(mem, &object, sizeof(object));
}


/**
 * This class copies the c style array underneath the pointer into a new
 * portion of memory and returns the size of the new portion as well as
 * its location. T will be an std::shared_ptr<int>
 */
template<typename T>
class c_int_array_serializer_simple : public cirrus::Serializer<T>{
 public:
    /**
     * Constructor for the serializer.
     * @param nslots number of objects of type T in the array to be serialized
     */
    explicit c_int_array_serializer_simple(unsigned int nslots) :
        num_slots(nslots) {}

    uint64_t size(const T& /* object */) const override {
        return num_slots * sizeof(int);
    }

    /**
     * Function that actually performs the serialization.
     * @param v a std::shared ptr to the first item in the array to be
     * serialized
     */
    void serialize(const T& ptr, void *mem) const override {
        int* address = ptr.get();
        unsigned int num_bytes = num_slots * sizeof(int);
        // copy the data
        std::memcpy(mem, address, num_bytes);
        return;
    }

 private:
    /** Number of items in the array being serialized. */
    unsigned int num_slots;
};

/**
 * Takes a pointer to raw mem passed in and copies it onto heap before returning
 * a smart pointer to it.
 */
template<typename T>
class c_array_deserializer_simple {
 public:
    /**
     * Constructor for the deserializer.
     * @param nslots number of objects of type T in the array to be deserialized
     */
    explicit c_array_deserializer_simple(unsigned int nslots) :
        num_slots(nslots) {}

    /**
     * Function that actually performs the deserialization.
     * @param data a pointer to the raw data to deserialize
     */
    std::shared_ptr<T>
    operator()(const void* data, unsigned int /* size */) {
        unsigned int size = sizeof(T) * num_slots;

        // cast the pointer
        const T *ptr = reinterpret_cast<const T*>(data);
        // allocate memory for the data to live in
        auto ret_ptr = std::shared_ptr<T>(new T[num_slots],
                std::default_delete< T[]>());
        // copy the data
        std::memcpy(ret_ptr.get(), ptr, size);
        return ret_ptr;
    }

 private:
     /** Number of items in the array being deserialized. */
     int num_slots;
};

/* Takes a pointer to raw mem passed in and returns as object. */
template<typename T, unsigned int SIZE>
T deserializer_simple(const void* data, unsigned int /* size */) {
    const T *ptr = reinterpret_cast<const T*>(data);
    T ret;
    std::memcpy(&ret, ptr, SIZE);
    return ret;
}

/**
 * Serializes std::string by copying the c_str into the designated buffer.
 */
class string_serializer_simple : public cirrus::Serializer<std::string>{
 public:
    uint64_t size(const std::string& v) const override {
        return v.size() + 1;
    }

    /**
     * Function that actually performs the serialization.
     * @param mem pointer to the buffer to write into
     * @param v the string to be serialized
     */
    void serialize(const std::string& v, void *mem) const override {
        auto length = v.size() + 1;
        std::unique_ptr<char[]> ptr(new char[length]);
        std::memcpy(mem, v.c_str(), length);
        return;
    }
};


/* Takes a pointer to raw mem passed in and returns as object. */
std::string string_deserializer_simple(const void* data,
    unsigned int /* size */) {
    const char *ptr = reinterpret_cast<const char*>(data);
    std::string ret(ptr);
    return ret;
}

namespace test_internal {
/**
 * Given a boolean indicating whether or not to return an RDMA client, returns
 * a std::unique_ptr to a BladeClient for use by the test.
 * @param use_rdma_client boolean indicating whether or not to return an
 * RDMAClient.
 * @return a std::unique_ptr for either an RDMAClient or a TCPClient.
 */
std::unique_ptr<BladeClient> GetClient(bool use_rdma_client) {
    std::unique_ptr<BladeClient> retClient;

    if (!use_rdma_client) {
        retClient = std::make_unique<TCPClient>();
    #ifdef HAVE_LIBRDMACM
    } else if (use_rdma_client) {
        retClient = std::make_unique<RDMAClient>();
    }
    #else
    }
    #endif  // HAVE_LIBRDMACM

    else
        throw std::runtime_error("RDMA specified on system without RDMA");

    return retClient;
}

/**
 * Parses command line arguments for a test.
 * @param argc number of command line arguments
 * @param argv array of pointers to actual arguments
 * @return boolean indicating true if RDMA should be used.
 */
bool ParseMode(int argc, char *argv[]) {
    bool use_rdma_client = false;
    if (argc >= 2) {
        if (strcmp(argv[1], "--tcp") == 0) {
            use_rdma_client = false;
        } else if (strcmp(argv[1], "--rdma") == 0) {
            use_rdma_client = true;
        } else {
            throw std::runtime_error("Error: Mode argument unrecognized. "
            "Usage: ./test_*.cpp <--tcp | --rdma> <ip>");
        }
    }
    return use_rdma_client;
}


/**
 * Given argc and argv, returns the ip as a std::string.
 * @param argc number of command line arguments
 * @param argv array of pointers to actual arguments
 * @return pointer to third command line argument, which should be ip address
 */
char* ParseIP(int argc, char *argv[]) {
    if (argc >= 3) {
        return argv[2];
    } else {
        throw std::runtime_error("Error: invalid number of arguments. "
        "Usage: ./test_*.cpp <--tcp | --rdma> <ip>");
    }
}

}  // namespace test_internal

}  // namespace cirrus

#endif  // TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_
