#include <examples/ml/Tasks.h>

#include "Redis.h"
#include "Utils.h"
#include "async.h"
#include "adapters/libevent.h"

static redisAsyncContext* gradient_r;
std::mutex start_lock;
volatile uint64_t onMessageCount_ = 0;

class GradientProxy {
  public:
    GradientProxy(auto redis_ip, auto redis_port) :
      redis_ip(redis_ip), redis_port(redis_port),
      base(event_base_new()) {
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "connectCallback" << std::endl;
      start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    static void onMessage(redisAsyncContext*, void* reply, void*) {
      redisReply *r = (redisReply*)reply;
      if (reply == NULL) return;
      if (r->type == REDIS_REPLY_ARRAY) {
        char* str_1 = r->element[1]->str;
        char* str_0 = r->element[0]->str;
        if ( str_0 && strcmp(str_0, "message") == 0 &&
            str_1 && strcmp(str_1, "gradients") == 0) {
          onMessageCount_++;
        }
      }
    }

    void thread_fn() {
      std::cout << "connecting to redis ip: " << REDIS_IP
        << " port: " << REDIS_PORT  << std::endl;
      gradient_r =
        redis_async_connect(redis_ip.c_str(), redis_port);

      if (!gradient_r) {
        throw std::runtime_error(
            "GradientProxy error connecting to redis");
      }
      std::cout << "connected to redis.." << std::endl;

      std::cout << "libevent attached" << std::endl;
      redisLibeventAttach(gradient_r, base);
      redis_connect_callback(gradient_r, connectCallback);
      redis_disconnect_callback(
          gradient_r, disconnectCallback);
      redis_subscribe_callback(
          gradient_r, onMessage, "gradients");

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
    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};

void run() {
  GradientProxy gp(REDIS_IP, REDIS_PORT);
  gp.run();
  start_lock.lock();

  uint64_t start = get_time_us();
  while (1) {
    sleep(1);
    auto elapsed_us = get_time_us() - start;
    std::cout << "Iters/sec: " << 1.0 * onMessageCount_ / elapsed_us * 1000 * 1000 << std::endl;
    onMessageCount_ = 0;
    start = get_time_us();
  }
}

int main() {
  run();
  return 0;
}
