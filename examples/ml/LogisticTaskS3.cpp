#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3Iterator.h"
#include "async.h"
#include "adapters/libevent.h"

// #define DEBUG

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
namespace LogisticTaskS3Global {
  std::mutex mp_start_lock;
  std::mutex gp_start_lock;
  std::mutex model_lock;
  std::unique_ptr<LRModel> model;
  std::unique_ptr<lr_model_deserializer> lmd;
  volatile bool first_time = true;
  volatile int model_version = 0;
  static auto prev_on_msg_time = get_time_us();
  redisAsyncContext* gradient_r;
}

/**
  * Works as a cache for remote model
  */
class ModelProxy {
  public:
    ModelProxy(auto redis_ip, auto redis_port, uint64_t mgs, auto* redis_lock) :
      redis_ip(redis_ip), redis_port(redis_port),
      redis_lock(redis_lock), base(event_base_new()) {
    LogisticTaskS3Global::mp_start_lock.lock();
    LogisticTaskS3Global::model.reset(new LRModel(mgs));
    LogisticTaskS3Global::lmd.reset(new lr_model_deserializer(mgs));
  }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "ModelProxy::connectCallback" << std::endl;
      LogisticTaskS3Global::mp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    LRModel get_model() {
#ifdef DEBUG
      std::cout << "ModelProxy waiting for model" << std::endl;
#endif
      // make sure we receive model at least once
      while (LogisticTaskS3Global::first_time == true) {
      }
#ifdef DEBUG
      std::cout << "ModelProxy model is here" << std::endl;
#endif

      LogisticTaskS3Global::model_lock.lock();
      LRModel ret = *LogisticTaskS3Global::model;
      LogisticTaskS3Global::model_lock.unlock();
      return ret;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      redisReply *r = reinterpret_cast<redisReply*>(reply);
      if (reply == NULL) return;

      auto now = get_time_us();
      std::cout << "Time since last (us): "
        << (now - LogisticTaskS3Global::prev_on_msg_time) << std::endl;
      LogisticTaskS3Global::prev_on_msg_time = now;
#ifdef DEBUG
      printf("onMessage\n");
#endif
      if (r->type == REDIS_REPLY_ARRAY) {
        const char* str = r->element[2]->str;
        uint64_t len = r->element[2]->len;

#ifdef DEBUG
        printf("len: %lu\n", len);
#endif

        // XXX fix this
        if (len > 100) {
#ifdef DEBUG
          std::cout << "Updating model at time: " << get_time_us()
            << " version: " << LogisticTaskS3Global::model_version
            << std::endl;
#endif
          LogisticTaskS3Global::model_lock.lock();
          *LogisticTaskS3Global::model = LogisticTaskS3Global::lmd->operator()(str, len);
          LogisticTaskS3Global::model_version++;
          LogisticTaskS3Global::model_lock.unlock();
          LogisticTaskS3Global::first_time = false;
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

class LogisticTaskGradientProxy {
  public:
    LogisticTaskGradientProxy(
        auto redis_ip, auto redis_port, auto* redis_lock) :
      redis_ip(redis_ip), redis_port(redis_port),
      redis_lock(redis_lock), base(event_base_new()) {
        LogisticTaskS3Global::gp_start_lock.lock();
  }
    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "GradientProxy::connectCallback" << std::endl;
      LogisticTaskS3Global::gp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      std::cout << "LogisticTaskGradientProxy onMessage" << std::endl;
      if (reply == NULL) return;
    }

    void thread_fn() {
      std::cout << "GradientProxy connecting to redis.." << std::endl;
      redis_lock->lock();
      LogisticTaskS3Global::gradient_r =
        redis_async_connect(redis_ip.c_str(), redis_port);
      if (!LogisticTaskS3Global::gradient_r) {
        throw std::runtime_error("GradientProxy::Error connecting to redis");
      }
      std::cout << "GradientProxy connected to redis.." << LogisticTaskS3Global::gradient_r
        << std::endl;

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(LogisticTaskS3Global::gradient_r, base);
      redis_connect_callback(LogisticTaskS3Global::gradient_r, connectCallback);
      redis_disconnect_callback(LogisticTaskS3Global::gradient_r, disconnectCallback);
      redis_lock->unlock();

      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&LogisticTaskGradientProxy::thread_fn, this));
    }
  private:
    std::string redis_ip;
    int redis_port;
    std::mutex* redis_lock;
    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};

void LogisticTaskS3::push_gradient(
    auto gradient_r, int worker, LRGradient* lrg) {
  lr_gradient_serializer lgs(MODEL_GRAD_SIZE);

#ifdef DEBUG
  std::cout << "Pushing gradient" << std::endl;
#endif

  uint64_t gradient_size = lgs.size(*lrg);
  auto data = std::unique_ptr<char[]>(new char[gradient_size]);
  lgs.serialize(*lrg, data.get());

  int gradient_id = GRADIENT_BASE + worker;

#ifdef DEBUG
  std::cout << "Publishing gradients" << std::endl;
#endif

  redis_lock.lock();
  int status = redisAsyncCommand(gradient_r, NULL, NULL,
      "PUBLISH gradients %b", data.get(), gradient_size);
#ifdef DEBUG
  std::cout << "Published gradients!" << std::endl;
#endif
  redis_lock.unlock();

  if (status == REDIS_ERR) {
    throw std::runtime_error("Error in redisAsyncCommand");
  }

  std::cout << "[WORKER] "
      << "Worker task stored gradient at id: " << gradient_id
      << " with version: " << lrg->getVersion()
      << " at time (us): " << get_time_us() << "\n";
}

/** We unpack each minibatch into samples and labels
  */
void LogisticTaskS3::unpack_minibatch(
    std::shared_ptr<double> minibatch,
    auto& samples, auto& labels) {
  uint64_t num_samples_per_batch = batch_size / features_per_sample;

  samples = std::shared_ptr<double>(
      new double[batch_size], std::default_delete<double[]>());
  labels = std::shared_ptr<double>(
      new double[num_samples_per_batch], std::default_delete<double[]>());

  for (uint64_t j = 0; j < num_samples_per_batch; ++j) {
    double* data = minibatch.get() + j * (features_per_sample + 1);
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
bool LogisticTaskS3::run_phase1(
    auto& samples, auto& labels, auto& model,
    auto& s3_iter, auto& mp, auto& prev_model_version) {

  try {
#ifdef DEBUG
    auto before_model = get_time_ns();
#endif
try_again:
    model = mp.get_model();
    if (LogisticTaskS3Global::model_version != prev_model_version) {
      prev_model_version = LogisticTaskS3Global::model_version;
#ifdef DEBUG
      std::cout << "new model."
        << " prev_model_version: " << prev_model_version
        << " model_version: " << LogisticTaskS3Global::model_version
        << std::endl;
#endif
    } else {
#ifdef DEBUG
      std::cout << "model is repeated."
        << " prev_model_version: " << prev_model_version
        << " model_version: " << LogisticTaskS3Global::model_version
        << std::endl;
#endif
      goto try_again;
    }

#ifdef DEBUG
    auto after_model = get_time_ns();
    std::cout << "[WORKER] "
      << "model get (ns): " << (after_model - before_model)
      << std::endl;
    std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();
#endif


    std::shared_ptr<double> minibatch = s3_iter.get_next();
#ifdef DEBUG
    std::cout << "[WORKER] got s3 data" << std::endl;
#endif
    unpack_minibatch(minibatch, samples, labels);

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
      << " prev_model_version: " << prev_model_version
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

auto connect_redis() {
  std::cout << "[WORKER] "
    << "Worker task connecting to REDIS. "
    << "IP: " << REDIS_IP << std::endl;
  auto r = redis_connect(REDIS_IP, REDIS_PORT);
  check_redis(r);
  return r;
}

void LogisticTaskS3::run(const Configuration& config, int worker) {
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();

  std::cout << "Connecting to redis.." << std::endl;
  redis_lock.lock();
  auto redis_con = connect_redis();
  redis_lock.unlock();

  std::cout << "Starting ModelProxy" << std::endl;
  ModelProxy mp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE, &redis_lock);
  mp.run();
  LogisticTaskS3Global::mp_start_lock.lock();
  std::cout << "Started ModelProxy" << std::endl;

  std::cout << "Starting GradientProxy" << std::endl;
  LogisticTaskGradientProxy gp(REDIS_IP, REDIS_PORT, &redis_lock);
  gp.run();
  LogisticTaskS3Global::gp_start_lock.lock();
  std::cout << "Started GradientProxy" << std::endl;
  
  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches
    << std::endl;
  wait_for_start(WORKER_TASK_RANK + worker, redis_con, nworkers);

  // Create iterator that goes from 0 to num_s3_batches
  S3Iterator s3_iter(0, num_s3_batches, config,
      config.get_s3_size(), features_per_sample,
      config.get_minibatch_size());

  std::cout << "[WORKER] starting loop" << std::endl;

  uint64_t version = 1;
  LRModel model(MODEL_GRAD_SIZE);
  int prev_model_version = -1;

  while (1) {
    // maybe we can wait a few iterations to get the model
    std::shared_ptr<double> samples;
    std::shared_ptr<double> labels;

    // get data, labels and model
#ifdef DEBUG
    std::cout << "[WORKER] running phase 1" << std::endl;
#endif
    if (!run_phase1(samples, labels, model, s3_iter, mp, prev_model_version)) {
      continue;
    }
#ifdef DEBUG
    std::cout << "[WORKER] phase 1 done" << std::endl;
#endif

    Dataset dataset(samples.get(), labels.get(),
        samples_per_batch, features_per_sample);

    // compute mini batch gradient
    std::unique_ptr<ModelGradient> gradient;

#ifdef DEBUG
    auto now = get_time_us();
    dataset.check();
    dataset.print_info();
    std::cout << "Computing gradient" << std::endl;
#endif
    try {
      gradient = model.minibatch_grad(dataset.samples_,
          labels.get(), samples_per_batch, config.get_epsilon());
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
      push_gradient(LogisticTaskS3Global::gradient_r, worker, lrg);
    } catch(...) {
      std::cout << "[WORKER] "
        << "Worker task error doing put of gradient" << "\n";
      exit(-1);
    }
  }
}

