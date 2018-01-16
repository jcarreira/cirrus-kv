#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3Iterator.h"
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

static auto connect_redis() {
  std::cout << "[WORKER] "
    << "Worker task connecting to REDIS. "
    << "IP: " << REDIS_IP << std::endl;
  auto redis_con = redis_connect(REDIS_IP, REDIS_PORT);
  check_redis(redis_con);
  return redis_con;
}

std::mutex start_lock;
redisAsyncContext* gradient_r;

class ModelProxy {
  public:
    ModelProxy(auto redis_ip, auto redis_port) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new()) {
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "ModelProxy::connectCallback" << std::endl;
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    static void onMessage(redisAsyncContext*, void* /**reply*/, void*) {
    }

    void thread_fn() {
      std::cout << "ModelProxy connecting to redis.." << std::endl;
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
    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};

/**
  * Works as a cache for remote model
  */
class SenderProxy {
  public:
    SenderProxy(
        auto redis_ip, auto redis_port) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new()) {
  }
    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "GradientProxy::connectCallback" << std::endl;
      start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      std::cout << "SenderProxy onMessage" << std::endl;
      if (reply == NULL) return;
    }

    void thread_fn() {
      std::cout << "GradientProxy connecting to redis.." << std::endl;
      gradient_r =
        redis_async_connect(redis_ip.c_str(), redis_port);
      if (!gradient_r) {
        throw std::runtime_error("GradientProxy::Error connecting to redis");
      }
      std::cout << "GradientProxy connected to redis.." << gradient_r
        << std::endl;

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(gradient_r, base);
      redis_connect_callback(gradient_r, connectCallback);
      redis_disconnect_callback(gradient_r, disconnectCallback);

      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&SenderProxy::thread_fn, this));
    }
  private:
    std::string redis_ip;
    int redis_port;
    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};

void run() {
  std::cout << "Connecting to redis.." << std::endl;
  redisContext* redis_con = connect_redis();
  
  //std::cout << "Starting ModelProxy" << std::endl;
  //ModelProxy mp(REDIS_IP, REDIS_PORT);
  //mp.run();

  std::cout << "Starting GradientProxy" << std::endl;
  SenderProxy gp(REDIS_IP, REDIS_PORT);
  gp.run();
  start_lock.lock();
  
  std::cout << "[WORKER] starting loop" << std::endl;

  char data[10000];
  uint64_t data_size = 14 * 8 - 4;
  uint64_t count = 0;
  while (1) {
    count++;
    //if (count % 10000 == 0) {
    //  std::cout << "count: " << count << std::endl;
    //}
    redisReply* reply = (redisReply*)redisCommand(redis_con, "PUBLISH gradients %b", data, data_size);
    freeReplyObject(reply);
  }
}

int main() {
  run();
  return 0;
}
