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

#define INSTS (1000000)  // 1 million
#define LOADING_DONE (INSTS + 1)

#define MODEL_GRAD_SIZE 10

#define BILLION (1000000000ULL)

#define SAMPLE_BASE 0
#define MODEL_BASE (BILLION)
#define GRADIENT_BASE (2 * BILLION)
#define LABEL_BASE (3 * BILLION)
uint64_t nworkers = 1;

int features_per_sample = 10;
int samples_per_batch = 100;
int batch_size = samples_per_batch * features_per_sample;

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


void run_tasks(int rank, const Configuration& config) {
    std::cout << "Run tasks rank: " << rank << std::endl;
    if (rank == 0) {
        // run_memory_task(config_path);
        sleep_forever();
    } else if (rank == 1) {
        sleep(8);
        PSTask pt(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, batch_size,
                samples_per_batch, features_per_sample,
                nworkers);
        pt.run(config);
        sleep_forever();
    } else if (rank == 2 || rank == 5) {
        sleep(10);
        LogisticTask lt(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, batch_size,
                samples_per_batch, features_per_sample);
        lt.run(config);
        //run_worker_task(config);
        sleep_forever();
    } else if (rank == 3) {
        sleep(3);
        LoadingTask lt(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, batch_size,
                samples_per_batch, features_per_sample);
        lt.run(config);
        sleep_forever();
    } else if (rank == 4) {
        sleep(5);
        ErrorTask et(IP, PORT, MODEL_GRAD_SIZE, MODEL_BASE,
                LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, batch_size,
                samples_per_batch, features_per_sample,
                nworkers);
        et.run(config);
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
}

Configuration load_configuration(const std::string& config_path) {
    Configuration config;
    config.read(config_path);
    return config;
}

int main(int argc, char** argv) {
    std::cout << "Starting parameter server" << std::endl;

    int rank, nprocs;

    init_mpi(argc, argv);
    int err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    err = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    // check_mpi_error(err);

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

