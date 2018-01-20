#include <stdlib.h>
#include <cstdint>
#include <string>
#include "Utils.h"
#include "Configuration.h"

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/Log.h"
#include "common/Exception.h"
#include <Tasks.h>

#include "config.h"

#define BILLION (1000000000ULL)
#define MILLION (1000000ULL)
#define SAMPLE_BASE   (0)
#define MODEL_BASE    (1 * BILLION)
#define GRADIENT_BASE (2 * BILLION)
#define LABEL_BASE    (3 * BILLION)
#define START_BASE    (4 * BILLION)

static const uint64_t GB = (1024*1024*1024);
static const uint32_t SIZE = 1;

void run_memory_task(const Configuration& /* config */) {
  std::cout << "Launching TCP server" << std::endl;
  int ret = system("~/tcpservermain");
  std::cout << "System returned: " << ret << std::endl;
}

void run_tasks(int rank, int nworkers, 
    int batch_size, const Configuration& config) {

  std::cout << "Run tasks rank: " << rank << std::endl;
  int features_per_sample = config.get_num_features();
  int samples_per_batch = config.get_minibatch_size();

  if (rank == PERFORMANCE_LAMBDA_RANK) {
    PerformanceLambdaTask lt(REDIS_IP, REDIS_PORT, features_per_sample, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    lt.run(config);
    sleep_forever();
  } else if (rank == PS_TASK_RANK) {
    PSTask pt(REDIS_IP, REDIS_PORT, features_per_sample, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    pt.run(config);
    sleep_forever();
  } else if (rank == LOADING_TASK_RANK) {
    LoadingTaskS3 lt(REDIS_IP, REDIS_PORT, features_per_sample, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    lt.run(config);
  } else if (rank == ERROR_TASK_RANK) {
    ErrorTask et(REDIS_IP, REDIS_PORT, features_per_sample, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    et.run(config);
    sleep_forever();
  } else if (rank >= WORKERS_BASE && rank < WORKERS_BASE + nworkers) {
    /**
     * Worker tasks run here
     * Number of tasks is determined by the value of nworkers
     */
    LogisticSparseTaskS3 lt(REDIS_IP, REDIS_PORT, features_per_sample, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    lt.run(config, rank - WORKERS_BASE);
    sleep_forever();

  }
  /**
    * SPARSE tasks
    */
  else if (rank == ERROR_SPARSE_TASK_RANK) {
    ErrorSparseTask et(REDIS_IP, REDIS_PORT, (1 << CRITEO_HASH_BITS) + 14, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    et.run(config);
    sleep_forever();
  } else if (rank == LOADING_SPARSE_TASK_RANK) {
    LoadingSparseTaskS3 lt(REDIS_IP, REDIS_PORT, (1 << CRITEO_HASH_BITS) + 14, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    lt.run(config);
  } else if (rank == PS_SPARSE_TASK_RANK) {
    PSSparseTask pt(REDIS_IP, REDIS_PORT, (1 << CRITEO_HASH_BITS) + 14, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    pt.run(config);
    sleep_forever();
  } else if (rank == WORKER_SPARSE_TASK_RANK) {
    LogisticSparseTaskS3 lt(REDIS_IP, REDIS_PORT, (1 << CRITEO_HASH_BITS) + 14, MODEL_BASE,
        LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
        batch_size, samples_per_batch, features_per_sample,
        nworkers, rank);
    lt.run(config, rank - WORKERS_BASE);
    sleep_forever();
  } else {
    throw std::runtime_error("Wrong task rank: " + std::to_string(rank));
  }
}

void print_arguments() {
  // nworkers is the number of processes computing gradients
  // rank starts at 0
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

void print_hostname() {
  char name[200];
  gethostname(name, 200);
  std::cout << "MPI multi task test running on hostname: " << name
    << std::endl;
}

int main(int argc, char** argv) {
  std::cout << "Starting parameter server" << std::endl;

  if (argc != 4) {
    print_arguments();
    throw std::runtime_error("Wrong number of arguments");
  }

  print_hostname();

  int nworkers = string_to<int>(argv[2]);
  std::cout << "Running parameter server with: "
    << nworkers << " workers"
    << std::endl;

  int rank = string_to<int>(argv[3]);
  std::cout << "Running parameter server with: "
    << rank << " rank"
    << std::endl;

  auto config = load_configuration(argv[1]);
  config.print();

  // from config we get
  int batch_size = config.get_minibatch_size() * config.get_num_features();

  std::cout
    << "samples_per_batch: " << config.get_minibatch_size()
    << " features_per_sample: " << config.get_num_features()
    << " batch_size: " << config.get_minibatch_size()
    << std::endl;

  // call the right task for this process
  std::cout << "Running task" << std::endl;
  run_tasks(rank, nworkers, batch_size, config);

  std::cout << "Test successful" << std::endl;

  return 0;
}

