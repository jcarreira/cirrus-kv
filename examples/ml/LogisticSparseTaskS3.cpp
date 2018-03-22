#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3SparseIterator.h"
#include "async.h"
#include "PSSparseServerInterface.h"

#include <pthread.h>

#undef DEBUG



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
namespace LogisticSparseTaskGlobal {
  redisContext* redis_con;
  PSSparseServerInterface* psint[2];
}

void LogisticSparseTaskS3::push_gradient(LRSparseGradient* lrg) {
#ifdef DEBUG
  auto before_push_us = get_time_us();
  std::cout << "Publishing gradients" << std::endl;
#endif
  LRSparseGradient** split_model = lrg->shard();
  LogisticSparseTaskGlobal::psint[0]->send_gradient(*split_model[0]);
  LogisticSparseTaskGlobal::psint[1]->send_gradient(*split_model[1]);
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
bool LogisticSparseTaskS3::get_dataset_minibatch(
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

void LogisticSparseTaskS3::run(const Configuration& config, int worker) {
  std::cout << "Starting LogisticSparseTaskS3"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();
  this->config = config;

  LogisticSparseTaskGlobal::psint[0] = new PSSparseServerInterface(PS_IP, PS_PORT);
  LogisticSparseTaskGlobal::psint[1] = new PSSparseServerInterface(PS_IP, PS_PORT+1);

  std::cout << "Connecting to redis.." << std::endl;
  redis_lock.lock();
  LogisticSparseTaskGlobal::redis_con = connect_redis();
  redis_lock.unlock();

  //uint64_t MODEL_BASE = (1000000000ULL);

  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches
    << std::endl;
  wait_for_start(WORKER_SPARSE_TASK_RANK + worker, LogisticSparseTaskGlobal::redis_con, nworkers);

  // Create iterator that goes from 0 to num_s3_batches
  auto train_range = config.get_train_range();
  S3SparseIterator s3_iter(
      train_range.first, train_range.second,
      config, config.get_s3_size(), config.get_minibatch_size(),
      worker);

  std::cout << "[WORKER] starting loop" << std::endl;

  uint64_t version = 1;
  SparseLRModel model(MODEL_GRAD_SIZE);

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

    SparseLRModel model(0);
    // we get the model subset with just the right amount of weights
    for (int i = 0; i < 2; i++) {
      LogisticSparseTaskGlobal::psint[i]->get_sparse_model(*dataset, i, 2, model);
    }


#ifdef DEBUG
    std::cout << "get model elapsed(us): " << get_time_us() - now << std::endl;
    std::cout << "Checking model" << std::endl;
    //model.check();
    std::cout << "Computing gradient" << "\n";
    now = get_time_us();
#endif

    try {
      gradient = model.minibatch_grad_sparse(*dataset, config.get_epsilon());
    } catch(const std::runtime_error& e) {
      std::cout << "Error. " << e.what() << std::endl;
      exit(-1);
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
    gradient->setVersion(version++);

    try {
      LRSparseGradient* lrg = dynamic_cast<LRSparseGradient*>(gradient.get());
      push_gradient(lrg);
    } catch(...) {
      std::cout << "[WORKER] "
        << "Worker task error doing put of gradient" << "\n";
      exit(-1);
    }
  }
}
