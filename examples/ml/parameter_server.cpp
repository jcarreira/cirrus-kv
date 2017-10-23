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

#include <Checksum.h>
#include "Input.h"
#include "Utils.h"
#include "Model.h"
#include "LRModel.h"
#include "ModelGradient.h"
#include "Configuration.h"
#include "Serializers.h"

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Log.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"
#include "common/Exception.h"
#include <Tasks.h>

#include "config.h"

#ifdef USE_S3
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#endif


#define INSTS (1000000)  // 1 million
#define LOADING_DONE (INSTS + 1)

#define MODEL_GRAD_SIZE 10

#define BILLION (1000000000ULL)
#define MILLION (1000000ULL)

#define SAMPLE_BASE   (0)
#define MODEL_BASE    (1 * BILLION)
#define GRADIENT_BASE (2 * BILLION)
#define LABEL_BASE    (3 * BILLION)
#define START_BASE    (4 * BILLION)
int nworkers = 2;

int num_classes = 2;
int features_per_sample = 10;
int samples_per_batch = 8000;
int batch_size = samples_per_batch * features_per_sample;

void sleep_forever() {
    while (1) {
        sleep(1000);
    }
}

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "172.31.6.107";

static const uint32_t SIZE = 1;

void run_memory_task(const Configuration& /* config */) {
    std::cout << "Launching TCP server" << std::endl;
    int ret = system("~/tcpservermain");
    std::cout << "System returned: " << ret << std::endl;
}


void run_tasks(int rank, const Configuration& config) {
    std::cout << "Run tasks rank: " << rank << std::endl;
    if (rank == 0) {
        // run_memory_task(config_path);
        sleep_forever();
    } else if (rank == 1) {
        //sleep(8);
        PSTask pt(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
                batch_size, samples_per_batch, features_per_sample,
                nworkers, rank);
        pt.run(config);
        sleep_forever();
    } else if (rank == 2) {
        //sleep(3);
        LoadingTask lt(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
                batch_size, samples_per_batch, features_per_sample,
                nworkers, rank);
        lt.run(config);
    } else if (rank == 3) {
        //sleep(5);
        ErrorTask et(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
                batch_size, samples_per_batch, features_per_sample,
                nworkers, rank);
        et.run(config);
        sleep_forever();
    } else if (rank >= 4 && rank < 4 + nworkers) {
        /**
          * Worker tasks run here
          * Number of tasks is determined by the value of nworkers
          */
        //sleep(10);
#ifdef PRELOAD_DATA
        std::cout << "Launching preloaded task" << std::endl;
        LogisticTaskPreloaded lt(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
                batch_size, samples_per_batch, features_per_sample,
                nworkers, rank);
#else
        LogisticTask lt(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
                batch_size, samples_per_batch, features_per_sample,
                nworkers, rank);
#endif
        lt.run(config, rank - 4);
        sleep_forever();

    } else {
        throw std::runtime_error("Wrong number of tasks");
    }
}

void print_arguments() {
    std::cout << "./parameter_server config_file nworkers rank" << std::endl;
}

Configuration load_configuration(const std::string& config_path) {
    Configuration config;
    std::cout << "Loading configuration"
        << std::endl;
    config.read(config_path);
    std::cout << "Configuration read"
        << std::endl;
    return config;
}

int main(int argc, char** argv) {
    std::cout << "Starting parameter server" << std::endl;

    int rank = 0;

    if (argc != 4) {
        print_arguments();
        throw std::runtime_error("Wrong number of arguments");
    }

    char name[200];
    gethostname(name, 200);
    std::cout << "MPI multi task test running on hostname: " << name
        << " with rank: " << rank
        << std::endl;

    nworkers = string_to<int>(argv[2]);
    std::cout << "Running parameter server with: "
        << nworkers << " workers"
        << std::endl;

    rank = string_to<int>(argv[3]);
    std::cout << "Running parameter server with: "
        << rank << " rank"
        << std::endl;
    auto config = load_configuration(argv[1]);
    config.print();

    // from config we get
    // 1. the number of classes
    // 2. the size of input
    samples_per_batch = config.get_minibatch_size();
    batch_size = samples_per_batch * features_per_sample;
    num_classes = config.get_num_classes();
    
    std::cout 
        << "samples_per_batch: " << samples_per_batch
        << " features_per_sample: " << features_per_sample
        << " batch_size: " << batch_size
        << std::endl;

    // call the right task for this process
    std::cout << "Running task"
        << std::endl;
    run_tasks(rank, config);

    //MPI_Finalize();
    std::cout << "Test successful" << std::endl;

    return 0;
}

