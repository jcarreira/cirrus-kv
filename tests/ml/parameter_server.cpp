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

#include "Input.h"

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/Time.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

#define INSTS (1000000) // 1 million
#define LOADING_DONE (INSTS + 1)

template<typename T>
class c_array_serializer_simple {
public:
    c_array_serializer_simple(int nslots) : NUMSLOTS(nslots) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const std::shared_ptr<T>& v) {
        std::unique_ptr<char[]> ptr(new char[NUMSLOTS]);
        return std::make_pair(std::move(ptr), NUMSLOTS);
    }

private:
    int NUMSLOTS;
}; 

template<typename T>
class c_array_deserializer_simple {
public:
    c_array_deserializer_simple(int nslots) : NUMSLOTS(nslots) {}
    
    std::shared_ptr<T>
    operator()(void* data, unsigned int /* size */) {
        unsigned int size = sizeof(T) * NUMSLOTS;

        // cast the pointer
        T *ptr = reinterpret_cast<T*>(data);
        auto ret_ptr = std::shared_ptr<T>(new T[NUMSLOTS],
                std::default_delete< T[]>());

        std::memcpy(ret_ptr.get(), ptr, size);
        return ret_ptr;
    }

private:
    int NUMSLOTS;
};

void sleep_forever() {
    while (1) {
        sleep(1000);
    }
}

//std::shared_ptr<T> c_array_deserializer_simple(void* data,
//        unsigned int /* size */) {
//
//    unsigned int size = sizeof(T) * NUMSLOTS;
//
//    // cast the pointer
//    T *ptr = reinterpret_cast<T*>(data);
//    auto ret_ptr = std::shared_ptr<T>(new T[NUMSLOTS],
//            std::default_delete< T[]>());
//
//    std::memcpy(ret_ptr.get(), ptr, size);
//    return ret_ptr;
//}

//template<typename T, unsigned int NUMSLOTS>
//std::pair<std::unique_ptr<char[]>, unsigned int>
//c_array_serializer_simple(const std::shared_ptr<T>& v) {
//    unsigned int size = NUMSLOTS * sizeof(T);
//    // allocate the array to copy into
//    std::unique_ptr<char[]> ptr(new char[size]);
//    std::memcpy(ptr.get(), v.get(), size);
//    return std::make_pair(std::move(ptr), size);
//}

/*
 * Takes a pointer to raw mem passed in and copies it onto heap before returning
 * a smart pointer to it.
 */
//template<typename T, unsigned int NUMSLOTS>
//std::shared_ptr<T> c_array_deserializer_simple(void* data,
//        unsigned int /* size */) {
//
//    unsigned int size = sizeof(T) * NUMSLOTS;
//
//    // cast the pointer
//    T *ptr = reinterpret_cast<T*>(data);
//    auto ret_ptr = std::shared_ptr<T>(new T[NUMSLOTS],
//            std::default_delete< T[]>());
//
//    std::memcpy(ret_ptr.get(), ptr, size);
//    return ret_ptr;
//}

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.86";
static const uint32_t SIZE = 1;

// #define CHECK_RESULTS

struct Msg {
    int id;
};

void run_memory_task(const std::string& config_path) {
    std::cout << "Launching TCP server" << std::endl;
    int ret = system("~/tcpservermain");
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

    sleep(3);

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<Msg>
        store(IP, PORT, &client,
            cirrus::serializer_simple<Msg>,
            cirrus::deserializer_simple<Msg,
                sizeof(Msg)>);

    for (int i = 0; i < 10; ++i) {
        Msg m;
        m.id = 1337;
        store.put(1, m);
        std::cout << "Parameter Server task changed" << std::endl;
    }

    sleep(1000);
}

void run_worker_task(const std::string& config_path) {
    
    sleep(5);

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<Msg>
        store(IP, PORT, &client,
            cirrus::serializer_simple<Msg>,
            cirrus::deserializer_simple<Msg,
                sizeof(Msg)>);

    while (1) {
        try {
            Msg m = store.get(1);
            if (m.id == 1337) {
                std::cout << "worker task received right id" << std::endl;
                break;
            }
        } catch (...) {
        }
    }

    sleep(1000);
}

/**
  * This task is used to load the object store with the training dataset
  * It reads from the criteo dataset files and writes to the object store
  * It signals when work is done by changing a bit in the object store
  */
void run_loading_task(const std::string& config_path) {
    std::cout << "Read criteo input..."
        << std::endl;

    Input input;
    int features_per_sample = 10; 
    int batch_size = 100; 
    auto dataset = input.read_input_csv(
            "/nscratch/joao/criteo_data/day_0_test_1K.csv", " ", 3,
            features_per_sample, true);

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        ostore(IP, PORT, &client,
                c_array_serializer_simple<double>(features_per_sample),
                c_array_deserializer_simple<double>(features_per_sample));

    cirrus::TCPClient client2;
    cirrus::ostore::FullBladeObjectStoreTempl<int>
        inst_store(IP, PORT, &client2,
            cirrus::serializer_simple<int>,
            cirrus::deserializer_simple<int, sizeof(int)>);

    std::cout << "Read "
        << dataset.samples()
        << " samples "
        << std::endl;
 
    std::cout << "Adding "
        << dataset.samples()
        << " samples in batches of size: "
        << batch_size
        << std::endl;

    // We put in batches of N samples
    for (int i = 0; i < dataset.samples() / batch_size; ++i) {
        auto sample = std::shared_ptr<double>(
                            new double[batch_size * features_per_sample],
                        std::default_delete<double[]>());
        std::memcpy(sample.get(), dataset.sample(i * batch_size),
                sizeof(double) * batch_size * features_per_sample);

        std::cout << "Adding object id: "
            << i
            << " with size: " << sizeof(double) * batch_size * features_per_sample
            << std::endl;

        try {
            ostore.put(i, sample);
        } catch(...) {
            std::cout << "Caught exception" << std::endl;
            exit(-1);
        }
    }

    std::cout << "Setting loading done to 1" << std::endl;
    int done = 1;
    inst_store.put(LOADING_DONE, done);
    std::cout << "Setting loading done to 1 completed" << std::endl;

    sleep_forever();
}


void run_tasks(int rank, const std::string& config_path) {
    if (rank == 0) {
        run_memory_task(config_path);
    } else if (rank == 1) {
        sleep(10000);
        //run_ps_task(config_path);
    } else if (rank == 2) {
        sleep(10000);
        //run_worker_task(config_path);
    } else if (rank == 3) {
        run_loading_task(config_path);
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

void print_arguments() {
    std::cout << "./parameter_server <task number>" << std::endl;
    std::cout << "Task types: "
        << "0: memory" << std::endl
        << "1: ps" << std::endl
        << "2: worker" << std::endl
        << "3: loading" << std::endl
        << std::endl;
}

/**
  * Cirrus provides an interface for registering tasks
  * A task has a name, an entry point (function) and required resources
  * Once tasks are registered, a description of the tasks to be run can be generated
  * This description can be used to reserve a set of amazon instances where the tasks can be deployed
  * ./system --generate
  * ./system --run --task=1
  */ 
int main(int argc, char** argv) {

    std::cout << "Starting parameter server" << std::endl;

    int rank, nprocs;

    init_mpi(argc, argv);
    int err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    err = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    //check_mpi_error(err);

    if (argc != 2) {
        print_arguments();
        throw std::runtime_error("Wrong number of arguments");
    }

    char name[200];
    gethostname(name, 200);
    std::cout << "MPI multi task test running on hostname: " << name
        << " with rank: " << rank
        << std::endl;

    std::string config_path = argv[1];
    run_tasks(rank, config_path);

    MPI_Finalize();
    std::cout << "Test successful" << std::endl;

    return 0;
}

