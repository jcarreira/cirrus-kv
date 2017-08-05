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
#include "Utils.h"
#include "Model.h"
#include "LRModel.h"
#include "ModelGradient.h"

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/Time.h"
#include "utils/Log.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

#define INSTS (1000000) // 1 million
#define LOADING_DONE (INSTS + 1)
#define LEARNING_RATE (1e-5)

#define MODEL_GRAD_SIZE 10
#define GRADIENT_BASE 1000
#define MODEL_BASE 2000
#define EPSILON 0.1
int nworkers = 5;

using cirrus::LOG;
using cirrus::INFO;

template<typename T>
class c_array_serializer_simple {
public:
    c_array_serializer_simple(int nslots) : numslots(nslots) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const std::shared_ptr<T>& v) {
        std::unique_ptr<char[]> ptr(new char[numslots]);
        return std::make_pair(std::move(ptr), numslots);
    }

private:
    int numslots;
}; 

template<typename T>
class c_array_deserializer_simple {
public:
    c_array_deserializer_simple(int nslots) : numslots(nslots) {}
    
    std::shared_ptr<T>
    operator()(void* data, unsigned int /* size */) {
        unsigned int size = sizeof(T) * numslots;

        // cast the pointer
        T *ptr = reinterpret_cast<T*>(data);
        auto ret_ptr = std::shared_ptr<T>(new T[numslots],
                std::default_delete< T[]>());

        std::memcpy(ret_ptr.get(), ptr, size);
        return ret_ptr;
    }

private:
    int numslots;
};


/**
  * LRModel serializer / deserializer
  */
class lr_model_serializer {
public:
    lr_model_serializer(int n) : n(n) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const LRModel& v) {
        auto model_serialized = v.serialize();

        //return std::make_pair(model_serialized.first,
        //                    static_cast<unsigned int>(model_serialized.second));
        return model_serialized;
    }

private:
    int n;
}; 

class lr_model_deserializer {
public:
    lr_model_deserializer(uint64_t n) : n(n) {}
    
    LRModel
    operator()(void* data, unsigned int /* size */) {
        LRModel model(n);
        model.loadSerialized(data);
        return model;
    }

private:
    uint64_t n;
};

/**
  * LRGradient serializer / deserializer
  */
class lr_gradient_serializer {
public:
    lr_gradient_serializer(int n) : n(n) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const LRGradient& g) {
        std::unique_ptr<char[]> mem(new char[g.getSerializedSize()]);
        g.serialize(mem.get());

        return std::make_pair(std::move(mem), g.getSerializedSize());
    }

private:
    int n;
}; 

class lr_gradient_deserializer {
public:
    lr_gradient_deserializer(uint64_t n) : n(n) {}
    
    LRGradient
    operator()(void* data, unsigned int /* size */) {
        LRGradient gradient(n);
        gradient.loadSerialized(data);
        return gradient;
    }

private:
    uint64_t n;
};

void sleep_forever() {
    while (1) {
        sleep(1000);
    }
}

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "10.10.49.86";
static const uint32_t SIZE = 1;

// #define CHECK_RESULTS

void run_memory_task(const std::string& config_path) {
    std::cout << "Launching TCP server" << std::endl;
    int ret = system("~/tcpservermain");
    std::cout << "System returned: " << ret << std::endl;
}

/**
  * This task is used to compute the training error in the background
  */
void training_error_task(const std::string& config_path) {

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

    // we need n object stores for different types of data:
    // 1. model
    // 2. gradient
    cirrus::TCPClient client;

    cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
        model_store(IP, PORT, &client,
            lr_model_serializer(MODEL_GRAD_SIZE),
            lr_model_deserializer(MODEL_GRAD_SIZE));
    cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
        gradient_store(IP, PORT, &client,
            lr_gradient_serializer(MODEL_GRAD_SIZE),
            lr_gradient_deserializer(MODEL_GRAD_SIZE));

    LRModel model(MODEL_GRAD_SIZE);
    model.randomize();

    // we keep a count of the gradient for each worker
    std::vector<int> gradientCounts;

    while (1) {
        // for every worker, check for a new gradient computed
        // if there is a new gradient, get it and update the model
        // once model is updated publish it
        for (int worker = 0; worker < nworkers; ++worker) {

            int gradient_id = GRADIENT_BASE + worker;
            // get gradient from store
            auto gradient = gradient_store.get(gradient_id);

            // check if this is a gradient we haven't used before
            if (gradient.getCount() > gradientCounts[worker]) {
                // if it's new
                gradientCounts[worker] = gradient.getCount();

                // do a gradient step and update model
                LOG<INFO>("Updating model");

                model.sgd_update(LEARNING_RATE, &gradient);

                LOG<INFO>("Publishing model");
                // publish the model back to the store so workers can use it
                model_store.put(MODEL_BASE, model);
            }
        }
    }

    sleep(1000);
}

void run_worker_task(const std::string& config_path) {

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
        gradient_store(IP, PORT, &client,
            lr_gradient_serializer(MODEL_GRAD_SIZE),
            lr_gradient_deserializer(MODEL_GRAD_SIZE));
    cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
        model_store(IP, PORT, &client,
                lr_model_serializer(MODEL_GRAD_SIZE),
                lr_model_deserializer(MODEL_GRAD_SIZE));

    while (1) {
        // get model from parameter server
        // get data from iterator
        // compute gradient from model and data
        // send gradient to object store

        // maybe we can wait a few iterations to get the model
        auto model = model_store.get(MODEL_BASE);

        // compute mini batch gradient
        auto gradient = model.minibatch_grad(rank, dataset, labels, EPSILON);
        gradient_store.put(gradient_id, gradient);
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

