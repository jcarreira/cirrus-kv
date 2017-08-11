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
#include "utils/CirrusTime.h"
#include "utils/Log.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"
#include "common/Exception.h"

#define INSTS (1000000) // 1 million
#define LOADING_DONE (INSTS + 1)

#define MODEL_GRAD_SIZE 10

#define BILLION (1000000000ULL)

#define SAMPLE_BASE 0
#define MODEL_BASE (BILLION)
#define GRADIENT_BASE (2 * BILLION)
#define LABEL_BASE (3 * BILLION)
uint64_t nworkers = 1;

int features_per_sample = 10; 
int samples_per_batch =100;
int batch_size = samples_per_batch * features_per_sample;

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
    operator()(void* data, unsigned int des_size) {
        unsigned int size = sizeof(T) * numslots;

        if (des_size != size) {
            std::cout << "Error in deserializing."
               << " Expecting size: " << size
               << " got des_size: " << des_size
               << std::endl;
            throw std::runtime_error(
                    "Wrong deserializer size at c_array_deserializer");
        }

        // cast the pointer
        T* ptr = reinterpret_cast<T*>(data);

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
        std::cout << "[SER]"
            << " lr_model_serializer n: " << n << std::endl;
        auto model_serialized = v.serialize();
        std::cout << "[SER]"
            << " lr_model_serializer serialized size: "
            << model_serialized.second << std::endl;

        return model_serialized;
    }

private:
    int n;
}; 

class lr_model_deserializer {
public:
    lr_model_deserializer(uint64_t n) : n(n) {}
    
    LRModel
    operator()(void* data, unsigned int des_size) {
        LRModel model(n);
        if (des_size != model.getSerializedSize()) {
            throw std::runtime_error(
                    "Wrong deserializer size at lr_model_deserializer");
        }
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
        std::cout << "[SER]"
            << " LRGradient serializer writing size: "
            << g.getSerializedSize()
            << std::endl;
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
    operator()(void* data, unsigned int des_size) {
        std::cout << "[SER]"
            << " LRGradient deserializing n: "
            << n
            << " size: " << des_size
            << std::endl;
        LRGradient gradient(n);
        if (des_size != gradient.getSerializedSize()) {
            throw std::runtime_error(
                    "Wrong deserializer size at lr_gradient_deserializer");
        }

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

    // we need n object stores for different types of data:
    // 1. model
    // 2. gradient
    std::cout << "[PS] "
        << "PS connecting to store" << std::endl;
    cirrus::TCPClient client;

    cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
        model_store(IP, PORT, &client,
            lr_model_serializer(MODEL_GRAD_SIZE),
            lr_model_deserializer(MODEL_GRAD_SIZE));
    cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
        gradient_store(IP, PORT, &client,
            lr_gradient_serializer(MODEL_GRAD_SIZE),
            lr_gradient_deserializer(MODEL_GRAD_SIZE));


    std::cout << "[PS] "
        << "PS task initializing model" << std::endl;
    // initialize model

    LRModel model(MODEL_GRAD_SIZE);
    model.randomize();
    std::cout << "[PS] "
        << "PS publishing model at id: "
        << MODEL_BASE
        << std::endl;
    // publish the model back to the store so workers can use it
    model_store.put(MODEL_BASE, model);
    
    std::cout << "[PS] "
        << "PS getting model" << std::endl;
    auto m = model_store.get(MODEL_BASE);
    std::cout << "[PS] "
        << "PS model is here" << std::endl;

    // we keep a version number for the gradient produced by each worker
    std::vector<int> gradientCounts;
    gradientCounts.resize(10);

    bool first_time = true;

    return;
    while (1) {
        // for every worker, check for a new gradient computed
        // if there is a new gradient, get it and update the model
        // once model is updated publish it
        for (uint32_t worker = 0; worker < nworkers; ++worker) {

            int gradient_id = GRADIENT_BASE + worker;
            std::cout << "[PS] "
                << "PS task checking gradient id: "
                << gradient_id << std::endl;
            
            // get gradient from store
            LRGradient gradient(MODEL_GRAD_SIZE);
            try {
                gradient = gradient_store.get(GRADIENT_BASE + gradient_id);
            } catch(const cirrus::NoSuchIDException& e) {
                if (!first_time) {
                    throw std::runtime_error("PS task not able to get a gradient");
                }
                // this happens because the worker task
                // has not uploaded the gradient yet
                sleep(1);
                continue;
            }

            first_time = false;

            std::cout << "[PS] "
                << "PS task received gradient with #count: "
                << gradient.getCount()
                << std::endl;

            // check if this is a gradient we haven't used before
            if (gradient.getCount() > gradientCounts[worker]) {
                std::cout << "[PS] "
                    << "PS task received new gradient: "
                    << gradient.getCount()
                    << std::endl;

                // if it's new
                gradientCounts[worker] = gradient.getCount();

                // do a gradient step and update model
                std::cout << "[PS] "
                    << "Updating model" << std::endl;

                model.sgd_update(config.get_learning_rate(), &gradient);

                std::cout << "[PS] "
                    << "Publishing model" << std::endl;
                // publish the model back to the store so workers can use it
                //model_store.put(MODEL_BASE, model);
            }
        }
    }
}

void run_compute_error_task(const Configuration& config) {

    std::cout << "Compute error task connecting to store" << std::endl;

    cirrus::TCPClient client;

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
                c_array_serializer<double>(samples_per_batch),
                c_array_deserializer<double>(samples_per_batch));

    bool first_time = true;
    while (1) {
        sleep(2);

        double loss = 0;
        try {
            // first we get the model
            std::cout << "Compute error task getting the model at id: "
                << MODEL_BASE
                << std::endl;
            LRModel model(MODEL_GRAD_SIZE);
            model = model_store.get(MODEL_BASE);

            // then we compute the error
            loss = 0;
            uint64_t count = 0;
            //uint64_t n_iter = dataset.samples() / samples_per_batch;

            // we don't really know how many minibatches there are
            // we keep reading until a get() fails
            for (int i = 0; 1; ++i) {
                std::shared_ptr<double> samples;
                std::shared_ptr<double> labels;;

                try {
                    samples = samples_store.get(SAMPLE_BASE + i);
                    labels = labels_store.get(LABEL_BASE + i);
                } catch(const cirrus::NoSuchIDException& e) {
                    if (i == 0) goto continue_point; // no loss to be computed
                    else goto compute_loss; // we looked at all minibatches
                }
        
                Dataset dataset(samples.get(), labels.get(),
                        samples_per_batch, features_per_sample);
                loss += model.calc_loss(dataset);
                count++;
            }

compute_loss:
            loss = loss / count; // compute average loss over all minibatches

        } catch(const cirrus::NoSuchIDException& e) {
            std::cout << "run_compute_error_task unknown id" << std::endl;
        }

        std::cout << "Learning loss: " << loss << std::endl;

continue_point:
        first_time = first_time;
    }
}

void run_worker_task(const Configuration& config) {

    std::cout << "[WORKER] "
        << "Worker task connecting to store" << std::endl;

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
                c_array_serializer<double>(samples_per_batch),
                c_array_deserializer<double>(samples_per_batch));

    bool first_time = true;

    int samples_id = 0;
    int labels_id  = 0;
    int gradient_id = GRADIENT_BASE;
    uint64_t count = 0;
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
            std::cout << "[WORKER] "
                << "Worker task getting the model at id: "
                << MODEL_BASE
                << std::endl;
            model = model_store.get(MODEL_BASE);

            std::cout << "[WORKER] "
                << "Worker task getting the training data with id: "
                << samples_id << std::endl;

            samples = samples_store.get(SAMPLE_BASE + samples_id);
            std::cout << "[WORKER] "
                << "Worker task received training data with id: "
                << samples_id << std::endl;
            labels = labels_store.get(LABEL_BASE + labels_id);
            std::cout << "[WORKER] "
                << "Worker task received label data with id: "
                << samples_id << std::endl;
        } catch(const cirrus::NoSuchIDException& e) {
            if (!first_time) {
                throw std::runtime_error(
                        "Couldn't get object from run_worker_task");
            }
            // this happens because the ps task
            // has not uploaded the model yet
            std::cout << "[WORKER] "
                << "Model could not be found at id: "
                    << MODEL_BASE
                    << std::endl;
            sleep(1);
            continue;
        }
        first_time = false;

        Dataset dataset(samples.get(), labels.get(),
                samples_per_batch, features_per_sample);

        std::cout << "[WORKER] "
            << "Worker task computing gradient" << std::endl;
        // compute mini batch gradient
        auto gradient = model.minibatch_grad(0, dataset.samples_,
                labels.get(), samples_per_batch, config.get_epsilon());
        gradient->setCount(count++);
        
        std::cout << "[WORKER] "
            << "Worker task storing gradient at id: "
            << gradient_id
            << std::endl;

        try {
            gradient_store.put(GRADIENT_BASE + gradient_id,
                    *dynamic_cast<LRGradient*>(gradient.get()));
        } catch(...) {
            std::cout << "[WORKER] "
                << "Worker task error doing put of gradient"
                << std::endl;
        }

        // move to next batch of samples
        samples_id++;
        labels_id++;
    }
}

/**
  * Load the object store with the training dataset
  * It reads from the criteo dataset files and writes to the object store
  * It signals when work is done by changing a bit in the object store
  */
void run_loading_task(const Configuration& config) {
    std::cout << "[LOADER] "
        << "Read criteo input..."
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

    std::cout << "[LOADER] "
        << "Read "
        << dataset.samples()
        << " samples "
        << std::endl;
 
    std::cout << "[LOADER] "
        << "Adding "
        << dataset.samples()
        << " samples in batches of size: "
        << batch_size
        << std::endl;

    // We put in batches of N samples
    for (int i = 0; i < dataset.samples() / samples_per_batch; ++i) {
        
        std::cout << "[LOADER] "
            << "Building samples batch" << std::endl;
        /**
          * Build sample object
          */
        auto sample = std::shared_ptr<double>(
                            new double[samples_per_batch * features_per_sample],
                        std::default_delete<double[]>());
        // this memcpy can be avoided with some trickery
        std::memcpy(sample.get(), dataset.sample(i * samples_per_batch),
                sizeof(double) * batch_size);
        
        std::cout << "[LOADER] "
            << "Building labels batch" << std::endl;
        /**
          * Build label object
          */
        auto label = std::shared_ptr<double>(
                            new double[samples_per_batch],
                        std::default_delete<double[]>());
        // this memcpy can be avoided with some trickery
        std::memcpy(label.get(), dataset.label(i * samples_per_batch),
                sizeof(double) * samples_per_batch);

        try {
            std::cout << "[LOADER] "
                << "Adding sample batch id: "
                << i
                << " samples with size: " << sizeof(double) * batch_size
                << std::endl;
            samples_store.put(SAMPLE_BASE + i, sample);
            std::cout << "[LOADER] "
                << "Putting label" << std::endl;
            labels_store.put(LABEL_BASE + i, label);
        } catch(...) {
            std::cout << "[LOADER] "
                << "Caught exception" << std::endl;
            exit(-1);
        }
    }

    std::cout << "[LOADER] "
        << "Trying to get sample with id: " << 0
        << std::endl;

    auto sample = samples_store.get(0);
    
    std::cout << "[LOADER] "
        << "Got sample with id: " << 0
        << std::endl;

    std::cout << "[LOADER] "
        << "Added all samples" << std::endl;
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
        sleep(3);
        run_loading_task(config);
        sleep_forever();
    } else if (rank == 4) {
        sleep(5);
        //run_compute_error_task(config);
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

