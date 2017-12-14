#ifndef _S3_ITERATOR_H_
#define _S3_ITERATOR_H_

#include "S3.h"
#include "Configuration.h"
#include <thread>
#include <mutex>
#include <list>
#include <string>
#include <common/Synchronization.h>
#include "Serializers.h"

class S3Iterator {
 public:
    S3Iterator(
        uint64_t left_id, uint64_t right_id,
        const Configuration& c,
        uint64_t s3_rows, uint64_t s3_cols,
        uint64_t minibatch_rows);

    std::shared_ptr<double> get_next();

    void thread_function();

 private:
  void push_samples(const std::shared_ptr<double>& samples);

  uint64_t left_id;
  uint64_t right_id;
  
  Configuration conf;

  Aws::S3::S3Client s3_client;

  uint64_t cur;
  uint64_t last;
  std::list<std::shared_ptr<double>> ring;

  uint64_t read_ahead = 5;

  std::thread* thread; // background thread
  std::mutex ring_lock; // used to synchronize access
  // used to control number of blocks to prefetch
  cirrus::PosixSemaphore pref_sem;

  uint64_t s3_rows;
  uint64_t s3_cols;
  uint64_t minibatch_rows;
  uint64_t features_per_sample;
};

#endif // _S3_ITERATOR_H_
