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

#undef DEBUG
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
  static PSSparseServerInterface* psi;
  static bool first_time = true;
  if (first_time) {
    first_time = false;
    psi = new PSSparseServerInterface(PS_IP, PS_PORT);
  }

  return psi->get_full_model();
}

static double log_aux(double v) {
  if (v == 0) {
    return -10000;
  }

  double res = log(v);
  if (std::isnan(res) || std::isinf(res)) {
    throw std::runtime_error(
        std::string("log_aux generated nan/inf v: ") +
        std::to_string(v));
  }
  return res;
}


static float s_1_float(float x) {
  float res = 1.0 / (1.0 + exp(-x));
  if (std::isnan(res) || std::isinf(res)) {
    throw std::runtime_error(
        std::string("s_1_float generated nan/inf x: " + std::to_string(x)
          + " res: " + std::to_string(res)));
  }

  return res;
}

float predict(const SparseLRModel& model, const std::vector<std::pair<int, FEATURE_TYPE>>& sample) {
  float res = 0;
  for (const auto& v : sample) {
    int index = v.first;
    int value = v.second;
    res += model.get_nth_weight(index) * value;
#ifdef DEBUG
    std::cout 
      << "res: " << res
      << " index: " << index
      << " value: " << value
      << " model v: " << model.get_nth_weight(index)
      << std::endl;
#endif
  }
  return res;
}

float compute_loss(SparseLRModel& model, std::vector<SparseDataset>& test_datasets, uint64_t& num_samples) {
  float res_loss = 0;
  for (uint64_t j = 0; j < test_datasets.size(); ++j) {
    const SparseDataset& test_dataset = test_datasets[j];
    for (uint64_t i = 0; i < test_dataset.num_samples(); ++i) {
      const std::vector<std::pair<int, FEATURE_TYPE>>& sample =
        test_dataset.get_row(i);
      float prediction = predict(model, sample);
      if (std::isinf(prediction) || std::isnan(prediction)) {
        throw std::runtime_error("Invalid value");
      }
      float s1 = s_1_float(prediction);
      float label = test_dataset.get_label(i);

      if (label == -1) {
        label = 0;
      }

      float loss = label * log_aux(s1) + (1 - label) * log_aux(1 - s1);
      res_loss += loss;
      num_samples++;
    }
  }
  return res_loss;
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

      total_loss = compute_loss(model, minibatches_vec, total_num_samples);

      //for (auto& ds : minibatches_vec) {
      //  std::pair<FEATURE_TYPE, FEATURE_TYPE> ret = model.calc_loss(ds);
      //  total_loss += ret.first;
      //  total_accuracy += ret.second;
      //  total_num_samples += ds.num_samples();
      //}
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
