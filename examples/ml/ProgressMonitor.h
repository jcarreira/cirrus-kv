#ifndef _PROGRESS_MONITOR_H_
#define _PROGRESS_MONITOR_H_

#include "Configuration.h"
#include "Redis.h"
  
static const char* PROGRESS_COUNTER = "batch_counter";

class ProgressMonitor {
 public:
    ProgressMonitor(const std::string& ip, int port);

    void increment_batches(int* prev_batch = nullptr);

 private:
    redisContext* redis_con;
};

#endif  // _PROGRESS_MONITOR_H_
