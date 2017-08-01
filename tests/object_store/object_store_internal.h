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

namespace cirrus {

/**
  * Struct used in many of the tests.
  */
template<unsigned int SIZE>
struct Dummy {
    char data[SIZE];
    int id;
    explicit Dummy(int id = 1492) : id(id) {}
};

/*
 * This class copies the c style array underneath the pointer into a new
 * portion of memory and returns the size of the new portion as well as
 * its location.
 */
template<typename T>
class c_array_serializer_simple {
 public:
    /**
     * Constructor for the serializer.
     * @param nslots number of objects of type T in the array to be serialized
     */
    explicit c_array_serializer_simple(unsigned int nslots) :
        num_slots(nslots) {}

    /**
     * Function that actually performs the serialization.
     * @param v a std::shared ptr to the first item in the array to be
     * serialized
     */
    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const std::shared_ptr<T>& v) {
        unsigned int num_bytes = num_slots * sizeof(T);
        // allocate the array to copy into
        std::unique_ptr<char[]> ptr(new char[num_bytes]);
        return std::make_pair(std::move(ptr), num_bytes);
    }

 private:
    unsigned int num_slots;
};

/*
 * Takes a pointer to raw mem passed in and copies it onto heap before returning
 * a smart pointer to it.
 */
template<typename T>
class c_array_deserializer_simple {
 public:
    explicit c_array_deserializer_simple(unsigned int nslots) :
        num_slots(nslots) {}

    std::shared_ptr<T>
    operator()(void* data, unsigned int /* size */) {
        unsigned int size = sizeof(T) * num_slots;

        // cast the pointer
        T *ptr = reinterpret_cast<T*>(data);
        auto ret_ptr = std::shared_ptr<T>(new T[num_slots],
                std::default_delete< T[]>());

        std::memcpy(ret_ptr.get(), ptr, size);
        return ret_ptr;
    }

 private:
     int num_slots;
};

/* This function simply copies an object into a new portion of memory. */
template<typename T>
std::pair<std::unique_ptr<char[]>, unsigned int>
                         serializer_simple(const T& v) {
    std::unique_ptr<char[]> ptr(new char[sizeof(T)]);
    std::memcpy(ptr.get(), &v, sizeof(T));
    return std::make_pair(std::move(ptr), sizeof(T));
}

/* Takes a pointer to raw mem passed in and returns as object. */
template<typename T, unsigned int SIZE>
T deserializer_simple(void* data, unsigned int /* size */) {
    T *ptr = reinterpret_cast<T*>(data);
    T ret;
    std::memcpy(&ret, ptr, SIZE);
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
    bool use_rdma_client;
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
