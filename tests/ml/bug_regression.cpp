#include <iostream>
#include <string>

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
#define GRADIENT_BASE 1000
#define MODEL_BASE 2000
#define LABEL_BASE 3000
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

        uint64_t array_size = numslots * sizeof(T);
        // allocate array
        std::unique_ptr<char[]> ptr(new char[array_size]);

        // copy samples to array
        memcpy(ptr.get(), array, array_size);
        std::cout << "Serialized array with size: " << array_size << std::endl;
        return std::make_pair(std::move(ptr), array_size);
    }

private:
    int numslots;
}; 

template<typename T>
class c_array_deserializer{
public:
    c_array_deserializer(int nslots) : numslots(nslots) {}
    
    std::shared_ptr<T>
    operator()(void* data, unsigned int input_size) {
        unsigned int size = sizeof(T) * numslots;

        // cast the pointer
        T* ptr = reinterpret_cast<T*>(data);
        char* alloc = new char[size];
        memset(alloc, 0, size); 

        //auto ret_ptr = std::shared_ptr<T>(new T[numslots],
        //        std::default_delete< T[]>());

        //std::memcpy(ret_ptr.get(), ptr, size);
        std::cout << "Deserializing from address: "
            << ptr
            << " to addr: " << ((void*)alloc)
            << " with size: " << size
            << " informed with size: " << input_size
            << std::endl;
        std::memcpy(alloc, ptr, size);
        return std::shared_ptr<T>(reinterpret_cast<T*>(alloc),
                std::default_delete< T[]>());
        //return ret_ptr;
    }

private:
    int numslots;
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

void run_memory_task(const Configuration& config) {
    std::cout << "Launching TCP server" << std::endl;
    int ret = system("~/tcpservermain");
    std::cout << "System returned: " << ret << std::endl;
}

/**
  * Load the object store with the training dataset
  * It reads from the criteo dataset files and writes to the object store
  * It signals when work is done by changing a bit in the object store
  */
void run_loading_task(const Configuration& config) {
    std::cout << "[LOADER] "
        << "Connecting to store..."
        << std::endl;

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
        << "Read criteo input..."
        << std::endl;

    Input input;

    auto dataset = input.read_input_csv(
            config.get_input_path(),
            " ", 3,
            config.get_limit_cols(), true);
    std::cout << "[LOADER] "
        << "Done reading criteo..."
        << std::endl;

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
            samples_store.put(i, sample);
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
        sleep(3);
        run_loading_task(config);
    } else {
        throw std::runtime_error("Wrong number of tasks");
    }
}

void print_arguments() {
    std::cout << "./parameter_server config_file" << std::endl;
}

Configuration load_configuration(const std::string& config_path) {
    Configuration config;
    config.read(config_path);
    return config;
}

/**
  * Cirrus provides an interface for registering tasks
  * A task has a name, an entry point (function) and required resources
  * Once tasks are registered, a description of
  * the tasks to be run can be generated
  * This description can be used to reserve a set of amazon instances
  * where the tasks can be deployed
  * ./system --generate
  * ./system --run --task=1
  */ 
int main(int argc, char** argv) {

    std::cout << "Starting parameter server" << std::endl;

    int rank, nprocs;

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
    run_tasks(0, config);

    std::cout << "Test successful" << std::endl;

    return 0;
}

