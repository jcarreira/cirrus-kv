#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "config.h"
#include "S3SparseIterator.h"
#include "Utils.h"
#include "async.h"
//#include "adapters/libevent.h"
#include "SparseLRModel.h"
#include "PSSparseServerInterface.h"
#include "PSSparseServerInterfaceWrapper.h"


#define DEBUG
#define ERROR_INTERVAL_USEC (100000) // time between error checks

/**
  * Ugly but necessary for now
  * both onMessage and ErrorSparseTask need access to this
  */
namespace ErrorSparseTaskGlobal {
  std::mutex model_lock;
  std::unique_ptr<SparseLRModel> model;
  std::unique_ptr<lr_model_deserializer> lmd;
  volatile bool model_is_here = true;
  uint64_t start_time;
  std::mutex mp_start_lock;
}

SparseLRModel get_model() {
  static PSSparseServerInterfaceWrapper* psi;
  static bool first_time = true;
  if (first_time) {
    first_time = false;
    psi = new PSSparseServerInterfaceWrapper(PS_IP, PS_PORT, NUM_PS);
  }

  return psi->get_full_model();
}

void ErrorSparseTask::run(const Configuration& config) {
  std::cout << "Compute error task connecting to store" << std::endl;

  // declare serializers
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);

  std::cout << "[ERROR_TASK] connecting to redis" << std::endl;
  auto redis_con  = redis_connect(REDIS_IP, REDIS_PORT);
  if (redis_con == NULL || redis_con -> err) {
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }

  std::cout << "Creating sequential S3Iterator" << std::endl;
  auto test_range = config.get_test_range();
  S3SparseIterator s3_iter(test_range.first, test_range.second, config,
      config.get_s3_size(), config.get_minibatch_size(), 0, false);

  // get data first
  // what we are going to use as a test set
start:
  std::vector<SparseDataset> minibatches_vec;
  std::cout << "[ERROR_TASK] getting minibatches from "
    << test_range.first << " to " << test_range.second
    << std::endl;
  for (uint64_t i = 0;
      i < (test_range.second - test_range.first) * (config.get_s3_size() / config.get_minibatch_size()); ++i) {
    try {
      std::cout << "Getting object: " << i << std::endl;
      const void* minibatch_data = s3_iter.get_next_fast();
      SparseDataset ds(reinterpret_cast<const char*>(minibatch_data),
          config.get_minibatch_size());
      minibatches_vec.push_back(ds);
    } catch(const cirrus::NoSuchIDException& e) {
      if (i == 0)
        goto start;  // no loss to be computed
      else
        break;  // we looked at all minibatches
    }
  }

  std::cout << "[ERROR_TASK] Got "
    << minibatches_vec.size() << " minibatches"
    << "\n";
  std::cout << "[ERROR_TASK] Building dataset"
    << "\n";

  ErrorSparseTaskGlobal::mp_start_lock.lock();

  wait_for_start(ERROR_SPARSE_TASK_RANK, redis_con, nworkers);
  ErrorSparseTaskGlobal::start_time = get_time_us();

  std::cout << "[ERROR_TASK] Computing accuracies"
    << "\n";
  while (1) {
    usleep(ERROR_INTERVAL_USEC); //

    try {
      // first we get the model
#ifdef DEBUG
      std::cout << "[ERROR_TASK] getting the full model at id: "
        << MODEL_BASE
        << "\n";
#endif
      SparseLRModel model = get_model();

#ifdef DEBUG
      std::cout << "[ERROR_TASK] received the model with id: "
        << MODEL_BASE << "\n";
#endif

      int nb = mp.get_number_batches();
      std::cout
        << "[ERROR_TASK] computing loss."
        << " number_batches: " << nb
        << std::endl;
      FEATURE_TYPE total_loss = 0;
      FEATURE_TYPE total_accuracy = 0;
      uint64_t total_num_samples = 0;
      for (auto& ds : minibatches_vec) {
        std::pair<FEATURE_TYPE, FEATURE_TYPE> ret = model.calc_loss(ds);
        total_loss += ret.first;
        total_accuracy += ret.second;
        total_num_samples += ds.num_samples();
      }
      std::cout
        << "Loss (Total/Avg): " << total_loss << "/" << (total_loss / total_num_samples)
        << " Accuracy: " << (total_accuracy / minibatches_vec.size())
        << " time(us): " << get_time_us()
        << " time from start (sec): "
        << (get_time_us() - ErrorSparseTaskGlobal::start_time) / 1000000.0
        << std::endl;
    } catch(const cirrus::NoSuchIDException& e) {
      std::cout << "run_compute_error_task unknown id" << std::endl;
    }
  }
}
