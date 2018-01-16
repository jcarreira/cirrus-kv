#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "async.h"
#include "adapters/libevent.h"

//#define DEBUG

void update_publish_gradient(auto& gradient);
void publish_model2();
void publish_model3();
void update_model(auto& gradient);
void print_progress();

namespace PSTaskGlobal {
  static std::unique_ptr<lr_gradient_deserializer> lgd;

  // used to monitor how much since last received gradient
  static auto prev_on_msg_time = get_time_us();

  static Configuration config;
  static redisAsyncContext* model_r;
  static redisAsyncContext* gradient_r;
  static uint64_t MODEL_GRAD_SIZE;
  
  static std::mutex ps_grad_start_lock; // used as barrier
  static std::mutex mp_start_lock; // used as barrier
 
  sem_t sem_new_model;
  static std::unique_ptr<LRModel> model; // last computed model
  std::mutex model_lock; // used to coordinate access to the last computed model

  redisContext* redis_con;
}

/**
  * Works as a cache for remote model
  */
class PSTaskModelProxy {
  public:
    PSTaskModelProxy(auto redis_ip, auto redis_port) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new()) {
        PSTaskGlobal::mp_start_lock.lock();
      }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "ModelProxy::connectCallback" << "\n";
      PSTaskGlobal::mp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << "\n";
    }

    void thread_fn() {
      std::cout << "ModelProxy connecting to redis.." << std::endl;
      PSTaskGlobal::model_r =
        redis_async_connect(redis_ip.c_str(), redis_port);
      if (!PSTaskGlobal::model_r) {
        throw std::runtime_error("ModelProxy::Error connecting to redis");
      }
      std::cout << "ModelProxy::connected to redis.." << PSTaskGlobal::model_r
        << std::endl;

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(PSTaskGlobal::model_r, base);
      redis_connect_callback(PSTaskGlobal::model_r, connectCallback);
      redis_disconnect_callback(PSTaskGlobal::model_r, disconnectCallback);

      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      std::cout << "Starting ModelProxy thread" << std::endl;
      thread = std::make_unique<std::thread>(
          std::bind(&PSTaskModelProxy::thread_fn, this));
    }

  private:
    std::string redis_ip;
    int redis_port;
    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};


auto PSTask::connect_redis() {
  auto redis_con  = redis_connect(REDIS_IP, REDIS_PORT);
  if (redis_con == NULL || redis_con -> err) {
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
  return redis_con;
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
      nworkers, worker_id) {
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

#ifdef DEBUG
  std::cout << "redisAsyncCommand PUBLISH MODEL" << "\n";
#endif
  redisAsyncCommand(PSTaskGlobal::model_r, NULL, NULL,
      "PUBLISH model %b", data.get(),
      lms.size(model));
}
#endif

#if 0
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
#endif

void PSTask::publish_model(const LRModel& model) {
  put_model(model);
}

volatile uint64_t onMessageCount = 0;

class PSGradientProxy {
  public:
    PSGradientProxy(auto redis_ip, auto redis_port, auto MODEL_GRAD_SIZE) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new()) {
      PSTaskGlobal::MODEL_GRAD_SIZE = MODEL_GRAD_SIZE;
      PSTaskGlobal::lgd.reset(
          new lr_gradient_deserializer(MODEL_GRAD_SIZE));
      PSTaskGlobal::ps_grad_start_lock.lock();
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "connectCallback" << std::endl;
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      onMessageCount++;
      //sem_post(&PSTaskGlobal::sem_new_model);
      //return;

      redisReply *r = (redisReply*)reply;
      if (reply == NULL) return;

      //auto now = get_time_us();
      //std::cout << "Time since last (us): "
      //  << (now - PSTaskGlobal::prev_on_msg_time) << "\n";
      //PSTaskGlobal::prev_on_msg_time = now;

#ifdef DEBUG
      std::cout << "onMessage PSGradientProxy" << "\n";
#endif
      if (r->type == REDIS_REPLY_ARRAY) {
        const char* str = r->element[2]->str;
        uint64_t len = r->element[2]->len;

#ifdef DEBUG
        std::cout << "len: "
          << r->element[0]->len << " "
          << r->element[1]->len << " "
          << r->element[2]->len << " "
          << "\n";

        if (r->element[0]->str) {
          std::cout <<"str0: " << r->element[0]->str << "\n";
        }
        if (r->element[1]->str) {
          std::cout <<"str1: " << r->element[1]->str << "\n";
        }
#endif
        // r->element[0]->str == "message"
        char* str_1 = r->element[1]->str;
        char* str_0 = r->element[0]->str;
        if ( str_0 && strcmp(str_0, "message") == 0 &&
            str_1 && strcmp(str_1, "gradients") == 0) {
          auto gradient = PSTaskGlobal::lgd->operator()(str, len);

#ifdef DEBUG
          std::cout << "Updating model at time: " << get_time_us()
            << " version: " << gradient.getVersion()
            << "\n";
#endif

          // update the model
          //update_model(gradient);
          //print_progress();
          sem_post(&PSTaskGlobal::sem_new_model);
        }
      } else {
        std::cout << "Not an array" << std::endl;
      }
    }

    void thread_fn() {
      std::cout << "connecting to redis.." << std::endl;
      PSTaskGlobal:: gradient_r =
        redis_async_connect(redis_ip.c_str(), redis_port);

      std::cout << "connected to redis.." << std::endl;

      if (!PSTaskGlobal::model_r || !PSTaskGlobal::gradient_r) {
        throw std::runtime_error(
            "PSTask.PSGradientProxy error connecting to redis");
      }

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(PSTaskGlobal::gradient_r, base);
      redis_connect_callback(PSTaskGlobal::gradient_r, connectCallback);
      redis_disconnect_callback(
          PSTaskGlobal::gradient_r, disconnectCallback);
      redis_subscribe_callback(
          PSTaskGlobal::gradient_r, onMessage, "gradients");

      PSTaskGlobal::ps_grad_start_lock.unlock();
      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&PSGradientProxy::thread_fn, this));
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
  
  sem_init(&PSTaskGlobal::sem_new_model, 0, 0);

  // initialize model
  PSTaskGlobal::model.reset(new LRModel(MODEL_GRAD_SIZE));
  PSTaskGlobal::model->randomize();
  std::cout << "[PS] "
    << "PS publishing model at id: " << MODEL_BASE
    << " csum: " << PSTaskGlobal::model->checksum() << std::endl;

  PSTaskGlobal::config = config;

  PSTaskModelProxy mp(REDIS_IP, REDIS_PORT);
  mp.run();
  PSTaskGlobal::mp_start_lock.lock();

  PSGradientProxy gp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE);
  gp.run();
  PSTaskGlobal::ps_grad_start_lock.lock();

  PSTaskGlobal::redis_con = connect_redis();
  wait_for_start(PS_TASK_RANK, PSTaskGlobal::redis_con, nworkers);

  publish_model(*PSTaskGlobal::model);

  uint64_t start = get_time_us();
  while (1) {
    sem_wait(&PSTaskGlobal::sem_new_model);
    //publish_model2();
    publish_model3();

    auto now = get_time_us();
    auto elapsed_us = now - start;
    if (elapsed_us > 1000000) {
      start = now;
      //std::cout << "onMessageCount: " << onMessageCount << std::endl;
      std::cout << "Events in the last sec: " << 1.0 * onMessageCount / elapsed_us * 1000 * 1000 << std::endl;
      onMessageCount = 0;
    }
  }
}

void print_progress() {
  static uint64_t count = 0;
  static auto start = get_time_us();

  // if it's the first time we record the timestamp
  if (count == 2000) {
    start = get_time_us();
  } else if (count > 2000 && count % 1000 == 0) {
    auto now = get_time_us();
    auto elapsed_us = now - start;
    double iterations_per_us = 1.0 * count / elapsed_us;
    double iterations_per_sec = iterations_per_us * 1000 * 1000;
    std::cout << "Iterations per sec: " << iterations_per_sec << "\n";
  }
  count++;
}

void update_model(auto& gradient) {
  // do a gradient step and update model
#ifdef DEBUG
  static int update_count = 0;
  std::cout << "[PS] " << "Updating model updating_count: " << update_count << "\n";
#endif

  PSTaskGlobal::model_lock.lock();
  PSTaskGlobal::model->sgd_update(
      PSTaskGlobal::config.get_learning_rate(), &gradient);
  PSTaskGlobal::model_lock.unlock();
}

// publish model in redis
void PSTask::publish_model3() {
#ifdef DEBUG
  static int publish_count = 0;
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " publish_count: " << (++publish_count)
    << "\n";
#endif

  PSTaskGlobal::model_lock.lock();
  auto model_copy = *PSTaskGlobal::model;
  PSTaskGlobal::model_lock.unlock();

  lr_model_serializer lms(PSTaskGlobal::MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(model_copy)]);
  lms.serialize(model_copy, data.get());

#ifdef DEBUG
  std::cout << "redisCommand PUBLISH MODEL" << std::endl;
#endif
  redis_put_binary_numid(PSTaskGlobal::redis_con, MODEL_BASE, data.get(), lms.size(model_copy));
}

void publish_model2() {
#ifdef DEBUG
  static int publish_count = 0;
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " publish_count: " << (++publish_count)
    << "\n";
#endif

  PSTaskGlobal::model_lock.lock();
  auto model_copy = *PSTaskGlobal::model;
  PSTaskGlobal::model_lock.unlock();

  lr_model_serializer lms(PSTaskGlobal::MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(model_copy)]);
  lms.serialize(model_copy, data.get());

#ifdef DEBUG
  std::cout << "redisCommand PUBLISH MODEL" << std::endl;
#endif
  redisReply* reply = (redisReply*)redisCommand(PSTaskGlobal::redis_con, "PUBLISH model %b", data.get(), lms.size(model_copy));
  freeReplyObject(reply);
}

void update_publish_gradient(auto& gradient) {
  throw std::runtime_error("Not supported. Review code");
  // do a gradient step and update model
#ifdef DEBUG
  static int count = 0;
  std::cout << "[PS] " << "Updating model" << "\n";
#endif

  PSTaskGlobal::model_lock.lock();
  PSTaskGlobal::model->sgd_update(
      PSTaskGlobal::config.get_learning_rate(), &gradient);
  auto model_copy = *PSTaskGlobal::model;
  PSTaskGlobal::model_lock.lock();

#ifdef DEBUG
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " count: " << (++count)
    << "\n";
#endif

  lr_model_serializer lms(PSTaskGlobal::MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(model_copy)]);
  lms.serialize(model_copy, data.get());

#ifdef DEBUG
  std::cout << "redisAsyncCommand PUBLISH MODEL" << std::endl;
#endif
  redisAsyncCommand(PSTaskGlobal::model_r, NULL, NULL,
      "PUBLISH model %b", data.get(),
      lms.size(model_copy));

  print_progress();
}

