/**
  * Showcase Cirrus with MPI
  */

#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <cctype>
#include <chrono>
#include <thread>
#include <random>
#include <memory>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

#include "Serialization.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1;

std::string deserializer_string(const void* data, unsigned int size) {
    std::string s;
    s.assign(reinterpret_cast<const char*>(data), size);
    return s;
}

/**
 * Test Cirrus on MPI
 */
void test_mpi_cirrus(int rank) {
    cirrus::TCPClient client;
    serializer_string serializer;
    cirrus::ostore::FullBladeObjectStoreTempl<std::string>
        store(IP, PORT, &client,
            serializer,
            deserializer_string);

    std::string d = "Test";

    try {
        store.put(rank, d);
    } catch(...) {
        throw std::runtime_error("Error inserting");
    }

    std::string d2 = store.get(rank);

    // should be 42
    std::cout << "d2: " << d2 << std::endl;

    if (d2 != "Test") {
        throw std::runtime_error("Wrong value");
    }
}

inline
void init_mpi(int argc, char**argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        std::cerr
            << "MPI implementation does not support multiple threads"
            << std::endl;
    }
}

int main(int argc, char** argv) {
    int rank, nprocs;

    init_mpi(argc, argv);
    int err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    err = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    char name[200];
    gethostname(name, 200);
    std::cout << "MPI test running on hostname: " << name << std::endl;

    test_mpi_cirrus(rank);
    MPI_Finalize();

    std::cout << "Test successful" << std::endl;

    return 0;
}
