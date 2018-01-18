#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3SparseIterator.h"
#include "async.h"
#include "adapters/libevent.h"

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
}

#if 0
/**
  * Works as a cache for remote model
  */
class ModelProxy {
  public:
    ModelProxy(auto redis_ip, auto redis_port, uint64_t mgs, auto* redis_lock) :
      redis_ip(redis_ip), redis_port(redis_port),
      redis_lock(redis_lock), base(event_base_new()) {
      LogisticSparseTaskGlobal::mp_start_lock.lock();
      LogisticSparseTaskGlobal::model.reset(new SparseLRModel(mgs));
      LogisticSparseTaskGlobal::lmd.reset(new lr_model_deserializer(mgs));
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "ModelProxy::connectCallback" << std::endl;
      LogisticSparseTaskGlobal::mp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    SparseLRModel get_model() {
#ifdef DEBUG
      std::cout << "ModelProxy waiting for model" << std::endl;
#endif
          
      sem_wait(&LogisticSparseTaskGlobal::new_model_semaphore);
      
#ifdef DEBUG
      std::cout << "received model" << "\n";
#endif

      LogisticSparseTaskGlobal::model_lock.lock();
      SparseLRModel ret = *LogisticSparseTaskGlobal::model;
      LogisticSparseTaskGlobal::model_lock.unlock();
      return ret;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      redisReply *r = reinterpret_cast<redisReply*>(reply);
      if (reply == NULL) return;

      //auto now = get_time_us();
      //std::cout << "Time since last (us): "
      //  << (now - LogisticSparseTaskGlobal::prev_on_msg_time) << "\n";
      //LogisticSparseTaskGlobal::prev_on_msg_time = now;
#ifdef DEBUG
      printf("onMessage\n");
#endif
      if (r->type == REDIS_REPLY_ARRAY) {
        const char* str = r->element[2]->str;
        uint64_t len = r->element[2]->len;

#ifdef DEBUG
        printf("len: %lu\n", len);
#endif

        const char* str0 = r->element[0]->str;
        const char* str1 = r->element[1]->str;

        if (strcmp(str0, "message") == 0 &&
            strcmp(str1, "model") == 0) {
#ifdef DEBUG
          std::cout << "Updating model at time: " << get_time_us()
            << " version: " << LogisticSparseTaskGlobal::model_version
            << std::endl;
#endif
          LogisticSparseTaskGlobal::model_lock.lock();
          *LogisticSparseTaskGlobal::model = LogisticSparseTaskGlobal::lmd->operator()(str, len);
          LogisticSparseTaskGlobal::model_version++;
          LogisticSparseTaskGlobal::model_lock.unlock();
          sem_post(&LogisticSparseTaskGlobal::new_model_semaphore);
        }
      } else {
        std::cout << "Not an array" << std::endl;
      }
    }

    void thread_fn() {
      std::cout << "ModelProxy connecting to redis.." << std::endl;
      redis_lock->lock();
      redisAsyncContext* model_r =
        redis_async_connect(redis_ip.c_str(), redis_port);
      if (!model_r) {
        throw std::runtime_error("ModelProxy::Error connecting to redis");
      }
      std::cout << "ModelProxy::connected to redis.." << model_r << std::endl;

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(model_r, base);
      redis_connect_callback(model_r, connectCallback);
      redis_disconnect_callback(model_r, disconnectCallback);
      redis_subscribe_callback(model_r, onMessage, "model");
      redis_lock->unlock();
      
      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      std::cout << "Starting ModelProxy thread" << std::endl;
      thread = std::make_unique<std::thread>(
          std::bind(&ModelProxy::thread_fn, this));
    }

  private:
    std::string redis_ip;
    int redis_port;

    std::mutex* redis_lock;
    struct event_base *base;

    std::unique_ptr<std::thread> thread;
};
#endif

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

void LogisticSparseTaskS3::push_gradient(auto /*gradient_r*/, LRGradient* lrg) {
  lr_gradient_serializer lgs(MODEL_GRAD_SIZE);

#ifdef DEBUG
  std::cout << "Pushing gradient" << std::endl;
#endif

  uint64_t gradient_size = lgs.size(*lrg);
  auto data = std::unique_ptr<char[]>(new char[gradient_size]);
  lgs.serialize(*lrg, data.get());

#ifdef DEBUG
  std::cout << "Publishing gradients" << std::endl;
#endif

  redis_lock.lock();
  //int status = redisAsyncCommand(gradient_r, NULL, NULL,
  //    "PUBLISH gradients %b", data.get(), gradient_size);
  redisReply* reply = (redisReply*)redisCommand(
      LogisticSparseTaskGlobal::redis_con, "PUBLISH gradients %b", data.get(), gradient_size);
  freeReplyObject(reply);

#ifdef DEBUG
  std::cout << "Published gradients!" << std::endl;
#endif
  redis_lock.unlock();

  //if (status == REDIS_ERR) {
  //  throw std::runtime_error("Error in redisAsyncCommand");
  //}

#ifdef DEBUG
  std::cout << "[WORKER] "
      << "Worker task published gradient"
      << " with version: " << lrg->getVersion()
      << " at time (us): " << get_time_us() << "\n";
#endif
}

/** We unpack each minibatch into samples and labels
  */
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

// get samples and labels data
bool LogisticSparseTaskS3::run_phase1(
    auto& dataset,
    auto& s3_iter) {

  try {
    const void* minibatch = s3_iter.get_next_fast();
#ifdef DEBUG
    std::cout << "[WORKER] "
      << "got s3 data"
      << std::endl;
#endif
    dataset.reset(new SparseDataset(reinterpret_cast<const char*>(minibatch),
          config.get_minibatch_size()));

#ifdef DEBUG
    std::chrono::steady_clock::time_point finish =
      std::chrono::steady_clock::now();
    uint64_t elapsed_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          finish - start).count();
    double bw = 1.0 * batch_size * sizeof(double) /
      elapsed_ns * 1000.0 * 1000 * 1000 / 1024 / 1024;
    std::cout << "[WORKER] Get Sample Elapsed (S3) "
      << " batch size: " << batch_size
      << " ns: " << elapsed_ns
      << " BW (MB/s): " << bw
      << " at time: " << get_time_us()
      //<< " prev_model_version: " << prev_model_version
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
        usleep(50);
        int len_model;
        std::string str_id = std::to_string(MODEL_BASE);
        char* data = redis_binary_get(redis_con, str_id.c_str(), &len_model);
        //char* data = redis_get_numid(redis_con, MODEL_BASE, &len_model);

        if (!data) {
          throw std::runtime_error("Null value returned from redis (does not exist?)");
        }

        std::cout << "Received data from redis len: " << len_model << std::endl;
        LogisticSparseTaskGlobal::model_lock.lock();
        LogisticSparseTaskGlobal::model->loadSerialized(data);
        LogisticSparseTaskGlobal::model_lock.unlock();
        
        free(data);
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

  sem_init(&LogisticSparseTaskGlobal::new_model_semaphore, 0, 0);

  std::cout << "Connecting to redis.." << std::endl;
  redis_lock.lock();
  LogisticSparseTaskGlobal::redis_con = connect_redis();
  redis_lock.unlock();

  SparseModelGet mg(1000000000UL, MODEL_GRAD_SIZE);
  mg.run();
  //std::cout << "Starting ModelProxy" << std::endl;
  //ModelProxy mp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE, &redis_lock);
  //mp.run();
  //LogisticSparseTaskGlobal::mp_start_lock.lock();
  //std::cout << "Started ModelProxy" << std::endl;

  std::cout << "Starting GradientProxy" << std::endl;
  LogisticSparseTaskGradientProxy gp(REDIS_IP, REDIS_PORT, &redis_lock);
  gp.run();
  std::cout << "GradientProxy locking" << std::endl;
  LogisticSparseTaskGlobal::gp_start_lock.lock();
  std::cout << "Started GradientProxy" << std::endl;
  
  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches
    << std::endl;
  wait_for_start(WORKER_TASK_RANK + worker, LogisticSparseTaskGlobal::redis_con, nworkers);

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
#endif

    // compute mini batch gradient
    std::unique_ptr<ModelGradient> gradient;

#ifdef DEBUG
    auto now = get_time_us();
    dataset->check();
    dataset->print_info();
    std::cout << "Computing gradient" << std::endl;
#endif

    LogisticSparseTaskGlobal::model_lock.lock();
    SparseLRModel model = *LogisticSparseTaskGlobal::model;
    LogisticSparseTaskGlobal::model_lock.unlock();
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
      LRGradient* lrg = dynamic_cast<LRGradient*>(gradient.get());
      push_gradient(LogisticSparseTaskGlobal::gradient_r, lrg);
    } catch(...) {
      std::cout << "[WORKER] "
        << "Worker task error doing put of gradient" << "\n";
      exit(-1);
    }
  }
}
