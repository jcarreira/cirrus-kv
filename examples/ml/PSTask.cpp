#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "async.h"
#include "adapters/libevent.h"

#define DEBUG

void update_publish_gradient(auto& gradient);

namespace PSTaskGlobal {
  static volatile bool first_time = true;
  static std::unique_ptr<lr_gradient_deserializer> lgd;//(MODEL_GRAD_SIZE);
  static auto prev_on_msg_time = get_time_us();
  static std::unique_ptr<LRModel> model;
  static Configuration config;
  static redisContext* model_r;
  static redisAsyncContext* gradient_r;
  static uint64_t MODEL_GRAD_SIZE;
}

auto PSTask::connect_redis() {
  auto r  = redis_connect(REDIS_IP, REDIS_PORT);
  if (r == NULL || r -> err) { 
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
  return r;
}

PSTask::PSTask(const std::string& redis_ip, uint64_t redis_port,
    uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
    uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
    uint64_t SAMPLE_BASE, uint64_t START_BASE,
    uint64_t batch_size, uint64_t samples_per_batch, 
    uint64_t features_per_sample, uint64_t nworkers,
    uint64_t worker_id) :
  MLTask(redis_ip, redis_port, MODEL_GRAD_SIZE, MODEL_BASE,
      LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
      batch_size, samples_per_batch, features_per_sample,
      nworkers, worker_id)
{
#if defined(USE_REDIS)
  PSTaskGlobal::model_r = connect_redis();
#endif
  gradientVersions.resize(nworkers, 0);
  worker_clocks.resize(nworkers, 0);
}


#ifndef USE_REDIS_CHANNEL
void PSTask::put_model(const LRModel& model) {
  lr_model_serializer lms(MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(model)]);
  lms.serialize(model, data.get());
  redis_put_binary_numid(r, MODEL_BASE, data.get(), lms.size(model));
}
#else
void PSTask::put_model(const LRModel& model) {
  lr_model_serializer lms(MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(model)]);
  lms.serialize(model, data.get());

  redisReply* reply = (redisReply*)redisCommand(
      PSTaskGlobal::model_r,
      "PUBLISH model %b", data.get(), lms.size(model));
  assert(reply);
}
#endif

void PSTask::get_gradient(auto r, auto& gradient, auto gradient_id) {
  int len_grad;
  lr_gradient_deserializer lgd(MODEL_GRAD_SIZE);

  char* data = redis_get_numid(r, gradient_id, &len_grad);
  if (data == nullptr) {
    throw cirrus::NoSuchIDException("");
  }
  gradient = lgd(data, len_grad);
  free(data);
}

void PSTask::publish_model(const LRModel& model) {
  put_model(model);
}

class GradientProxy {
  public:
    GradientProxy(auto redis_ip, auto redis_port, auto MODEL_GRAD_SIZE) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new())
    {
      PSTaskGlobal::MODEL_GRAD_SIZE = MODEL_GRAD_SIZE;
      PSTaskGlobal::lgd.reset(
          new lr_gradient_deserializer(MODEL_GRAD_SIZE));
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "connectCallback" << std::endl;
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }
    
    static void onMessage(redisAsyncContext*, void *reply, void*) {
      redisReply *r = (redisReply*)reply;
      if (reply == NULL) return;

      auto now = get_time_us();
      std::cout << "Time since last (us): "
        << (now - PSTaskGlobal::prev_on_msg_time) << std::endl;
      PSTaskGlobal::prev_on_msg_time = now;
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
            << std::endl;
#endif
          //model_lock.lock();
          auto gradient = PSTaskGlobal::lgd->operator()(str, len);
          update_publish_gradient(gradient);
          //model_lock.unlock();
          PSTaskGlobal::first_time = false;
        }
      } else {
        std::cout << "Not an array" << std::endl;
      }
    }
    
    void thread_fn() {
      std::cout << "connecting to redis.." << std::endl;
      //redis_lock->lock();
      PSTaskGlobal:: gradient_r =
        redis_async_connect(redis_ip.c_str(), redis_port);

      std::cout << "connected to redis.." << std::endl;

      if (!PSTaskGlobal::model_r || !PSTaskGlobal::gradient_r) {
        throw std::runtime_error("Error connecting to redis");
      }
      
      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(PSTaskGlobal::gradient_r, base);
      redis_connect_callback(PSTaskGlobal::gradient_r, connectCallback);
      redis_disconnect_callback(
          PSTaskGlobal::gradient_r, disconnectCallback);
      redis_subscribe_callback(
          PSTaskGlobal::gradient_r, onMessage, "gradients");
      //redis_lock->unlock();
      
      //mp_start_lock.unlock();
      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }
    
    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&GradientProxy::thread_fn, this));
    }

  private:
    std::string redis_ip;
    int redis_port;
    uint64_t MODEL_GRAD_SIZE;
    struct event_base *base;
    
    std::unique_ptr<std::thread> thread;
};


/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void PSTask::run(const Configuration& config) {
  std::cout << "[PS] " << "PS task initializing model" << std::endl;
  // initialize model
  PSTaskGlobal::model.reset(new LRModel(MODEL_GRAD_SIZE));
  PSTaskGlobal::model->randomize();
  std::cout << "[PS] "
    << "PS publishing model at id: " << MODEL_BASE
    << " csum: " << PSTaskGlobal::model->checksum() << std::endl;

  publish_model(*PSTaskGlobal::model);
  wait_for_start(PS_TASK_RANK, PSTaskGlobal::model_r, nworkers);

  PSTaskGlobal::config = config;
  GradientProxy gp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE);
  gp.run();

  publish_model(*PSTaskGlobal::model);

  while (1) {
    sleep(1);
  }
}

void print_progress() {
  static uint64_t count = 0;
  static auto start = get_time_us();

  // if it's the first time we record the timestamp
  if (count == 0) {
    start = get_time_us();
  } else if (count % 10 == 0) {
    auto now = get_time_us();
    auto elapsed_us = now - start;
    double iterations_per_us = 1.0 * count / elapsed_us;
    double iterations_per_sec = iterations_per_us * 1000 * 1000;
    std::cout << "Iterations per sec: " << iterations_per_sec << "\n";
  }
  count++;
}

void PSTask::update_gradient_version(
    auto& gradient, int worker, LRModel& model, Configuration config ) {
#ifdef DEBUG
  std::cout << "[PS] " << "PS task received new gradient version: "
    << gradient.getVersion() << std::endl;
#endif
  // if it's new
  gradientVersions[worker] = gradient.getVersion();

  // do a gradient step and update model
#ifdef DEBUG
  std::cout << "[PS] " << "Updating model" << std::endl;
#endif

  PSTaskGlobal::model->sgd_update(config.get_learning_rate(), &gradient);

  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << std::endl;
    //<< " checksum: " << model.checksum()
  // publish the model back to the store so workers can use it
#ifdef USE_CIRRUS
  model_store.put(MODEL_BASE, model);
#elif defined(USE_REDIS)
  put_model(model);
#endif

  print_progress();
}

void update_publish_gradient(auto& gradient) {
  // do a gradient step and update model
#ifdef DEBUG
  std::cout << "[PS] " << "Updating model" << std::endl;
#endif

  PSTaskGlobal::model->sgd_update(
      PSTaskGlobal::config.get_learning_rate(), &gradient);

  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << std::endl;
  
  lr_model_serializer lms(PSTaskGlobal::MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(*PSTaskGlobal::model)]);
  lms.serialize(*PSTaskGlobal::model, data.get());

  redisReply* reply = (redisReply*)redisCommand(
      PSTaskGlobal::model_r, "PUBLISH model %b", data.get(),
      lms.size(*PSTaskGlobal::model));
  assert(reply);

  print_progress();
}

