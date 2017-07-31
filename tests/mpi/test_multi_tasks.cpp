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
#include "utils/Time.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";
static const uint32_t SIZE = 1;

// #define CHECK_RESULTS

struct Msg {
    int id;
};


#if 0

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);
    struct cirrus::Dummy<SIZE> d(42);

    try {
        store.put(1, d);
    } catch(...) {
        throw std::runtime_error("Error inserting");
    }

    struct cirrus::Dummy<SIZE> d2 = store.get(1);

    // should be 42
    std::cout << "d2.id: " << d2.id << std::endl;

    if (d2.id != 42) {
        throw std::runtime_error("Wrong value");
    }
#endif

void run_memory_task(const std::string& config_path) {
    std::cout << "Launching TCP server" << std::endl;
    int ret = system("./tcpservermain");
    std::cout << "System returned: " << ret << std::endl;
}


/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void run_ps_task(const std::string& config_path) {
    // should read from the config file how many workers there are

    // communication workers -> PS to communicate gradients
    // objects ids 5000 + worker ID

    // communication PS -> workers
    // object ids 10000 + worker ID

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<Msg>
        store(IP, PORT, &client,
            cirrus::serializer_simple<Msg>,
            cirrus::deserializer_simple<Msg,
                sizeof(Msg)>);

    Msg m;
    m.id = 1337;
    store.put(1, m);

    sleep(1000);
}

void run_worker_task(const std::string& config_path) {

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<Msg>
        store(IP, PORT, &client,
            cirrus::serializer_simple<Msg>,
            cirrus::deserializer_simple<Msg,
                sizeof(Msg)>);

    while (1) {
        Msg m = store.get(1);
        if (m.id == 1337) {
            std::cout << "worker task received right id" << std::endl;
        }
    }

    sleep(1000);
}


void run_tasks(int rank, const std::string& config_path) {
    if (rank == 0) {
        run_memory_task(config_path);
    } else if (rank == 1) {
        run_ps_task(config_path);
    } else if (rank == 2) {
        run_worker_task(config_path);
    } else {
        throw std::runtime_error("Wrong number of tasks");
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
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

int main(int argc, char** argv) {
    int rank, nprocs;

    init_mpi(argc, argv);
    int err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    err = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    //check_mpi_error(err);

    if (argc != 2) {
        throw std::runtime_error("Wrong number of arguments");
    }

    char name[200];
    gethostname(name, 200);
    std::cout << "MPI multi task test running on hostname: " << name << std::endl;

    std::string config_path = argv[1];
    run_tasks(rank, config_path);

    MPI_Finalize();
    std::cout << "Test successful" << std::endl;

    return 0;
}

