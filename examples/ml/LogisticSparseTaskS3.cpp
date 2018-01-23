#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3SparseIterator.h"
#include "async.h"
//#include "adapters/libevent.h"
#include "PSSparseServerInterface.h"

#include <pthread.h>

//#define DEBUG

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
  std::mutex mp_start_lock;
  std::mutex gp_start_lock;
  std::mutex model_lock;
  std::unique_ptr<SparseLRModel> model;
  volatile int model_version = 0;
  static auto prev_on_msg_time = get_time_us();
  redisAsyncContext* gradient_r;
  sem_t new_model_semaphore;
  redisContext* redis_con;
  PSSparseServerInterface* psint;
}

#if 0
class LogisticSparseTaskGradientProxy {
  public:
    LogisticSparseTaskGradientProxy(
        auto redis_ip, auto redis_port, auto* redis_lock) :
      redis_ip(redis_ip), redis_port(redis_port),
      redis_lock(redis_lock), base(event_base_new()) {
      LogisticSparseTaskGlobal::gp_start_lock.lock();
  }
    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "GradientProxy::connectCallback" << std::endl;
      LogisticSparseTaskGlobal::gp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      std::cout << "LogisticSparseTaskGradientProxy onMessage" << std::endl;
      if (reply == NULL) return;
    }

    void thread_fn() {
      std::cout << "GradientProxy connecting to redis.." << std::endl;
      redis_lock->lock();
      LogisticSparseTaskGlobal::gradient_r =
        redis_async_connect(redis_ip.c_str(), redis_port);
      if (!LogisticSparseTaskGlobal::gradient_r) {
        throw std::runtime_error("GradientProxy::Error connecting to redis");
      }
      std::cout << "GradientProxy connected to redis.." << LogisticSparseTaskGlobal::gradient_r
        << std::endl;

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(LogisticSparseTaskGlobal::gradient_r, base);
      redis_connect_callback(LogisticSparseTaskGlobal::gradient_r, connectCallback);
      redis_disconnect_callback(LogisticSparseTaskGlobal::gradient_r, disconnectCallback);
      redis_lock->unlock();

      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&LogisticSparseTaskGradientProxy::thread_fn, this));
    }
  private:
    std::string redis_ip;
    int redis_port;
    std::mutex* redis_lock;
    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};
#endif

void LogisticSparseTaskS3::push_gradient(auto /*gradient_r*/, LRSparseGradient* lrg) {
#ifdef DEBUG
  auto before_push_us = get_time_us();
  std::cout << "Pushing gradient" << std::endl;
#endif

//  uint64_t gradient_size = lrg->getSerializedSize();
//  std::shared_ptr<char> data = std::shared_ptr<char>(new char[gradient_size],
//      std::default_delete<char[]>());
//  lrg->serialize(data.get());
#ifdef DEBUG
  auto after_1 = get_time_us();
#endif

#ifdef DEBUG
  std::cout << "Publishing gradients" << std::endl;
#endif

//  redis_lock.lock();
//  redisReply* reply = (redisReply*)redisCommand(
//      LogisticSparseTaskGlobal::redis_con, "PUBLISH gradients %b", data.get(), gradient_size);
#ifdef DEBUG
  auto after_2 = get_time_us();
#endif
//  freeReplyObject(reply);
//  redis_lock.unlock();
  LogisticSparseTaskGlobal::psint->send_gradient(*lrg);

#ifdef DEBUG
  std::cout << "Published gradients!" << std::endl;
#endif


#ifdef DEBUG
  auto elapsed_push_us = get_time_us() - before_push_us;
  static uint64_t before = 0;
  if (before == 0)
    before = get_time_us();
  auto now = get_time_us();
  std::cout << "[WORKER] "
      << "Worker task published gradient"
      << " with version: " << lrg->getVersion()
      << " at time (us): " << get_time_us()
      << " part1 (us): " << (after_1 - before_push_us)
      << " part2 (us): " << (after_2 - after_1)
      << " took(us): " << elapsed_push_us
      << " bw(MB/s): " << std::fixed << (1.0 * gradient_size / elapsed_push_us / 1024 / 1024 * 1000 * 1000)
      << " since last(us): " << (now - before)
      << "\n";
  before = now;
#endif
}

/** We unpack each minibatch into samples and labels
  */
#if 0
void LogisticSparseTaskS3::unpack_minibatch(
    std::shared_ptr<FEATURE_TYPE> minibatch,
    auto& samples, auto& labels) {
  uint64_t num_samples_per_batch = batch_size / features_per_sample;

  samples = std::shared_ptr<FEATURE_TYPE>(
      new FEATURE_TYPE[batch_size], std::default_delete<FEATURE_TYPE[]>());
  labels = std::shared_ptr<FEATURE_TYPE>(
      new FEATURE_TYPE[num_samples_per_batch], std::default_delete<FEATURE_TYPE[]>());

  for (uint64_t j = 0; j < num_samples_per_batch; ++j) {
    FEATURE_TYPE* data = minibatch.get() + j * (features_per_sample + 1);
    labels.get()[j] = *data;

    if (!FLOAT_EQ(*data, 1.0) && !FLOAT_EQ(*data, 0.0))
      throw std::runtime_error(
          "Wrong label in unpack_minibatch " + std::to_string(*data));

    data++;
    std::copy(data,
        data + features_per_sample,
        samples.get() + j * features_per_sample);
  }
}
#endif

// get samples and labels data
bool LogisticSparseTaskS3::run_phase1(
    auto& dataset,
    auto& s3_iter) {

  try {
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
  } catch(const cirrus::NoSuchIDException& e) {
    // this happens because the ps task
    // has not uploaded the model yet
    std::cout << "[WORKER] "
      << "Model could not be found at id: " << MODEL_BASE << "\n";
    return false;
  }

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

class SparseModelGet {
  public:
    SparseModelGet(auto MODEL_BASE, auto MODEL_GRAD_SIZE) : MODEL_BASE(MODEL_BASE), MODEL_GRAD_SIZE(MODEL_GRAD_SIZE) {}

    void thread_fn() {
      //uint64_t count = 0;
      while (1) {
        int len_model;
        std::string str_id = std::to_string(MODEL_BASE);
#ifdef DEBUG
        auto before_us = get_time_us();
#endif
        char* data = redis_binary_get(redis_con, str_id.c_str(), &len_model);
#ifdef DEBUG
        auto elapsed_us = get_time_us() - before_us;
        std::cout
          << "Get model elapsed (us): " << elapsed_us
          << " bw (MB/s): " << (1.0 * len_model / elapsed_us * 1000 * 1000 / 1024 / 1024 )
          << "\n";
#endif

        if (!data) {
          throw std::runtime_error(
              "Null value returned from redis model (does not exist?)");
        }

        //std::cout << "Received data from redis len: " << len_model << "\n";
        LogisticSparseTaskGlobal::model_lock.lock();
        LogisticSparseTaskGlobal::model->loadSerialized(data);
        LogisticSparseTaskGlobal::model_lock.unlock();
        
        free(data);
        usleep(50);
      }
    }

    void run() {
      LogisticSparseTaskGlobal::model.reset(new SparseLRModel(MODEL_GRAD_SIZE));
      redis_con = connect_redis();
      thread = std::make_unique<std::thread>(
          std::bind(&SparseModelGet::thread_fn, this));
    }
  private:
    uint64_t MODEL_BASE;
    uint64_t MODEL_GRAD_SIZE;
    redisContext* redis_con;
    std::unique_ptr<std::thread> thread;
};


void LogisticSparseTaskS3::run(const Configuration& config, int worker) {
  std::cout << "Starting LogisticSparseTaskS3"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();
  this->config = config;

  LogisticSparseTaskGlobal::psint = new PSSparseServerInterface("172.31.0.28", 1337);

  sem_init(&LogisticSparseTaskGlobal::new_model_semaphore, 0, 0);

  std::cout << "Connecting to redis.." << std::endl;
  redis_lock.lock();
  LogisticSparseTaskGlobal::redis_con = connect_redis();
  redis_lock.unlock();

  uint64_t MODEL_BASE = (1000000000ULL);
  SparseModelGet mg(MODEL_BASE, MODEL_GRAD_SIZE);
  mg.run();
  //std::cout << "Starting ModelProxy" << std::endl;
  //ModelProxy mp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE, &redis_lock);
  //mp.run();
  //LogisticSparseTaskGlobal::mp_start_lock.lock();
  //std::cout << "Started ModelProxy" << std::endl;

  //std::cout << "Starting GradientProxy" << std::endl;
  //LogisticSparseTaskGradientProxy gp(REDIS_IP, REDIS_PORT, &redis_lock);
  //gp.run();
  //std::cout << "GradientProxy locking" << std::endl;
  //LogisticSparseTaskGlobal::gp_start_lock.lock();
  //std::cout << "Started GradientProxy" << std::endl;
  
  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches
    << std::endl;
  wait_for_start(WORKER_SPARSE_TASK_RANK + worker, LogisticSparseTaskGlobal::redis_con, nworkers);

  // Create iterator that goes from 0 to num_s3_batches
  S3SparseIterator s3_iter(0, num_s3_batches, config,
      config.get_s3_size(), config.get_minibatch_size());

  std::cout << "[WORKER] starting loop" << std::endl;

  uint64_t version = 1;
  SparseLRModel model(MODEL_GRAD_SIZE);

  while (1) {
    // get data, labels and model
#ifdef DEBUG
    std::cout << "[WORKER] running phase 1" << std::endl;
#endif
    std::unique_ptr<SparseDataset> dataset;
    if (!run_phase1(dataset, s3_iter)) {
      continue;
    }
#ifdef DEBUG
    std::cout << "[WORKER] phase 1 done" << std::endl;
    //dataset->check();
    //dataset->print_info();
#endif
    // compute mini batch gradient
    std::unique_ptr<ModelGradient> gradient;

    LogisticSparseTaskGlobal::model_lock.lock();
    SparseLRModel model = *LogisticSparseTaskGlobal::model;
    LogisticSparseTaskGlobal::model_lock.unlock();

#ifdef DEBUG
    std::cout << "Checking model" << std::endl;
    //model.check();
    std::cout << "Computing gradient" << "\n";
    auto now = get_time_us();
#endif

    try {
      gradient = model.minibatch_grad(*dataset, config.get_epsilon());
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
      push_gradient(LogisticSparseTaskGlobal::gradient_r, lrg);
    } catch(...) {
      std::cout << "[WORKER] "
        << "Worker task error doing put of gradient" << "\n";
      exit(-1);
    }
  }
}

