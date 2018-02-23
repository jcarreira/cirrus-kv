#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3SparseIterator.h"
#include "async.h"
#include "PSSparseServerInterface.h"
#include "SparseMFModel.h"

#include <pthread.h>

#undef DEBUG

class MFModelGet {
  public:
    MFModelGet(const std::string& ps_ip, int ps_port) :
      ps_ip(ps_ip), ps_port(ps_port) {
      psi = std::make_unique<PSSparseServerInterface>(ps_ip, ps_port);
    }

    SparseMFModel get_new_model(
        const SparseDataset& ds, uint64_t user_base_index, uint64_t mb_size) {
      return psi->get_sparse_mf_model(ds, user_base_index, mb_size);
    }

  private:
    std::unique_ptr<PSSparseServerInterface> psi;
    std::string ps_ip;
    int ps_port;
};

void check_redis(auto r) {
  if (r == NULL || r -> err) {
    std::cout << "[WORKER] "
      << "Error connecting to REDIS"
      << " IP: " << REDIS_IP
      << std::endl;
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
}

// we need global variables because of the static callbacks
namespace MFNetflixTaskTaskGlobal {
  std::unique_ptr<MFModelGet> mf_model_get;
  redisContext* redis_con;
  PSSparseServerInterface* psint;
}

void MFNetflixTask::push_gradient(MFSparseGradient& mfg) {
#ifdef DEBUG
  auto before_push_us = get_time_us();
  std::cout << "Publishing gradients" << std::endl;
#endif
  MFNetflixTaskTaskGlobal::psint->send_mf_gradient(mfg);
#ifdef DEBUG
  std::cout << "Published gradients!" << std::endl;
  auto elapsed_push_us = get_time_us() - before_push_us;
  static uint64_t before = 0;
  if (before == 0)
    before = get_time_us();
  auto now = get_time_us();
  std::cout << "[WORKER] "
      << "Worker task published gradient"
      << " with version: " << lrg->getVersion()
      << " at time (us): " << get_time_us()
      << " took(us): " << elapsed_push_us
      << " bw(MB/s): " << std::fixed <<
         (1.0 * lrg->getSerializedSize() / elapsed_push_us / 1024 / 1024 * 1000 * 1000)
      << " since last(us): " << (now - before)
      << "\n";
  before = now;
#endif
}

// get samples and labels data
bool MFNetflixTask::get_dataset_minibatch(
    auto& dataset,
    auto& s3_iter) {
#ifdef DEBUG
  auto start = get_time_us();
#endif

  const void* minibatch = s3_iter.get_next_fast();
#ifdef DEBUG
  auto finish1 = get_time_us();
#endif
  dataset.reset(new SparseDataset(reinterpret_cast<const char*>(minibatch),
        config.get_minibatch_size())); // this takes 11 us

#ifdef DEBUG
  auto finish2 = get_time_us();
  double bw = 1.0 * dataset->getSizeBytes() /
    (finish2-start) * 1000.0 * 1000 / 1024 / 1024;
  std::cout << "[WORKER] Get Sample Elapsed (S3) "
    << " minibatch size: " << config.get_minibatch_size()
    << " part1(us): " << (finish1 - start)
    << " part2(us): " << (finish2 - finish1)
    << " BW (MB/s): " << bw
    << " at time: " << get_time_us()
    << "\n";
#endif
  return true;
}

static auto connect_redis() {
  std::cout << "[WORKER] "
    << "Worker task connecting to REDIS. "
    << "IP: " << REDIS_IP << std::endl;
  auto redis_con = redis_connect(REDIS_IP, REDIS_PORT);
  check_redis(redis_con);
  return redis_con;
}

void MFNetflixTask::run(const Configuration& config, int worker) {
  std::cout << "Starting MFNetflixTask"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();
  this->config = config;

  MFNetflixTaskTaskGlobal::psint = new PSSparseServerInterface(PS_IP, PS_PORT);

  std::cout << "Connecting to redis.." << std::endl;
  redis_lock.lock();
  MFNetflixTaskTaskGlobal::redis_con = connect_redis();
  redis_lock.unlock();

  //uint64_t MODEL_BASE = (1000000000ULL);
  MFNetflixTaskTaskGlobal::mf_model_get =
    std::make_unique<MFModelGet>(PS_IP, PS_PORT);
  
  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches
    << std::endl;
  wait_for_start(WORKER_SPARSE_TASK_RANK + worker, MFNetflixTaskTaskGlobal::redis_con, nworkers);

  // Create iterator that goes from 0 to num_s3_batches
  auto train_range = config.get_train_range();

  /** We sequentially iterate over data
    * This is necessary because we need to know the index of each row
    * in the dataset matrix because that tells us which user it belongs to
    * (same doesn't happen with Logistic Regression)
    */
  S3SparseIterator s3_iter(
      train_range.first, train_range.second,
      config, config.get_s3_size(), config.get_minibatch_size(),
      worker, false);

  std::cout << "[WORKER] starting loop" << std::endl;

  uint64_t sample_index = 0;
  while (1) {
    // get data, labels and model
#ifdef DEBUG
    std::cout << "[WORKER] running phase 1" << std::endl;
#endif
    std::unique_ptr<SparseDataset> dataset;
    if (!get_dataset_minibatch(dataset, s3_iter)) {
      continue;
    }
#ifdef DEBUG
    std::cout << "[WORKER] phase 1 done" << std::endl;
    //dataset->check();
    //dataset->print_info();
    auto now = get_time_us();
#endif
    // compute mini batch gradient
    std::unique_ptr<ModelGradient> gradient;

    // we get the model subset with just the right amount of weights
    SparseMFModel model =
      MFNetflixTaskTaskGlobal::mf_model_get->get_new_model(
          *dataset, sample_index, config.get_minibatch_size());

#ifdef DEBUG
    std::cout << "get model elapsed(us): " << get_time_us() - now << std::endl;
    std::cout << "Checking model" << std::endl;
    //model.check();
    std::cout << "Computing gradient" << "\n";
    now = get_time_us();
#endif

    try {
      auto gradient = model.minibatch_grad(*dataset, config, sample_index);
    } catch(...) {
      std::cout << "There was an error computing the gradient" << std::endl;
      exit(-1);
    }
#ifdef DEBUG
    auto elapsed_us = get_time_us() - now;
    std::cout << "[WORKER] Gradient compute time (us): " << elapsed_us
      << " at time: " << get_time_us()
      << " version " << version << "\n";
#endif

    push_gradient(*dynamic_cast<MFSparseGradient*>(gradient.get()));
    sample_index += config.get_minibatch_size();
  }
}

