#ifndef _S3_SPARSEITERATOR_H_
#define _S3_SPARSEITERATOR_H_

#include "S3.h"
#include <common/Synchronization.h>
#include "Configuration.h"

#include <thread>
#include <list>
#include <mutex>
#include "Serializers.h"
#include "ProgressMonitor.h"

class S3SparseIterator {
 public:
    S3SparseIterator(
        uint64_t left_id, uint64_t right_id,
        const Configuration& c,
        uint64_t s3_rows,
        uint64_t minibatch_rows);

    const void* get_next_fast();

    void thread_function(const Configuration&);

 private:
  void push_samples(std::ostringstream* oss);

  uint64_t left_id;
  uint64_t right_id;

  Configuration conf;

  std::shared_ptr<Aws::S3::S3Client> s3_client;

  uint64_t cur;
  uint64_t last;
  std::list<std::shared_ptr<FEATURE_TYPE>> ring;

  uint64_t read_ahead = 1;

  std::thread* thread;   // background thread
  std::mutex ring_lock;  // used to synchronize access
  // used to control number of blocks to prefetch
  cirrus::PosixSemaphore pref_sem;

  uint64_t s3_rows;
  uint64_t minibatch_rows;
  uint64_t features_per_sample;

  ProgressMonitor pm;
};

#endif  // _S3_SPARSEITERATOR_H_
