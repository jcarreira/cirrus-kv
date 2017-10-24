#ifndef _REDIS_ITERATOR_H_
#define _REDIS_ITERATOR_H_

#include <thread>
#include <mutex>
#include <list>
#include "Redis.h"

class RedisIterator {
 public:
    RedisIterator(uint64_t left_id, uint64_t right_id, const std::string& IP, int port);

    char* get_next();

    void thread_function();

 private:
  uint64_t left_id;
  uint64_t right_id;
  std::string IP;
  int port;
  redisContext* r;

  std::thread* thread;
  std::mutex ring_lock;
  uint64_t cur;
  uint64_t last;
  std::list<char*> ring;

  uint64_t read_ahead = 5;
};

#endif // _REDIS_ITERATOR_H_
