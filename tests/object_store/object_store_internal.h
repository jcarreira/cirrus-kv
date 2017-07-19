#ifndef TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_
#define TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_

#include <string.h>
#include <utility>
#include <memory>
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

/**
 * Given a boolean indicating whether or not to return an RDMA client, returns
 * a std::unique_ptr to a BladeClient for use by the test.
 * @param use_rdma_client boolean indicating whether or not to return an
 * RDMAClient.
 * @return a std::unique_ptr for either an RDMAClient or a TCPClient.
 */
std::unique_ptr<BladeClient> getClient(bool use_rdma_client) {
    std::unique_ptr<BladeClient> retClient;

    if (!use_rdma_client) {
        retClient = std::make_unique<TCPClient>();
    }
    #ifdef HAVE_LIBRDMACM
    else if (use_rdma_client) retClient = std::make_unique<RDMAClient>();
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
auto parse_mode(int argc, char *argv[]) -> bool {
    bool use_rdma_client;
    if (argc == 1) {
        // Default to TCP
        use_rdma_client = false;
    } else if (argc == 2) {
        if (strcmp(argv[1], "--tcp") == 0) {
            use_rdma_client = false;
        } else if (strcmp(argv[1], "--rdma") == 0) {
            use_rdma_client = true;
        } else {
            throw std::runtime_error("Mode argument unrecognized.");
        }
    } else {
        std::cout << "Error: ./test_*.cpp [mode] "
            "(--tcp or --rdma)" << std::endl;
        throw std::runtime_error("Error: ./test_*.cpp [mode] "
            "(--tcp or --rdma)");
    }
    return use_rdma_client;
}

}  // namespace cirrus

#endif  // TESTS_OBJECT_STORE_OBJECT_STORE_INTERNAL_H_
