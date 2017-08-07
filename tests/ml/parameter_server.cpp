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
#include "Configuration.h"

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/Time.h"
#include "utils/Log.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

#define INSTS (1000000) // 1 million
#define LOADING_DONE (INSTS + 1)

#define MODEL_GRAD_SIZE 10
#define GRADIENT_BASE 1000
#define MODEL_BASE 2000
uint64_t nworkers = 1;

int features_per_sample = 10; 
int samples_per_batch =100;
int batch_size = samples_per_batch * features_per_sample;

using cirrus::LOG;
using cirrus::INFO;

// This is used for non-array objects
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

template<typename T>
class c_array_serializer{
public:
    c_array_serializer(int nslots) : numslots(nslots) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const std::shared_ptr<T>& v) {
        T* array = v.get();

        // allocate array
        std::unique_ptr<char[]> ptr(new char[numslots * sizeof(T)]);

        // copy samples to array
        memcpy(ptr.get(), array, numslots * sizeof(T));
        return std::make_pair(std::move(ptr), numslots * sizeof(T));
    }

private:
    int numslots;
}; 

template<typename T>
class c_array_deserializer{
public:
    c_array_deserializer(int nslots) : numslots(nslots) {}
    
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
const char IP[] = "10.10.49.87";
static const uint32_t SIZE = 1;

// #define CHECK_RESULTS

void run_memory_task(const Configuration& config) {
    std::cout << "Launching TCP server" << std::endl;
    int ret = system("~/tcpservermain");
    std::cout << "System returned: " << ret << std::endl;
}

/**
  * This task is used to compute the training error in the background
  */
void training_error_task(const Configuration& config) {

}

/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void run_ps_task(const Configuration& config) {
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

    // initialize model

    LRModel model(MODEL_GRAD_SIZE);
    model.randomize();
    std::cout << "PS publishing model at id: "
        << MODEL_BASE
        << std::endl;
    // publish the model back to the store so workers can use it
    model_store.put(MODEL_BASE, model);
    
    std::cout << "PS getting model" << std::endl;
    auto m = model_store.get(MODEL_BASE);
    std::cout << "PS model is here" << std::endl;

    // we keep a version number for the gradient produced by each worker
    std::vector<int> gradientCounts;

    while (1) {
        // for every worker, check for a new gradient computed
        // if there is a new gradient, get it and update the model
        // once model is updated publish it
        for (uint32_t worker = 0; worker < nworkers; ++worker) {

            int gradient_id = GRADIENT_BASE + worker;
            // get gradient from store
            auto gradient = gradient_store.get(gradient_id);

            // check if this is a gradient we haven't used before
            if (gradient.getCount() > gradientCounts[worker]) {
                // if it's new
                gradientCounts[worker] = gradient.getCount();

                // do a gradient step and update model
                LOG<INFO>("Updating model");

                model.sgd_update(config.get_learning_rate(), &gradient);

                LOG<INFO>("Publishing model");
                // publish the model back to the store so workers can use it
                model_store.put(MODEL_BASE, model);
            }
        }
    }

    sleep_forever();
}

void run_worker_task(const Configuration& config) {

    cirrus::TCPClient client;

    // used to publish the gradient
    cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
        gradient_store(IP, PORT, &client,
            lr_gradient_serializer(MODEL_GRAD_SIZE),
            lr_gradient_deserializer(MODEL_GRAD_SIZE));

    // this is used to access the most up to date model
    cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
        model_store(IP, PORT, &client,
                lr_model_serializer(MODEL_GRAD_SIZE),
                lr_model_deserializer(MODEL_GRAD_SIZE));

    // this is used to access the training data sample
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        samples_store(IP, PORT, &client,
                c_array_serializer<double>(batch_size),
                c_array_deserializer<double>(batch_size));
    
    // this is used to access the training labels
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        labels_store(IP, PORT, &client,
                c_array_serializer<double>(batch_size),
                c_array_deserializer<double>(batch_size));

    int samples_id = 0;
    int labels_id  = 0;
    int gradient_id = GRADIENT_BASE;
    while (1) {
        // get model from parameter server
        // get data from iterator
        // compute gradient from model and data
        // send gradient to object store

        // maybe we can wait a few iterations to get the model
        LRModel model(MODEL_GRAD_SIZE);
        std::shared_ptr<double> samples;
        std::shared_ptr<double> labels;;
        try {
            std::cout << "Worker task getting the model at id: "
                << MODEL_BASE
                << std::endl;
            model = model_store.get(MODEL_BASE);

            std::cout << "Worker task getting the training data with id: " <<
                    samples_id << std::endl;
            samples = samples_store.get(samples_id);
            labels = labels_store.get(labels_id);
        } catch(const cirrus::NoSuchIDException& e) {
            if (!first_time) {
                throw std::runtime_error("Error in exception");
            }
            // this happens because the ps task
            // has not uploaded the model yet
            std::cout << "Model could not be found at id: "
                    << MODEL_BASE
                    << std::endl;
            sleep(1);
            continue;
        }
        first_time = false;

        auto samples = samples_store.get(samples_id);
        auto labels = labels_store.get(labels_id);
        Dataset dataset(samples.get(), labels.get(), samples_per_batch, features_per_sample);

        // compute mini batch gradient
        auto gradient = model.minibatch_grad(0, dataset.samples_,
                labels.get(), samples_per_batch, config.get_epsilon());
        
        std::cout << "Worker task storing gradient" << std::endl;
        gradient_store.put(gradient_id,
                           *dynamic_cast<LRGradient*>(gradient.get()));

        // move to next batch of samples
        samples_id++;
        labels_id++;
    }

    sleep(1000);
}

/**
  * Load the object store with the training dataset
  * It reads from the criteo dataset files and writes to the object store
  * It signals when work is done by changing a bit in the object store
  */
void run_loading_task(const Configuration& config) {
    std::cout << "Read criteo input..."
        << std::endl;

    Input input;

    auto dataset = input.read_input_csv(
            config.get_input_path(),
            " ", 3,
            config.get_limit_cols(), true);

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        samples_store(IP, PORT, &client,
                c_array_serializer<double>(samples_per_batch * features_per_sample),
                c_array_deserializer<double>(samples_per_batch * features_per_sample));
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        labels_store(IP, PORT, &client,
                c_array_serializer<double>(samples_per_batch),
                c_array_deserializer<double>(samples_per_batch));

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
    for (int i = 0; i < dataset.samples() / samples_per_batch; ++i) {
        
        LOG<INFO>("Building samples batch");
        /**
          * Build sample object
          */
        auto sample = std::shared_ptr<double>(
                            new double[samples_per_batch * features_per_sample],
                        std::default_delete<double[]>());
        // this memcpy can be avoided with some trickery
        std::memcpy(sample.get(), dataset.sample(i * samples_per_batch),
                sizeof(double) * batch_size);
        
        LOG<INFO>("Building labels batch");
        /**
          * Build label object
          */
        auto label = std::shared_ptr<double>(
                            new double[samples_per_batch],
                        std::default_delete<double[]>());
        // this memcpy can be avoided with some trickery
        std::memcpy(label.get(), dataset.label(i * samples_per_batch),
                sizeof(double) * samples_per_batch);

        std::cout << "Adding sample batch id: "
            << i
            << " samples with size: " << sizeof(double) * batch_size
            << std::endl;

        try {
            std::cout << "Putting sample" << std::endl;
            samples_store.put(i, sample);
            std::cout << "Putting label" << std::endl;
            labels_store.put(i, label);
        } catch(...) {
            std::cout << "Caught exception" << std::endl;
            exit(-1);
        }
    }

    std::cout << "Added all samples" << std::endl;
}

void run_tasks(int rank, const Configuration& config) {
    std::cout << "Run tasks rank: " << rank << std::endl;
    if (rank == 0) {
        //run_memory_task(config_path);
        sleep_forever();
    } else if (rank == 1) {
        sleep(8);
        run_ps_task(config);
        sleep_forever();
    } else if (rank == 2) {
        sleep(10);
        run_worker_task(config);
        sleep_forever();
    } else if (rank == 3) {
        sleep(5);
        run_loading_task(config);
        sleep_forever();
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
    std::cout << "./parameter_server config_file" << std::endl;
    //std::cout << "Task types: "
    //    << "0: memory" << std::endl
    //    << "1: ps" << std::endl
    //    << "2: worker" << std::endl
    //    << "3: loading" << std::endl
    //    << std::endl;
}

Configuration load_configuration(const std::string& config_path) {
    Configuration config;
    config.read(config_path);
    return config;
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

    auto config = load_configuration(argv[1]);
    // call the right task for this process
    run_tasks(rank, config);

    MPI_Finalize();
    std::cout << "Test successful" << std::endl;

    return 0;
}

