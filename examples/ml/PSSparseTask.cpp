#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "async.h"
#include "adapters/libevent.h"

//#define DEBUG
    
static void update_model(auto&);
static void print_progress();

namespace PSSparseTaskGlobal {
  // used to monitor how much since last received gradient
  static auto prev_on_msg_time = get_time_us();

  static Configuration config;
  static redisAsyncContext* model_r;
  static redisAsyncContext* gradient_r;
  static uint64_t MODEL_GRAD_SIZE;
  
  static std::mutex ps_grad_start_lock; // used as barrier
  static std::mutex mp_start_lock; // used as barrier
 
  sem_t sem_new_model;
  static std::unique_ptr<SparseLRModel> model; // last computed model
  std::mutex model_lock; // used to coordinate access to the last computed model

  redisContext* redis_con;
  volatile uint64_t onMessageCount = 0;
}

/**
  * Works as a cache for remote model
  */
class PSSparseTaskModelProxy {
  public:
    PSSparseTaskModelProxy(auto redis_ip, auto redis_port) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new()) {
    PSSparseTaskGlobal::mp_start_lock.lock();
  }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "ModelProxy::connectCallback" << "\n";
      PSSparseTaskGlobal::mp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << "\n";
    }

    void thread_fn() {
      std::cout << "ModelProxy connecting to redis.." << std::endl;
      PSSparseTaskGlobal::model_r =
        redis_async_connect(redis_ip.c_str(), redis_port);
      if (!PSSparseTaskGlobal::model_r) {
        throw std::runtime_error("ModelProxy::Error connecting to redis");
      }
      std::cout << "ModelProxy::connected to redis.." << PSSparseTaskGlobal::model_r
        << std::endl;

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(PSSparseTaskGlobal::model_r, base);
      redis_connect_callback(PSSparseTaskGlobal::model_r, connectCallback);
      redis_disconnect_callback(PSSparseTaskGlobal::model_r, disconnectCallback);

      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      std::cout << "Starting ModelProxy thread" << std::endl;
      thread = std::make_unique<std::thread>(
          std::bind(&PSSparseTaskModelProxy::thread_fn, this));
    }

  private:
    std::string redis_ip;
    int redis_port;
    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};


auto PSSparseTask::connect_redis() {
  auto redis_con  = redis_connect(REDIS_IP, REDIS_PORT);
  if (redis_con == NULL || redis_con -> err) {
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
  return redis_con;
}

PSSparseTask::PSSparseTask(const std::string& redis_ip, uint64_t redis_port,
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
}

std::shared_ptr<char> serialize_model(const SparseLRModel& model, uint64_t* model_size) {
  *model_size = model.getSerializedSize();
  //std::cout << "Serializing model with size: " << *model_size
  //  << std::endl;
  char* d = model.serializeTo2(model.getSerializedSize());
  auto data = std::shared_ptr<char>(d, std::default_delete<char[]>());
  return data;
}

#ifndef USE_REDIS_CHANNEL
void PSSparseTask::put_model(const SparseLRModel& model) {
  uint64_t model_size;
  std::shared_ptr<char> data = serialize_model(model, &model_size);
  redis_put_binary_numid(r, MODEL_BASE, data.get(), model_size);
}
#else
void PSSparseTask::put_model(const SparseLRModel& model) {
  //lr_model_serializer lms(MODEL_GRAD_SIZE);
  //auto data = std::unique_ptr<char[]>(
  //    new char[lms.size(model)]);
  //lms.serialize(model, data.get());
  uint64_t model_size;
  std::shared_ptr<char> data = serialize_model(model, &model_size);

#ifdef DEBUG
  std::cout << "redisAsyncCommand PUBLISH MODEL" << "\n";
#endif
  redisAsyncCommand(PSSparseTaskGlobal::model_r, NULL, NULL,
      "PUBLISH model %b", data.get(),
      model_size);
}
#endif

void PSSparseTask::publish_model(const SparseLRModel& model) {
  put_model(model);
}

class PSTaskGradientProxy {
  public:
    PSTaskGradientProxy(auto redis_ip, auto redis_port, auto MODEL_GRAD_SIZE) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new()) {
      PSSparseTaskGlobal::MODEL_GRAD_SIZE = MODEL_GRAD_SIZE;
      PSSparseTaskGlobal::ps_grad_start_lock.lock();
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "connectCallback" << std::endl;
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      PSSparseTaskGlobal::onMessageCount++;
      redisReply *r = (redisReply*)reply;
      if (reply == NULL) return;

      //auto now = get_time_us();
      //std::cout << "Time since last (us): "
      //  << (now - PSSparseTaskGlobal::prev_on_msg_time) << "\n";
      //PSSparseTaskGlobal::prev_on_msg_time = now;

#ifdef DEBUG
      std::cout << "onMessage PSTaskGradientProxy" << "\n";
#endif
      if (r->type == REDIS_REPLY_ARRAY) {
        const char* str = r->element[2]->str;
        //uint64_t len = r->element[2]->len;

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
        if ( str_0 && strncmp(str_0, "mes", 3) == 0 && // message
            str_1 && strncmp(str_1, "grad", 4) == 0) { // grad

          LRSparseGradient gradient(0);
          gradient.loadSerialized(str);

#ifdef DEBUG
          std::cout << "Updating model at time: " << get_time_us()
            << " version: " << gradient.getVersion()
            << "\n";
#endif

          // update the model
          update_model(gradient);
          print_progress();
          sem_post(&PSSparseTaskGlobal::sem_new_model);
        }
      } else {
        std::cout << "Not an array" << std::endl;
      }
    }

    void thread_fn() {
      std::cout << "connecting to redis.." << std::endl;
      PSSparseTaskGlobal:: gradient_r =
        redis_async_connect(redis_ip.c_str(), redis_port);

      std::cout << "connected to redis.." << std::endl;

      if (!PSSparseTaskGlobal::model_r || !PSSparseTaskGlobal::gradient_r) {
        throw std::runtime_error(
            "PSSparseTask.PSTaskGradientProxy error connecting to redis");
      }

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(PSSparseTaskGlobal::gradient_r, base);
      redis_connect_callback(PSSparseTaskGlobal::gradient_r, connectCallback);
      redis_disconnect_callback(
          PSSparseTaskGlobal::gradient_r, disconnectCallback);
      redis_subscribe_callback(
          PSSparseTaskGlobal::gradient_r, onMessage, "gradients");

      PSSparseTaskGlobal::ps_grad_start_lock.unlock();
      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&PSTaskGradientProxy::thread_fn, this));
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
void PSSparseTask::run(const Configuration& config) {
  std::cout
    << "PS task initializing model"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;
  
  sem_init(&PSSparseTaskGlobal::sem_new_model, 0, 0);

  // initialize model
  PSSparseTaskGlobal::model.reset(new SparseLRModel(MODEL_GRAD_SIZE));
  PSSparseTaskGlobal::model->randomize();
  std::cout << "[PS] "
    << "PS publishing model at id: " << MODEL_BASE
    << " csum: " << PSSparseTaskGlobal::model->checksum() << std::endl;

  PSSparseTaskGlobal::config = config;

  PSSparseTaskModelProxy mp(REDIS_IP, REDIS_PORT);
  mp.run();
  PSSparseTaskGlobal::mp_start_lock.lock();

  PSTaskGradientProxy gp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE);
  gp.run();
  PSSparseTaskGlobal::ps_grad_start_lock.lock();

  PSSparseTaskGlobal::redis_con = connect_redis();
  publish_model_redis();
  wait_for_start(PS_SPARSE_TASK_RANK, PSSparseTaskGlobal::redis_con, nworkers);

  //publish_model(*PSSparseTaskGlobal::model);

  uint64_t start = get_time_us();
  while (1) {
    sem_wait(&PSSparseTaskGlobal::sem_new_model);
    publish_model_redis();

    auto now = get_time_us();
    auto elapsed_us = now - start;
    if (elapsed_us > 1000000) {
      start = now;
      std::cout << "Events in the last sec: " 
        << 1.0 * PSSparseTaskGlobal::onMessageCount / elapsed_us * 1000 * 1000 << std::endl;
      PSSparseTaskGlobal::onMessageCount = 0;
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

  PSSparseTaskGlobal::model_lock.lock();
  PSSparseTaskGlobal::model->sgd_update(
      PSSparseTaskGlobal::config.get_learning_rate(), &gradient);
  PSSparseTaskGlobal::model_lock.unlock();
}

void PSSparseTask::publish_model_redis() {
#ifdef DEBUG
  static int publish_count = 0;
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " publish_count: " << (++publish_count)
    << "\n";
#endif

  PSSparseTaskGlobal::model_lock.lock();
  auto model_copy = *PSSparseTaskGlobal::model;
  PSSparseTaskGlobal::model_lock.unlock();

#ifdef DEBUG
  std::cout << "Checking model" << std::endl;
  model_copy.check();
#endif
  
  uint64_t model_size;
  std::shared_ptr<char> data = serialize_model(model_copy, &model_size);

#ifdef DEBUG
  std::cout << "redisCommand PUBLISH MODEL" << std::endl;
  auto before_us = get_time_us();
#endif

  redis_put_binary_numid(PSSparseTaskGlobal::redis_con, MODEL_BASE,
      reinterpret_cast<const char*>(data.get()), model_size);
#ifdef DEBUG
  auto elapsed_us = get_time_us() - before_us;
  std::cout 
    << "Put model elapsed (us): " << elapsed_us
    << " bw (MB/s): " << (1.0 * model_size / 1024 / 1024 / elapsed_us * 1000 * 1000)
    << "\n";
#endif
}

void PSSparseTask::publish_model_pubsub() {
#ifdef DEBUG
  static int publish_count = 0;
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " publish_count: " << (++publish_count)
    << "\n";
#endif

  PSSparseTaskGlobal::model_lock.lock();
  auto model_copy = *PSSparseTaskGlobal::model;
  PSSparseTaskGlobal::model_lock.unlock();

  uint64_t model_size;
  std::shared_ptr<char> data = serialize_model(model_copy, &model_size);

#ifdef DEBUG
  std::cout << "redisCommand PUBLISH MODEL" << std::endl;
#endif
  redisReply* reply = (redisReply*)redisCommand(PSSparseTaskGlobal::redis_con, "PUBLISH model %b", data.get(), model_size);
  freeReplyObject(reply);
}

void PSSparseTask::update_publish_gradient(auto& gradient) {
  throw std::runtime_error("Not supported. Review code");
  // do a gradient step and update model
#ifdef DEBUG
  static int count = 0;
  std::cout << "[PS] " << "Updating model" << "\n";
#endif

  PSSparseTaskGlobal::model_lock.lock();
  PSSparseTaskGlobal::model->sgd_update(
      PSSparseTaskGlobal::config.get_learning_rate(), &gradient);
  auto model_copy = *PSSparseTaskGlobal::model;
  PSSparseTaskGlobal::model_lock.lock();

#ifdef DEBUG
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " count: " << (++count)
    << "\n";
#endif

  uint64_t model_size;
  std::shared_ptr<char> data = serialize_model(model_copy, &model_size);

#ifdef DEBUG
  std::cout << "redisAsyncCommand PUBLISH MODEL" << std::endl;
#endif
  redisAsyncCommand(PSSparseTaskGlobal::model_r, NULL, NULL,
      "PUBLISH model %b", data.get(),
      model_size);

  print_progress();
}

