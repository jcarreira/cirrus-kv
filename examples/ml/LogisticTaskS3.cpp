#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3Iterator.h"
#include "async.h"
#include "adapters/libevent.h"

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


/**
  * Ugly but necessary for now
  * both onMessage and LogisticTaskS3 need access to this
  */
std::mutex mp_start_lock;
std::mutex model_lock;
std::unique_ptr<LRModel> model;
std::unique_ptr<lr_model_deserializer> lmd;
volatile bool first_time = true;

/**
  * Works as a cache for remote model
  */
class ModelProxy {
  public:
    ModelProxy(auto redis_ip, auto redis_port, uint64_t mgs, auto* redis_lock) :
      redis_ip(redis_ip), redis_port(redis_port),
      redis_lock(redis_lock), base(event_base_new())
  { 
    mp_start_lock.lock();
    model.reset(new LRModel(mgs));
    lmd.reset(new lr_model_deserializer(mgs));
  }

    ~ModelProxy() {
      thread_terminate = true;
    }
    
    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "connectCallback" << std::endl;
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    LRModel get_model() {
      std::cout << "get_model" << std::endl;
      // make sure we receive model at least once
      while (first_time == true) {
      }
      std::cout << "received model" << std::endl;

      model_lock.lock();
      LRModel ret = *model;
      model_lock.unlock();
      return ret;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      redisReply *r = (redisReply*)reply;
      if (reply == NULL) return;

      printf("onMessage\n");
      if (r->type == REDIS_REPLY_ARRAY) {
        const char* str = r->element[2]->str;
        uint64_t len = r->element[2]->len;

        printf("len: %lu\n", len);

        const char* str0 = r->element[0]->str;
        uint64_t len0 = r->element[0]->len;
        const char* str1 = r->element[1]->str;
        uint64_t len1 = r->element[1]->len;

        std::cout << "str0: " << str0
          << " len0: " << len0
          << " str1: " << str1
          << " len1: " << len1
          << std::endl;

        if (strcmp(str0, "message") == 0 &&
            strcmp(str1, "model") == 0) {
          printf("Updating model at time: %lu\n", get_time_us());
          model_lock.lock();
          *model = lmd->operator()(str, len);
          model_lock.unlock();
          printf("Updated model\n");
          first_time = false;
        }
      } else {
        std::cout << "Not an array" << std::endl;
      }
    }

    void thread_fn() {
      std::cout << "connecting to redis.." << std::endl;
      redis_lock->lock();
      redisAsyncContext* model_r = redis_async_connect(redis_ip.c_str(), redis_port);
      std::cout << "connected to redis.." << model_r << std::endl;
      if (!model_r) {
        throw std::runtime_error("Error connecting to redis");
      }
      
      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(model_r, base);
      redis_connect_callback(model_r, connectCallback);
      redis_disconnect_callback(model_r, disconnectCallback);
      redis_subscribe_callback(model_r, onMessage, "model");
      redis_lock->unlock();
      
      mp_start_lock.unlock();
      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&ModelProxy::thread_fn, this));
    }

  private:
    volatile bool thread_terminate = false;

    std::string redis_ip;
    int redis_port;

    std::mutex* redis_lock;
    struct event_base *base;

    std::unique_ptr<std::thread> thread;
};

void LogisticTaskS3::push_gradient(auto r, int worker, LRGradient* lrg) {
  lr_gradient_serializer lgs(MODEL_GRAD_SIZE);

  uint64_t gradient_size = lgs.size(*lrg);
  auto data = std::unique_ptr<char[]>(new char[gradient_size]);
  lgs.serialize(*lrg, data.get());

  int gradient_id = GRADIENT_BASE + worker;
       
  redis_lock.lock(); 
  redis_put_binary_numid(r, gradient_id, data.get(), gradient_size);
  redis_lock.unlock(); 

  std::cout << "[WORKER] "
      << "Worker task stored gradient at id: " << gradient_id
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
    auto& dataset, auto& model,
    auto& s3_iter, uint64_t /*features_per_sample*/, auto& mp) {

  try {
    auto before_model = get_time_ns();
    model = mp.get_model();
    auto after_model = get_time_ns();
    std::cout << "[WORKER] "
      << "model get (ns): " << (after_model - before_model)
      << std::endl;

    std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();

    const double* minibatch = s3_iter.get_next_fast();
    std::cout << "[WORKER] "
      << "got s3 data"
      << std::endl;
    //unpack_minibatch(minibatch, samples, labels);
    dataset.reset(new Dataset(minibatch, samples_per_batch, features_per_sample));

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
      << "at time: " << get_time_us()
      << "\n";
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

  std::cout << "Starting ModelProxy" << std::endl;
  ModelProxy mp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE, &redis_lock);
  mp.run();
  mp_start_lock.lock();
  std::cout << "Started ModelProxy" << std::endl;

  // we use redis
  std::cout << "Connecting to redis.." << std::endl;
  redis_lock.lock();
  auto r = connect_redis();
  redis_lock.unlock();

  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches << std::endl;
  wait_for_start(WORKER_TASK_RANK + worker, r, nworkers);
  
  // Create iterator that goes from 0 to num_s3_batches
  S3Iterator s3_iter(0, num_s3_batches, config,
      config.get_s3_size(), features_per_sample,
      config.get_minibatch_size());
  
  std::cout << "[WORKER] starting loop" << std::endl;
  
  uint64_t version = 0;
  while (1) {
    // maybe we can wait a few iterations to get the model
    //std::shared_ptr<double> samples;
    //std::shared_ptr<double> labels;
    LRModel model(MODEL_GRAD_SIZE);

    // get data, labels and model
    std::cout << "[WORKER] running phase 1" << std::endl;
    std::unique_ptr<Dataset> dataset;
    if (!run_phase1(dataset, model, s3_iter, features_per_sample, mp)) {
      continue;
    }
    std::cout << "[WORKER] phase 1 done" << std::endl;

    //Dataset dataset(samples.get(), labels.get(),
    //    samples_per_batch, features_per_sample);
    //dataset.check();
    //dataset.print_info();

    auto now = get_time_us();
    // compute mini batch gradient
    std::unique_ptr<ModelGradient> gradient;

    std::cout << "Computing gradient" << std::endl;
    try {
      gradient = model.minibatch_grad(dataset->samples_,
          const_cast<double*>(dataset->labels_.get()), samples_per_batch, config.get_epsilon());
    } catch(const std::runtime_error& e) {
      std::cout << "Error. " << e.what() << std::endl;
      exit(-1);
    } catch(...) {
      std::cout << "There was an error computing the gradient" << std::endl;
      exit(-1);
    }
    auto elapsed_us = get_time_us() - now;
    std::cout << "[WORKER] Gradient compute time (us): " << elapsed_us
      << "at time: " << get_time_us()
      << " version " << version << "\n";
    gradient->setVersion(version++);

    try {
      LRGradient* lrg = dynamic_cast<LRGradient*>(gradient.get());
      push_gradient(r, worker, lrg);
    } catch(...) {
      std::cout << "[WORKER] "
        << "Worker task error doing put of gradient" << "\n";
      exit(-1);
    }
  }
}

