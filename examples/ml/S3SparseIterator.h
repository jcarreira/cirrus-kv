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
#include <CircularBuffer.h>

class S3SparseIterator {
 public:
    S3SparseIterator(
        uint64_t left_id, uint64_t right_id, // range for iteration
        const Configuration& c,
        uint64_t s3_rows, //XXX is this redundant?
        uint64_t minibatch_rows,
        int worker_id = 0,
        bool random_access = true);

    const void* get_next_fast(); //XXX rename. gets next minibatch

    void thread_function(const Configuration&); //XXX this should be private?

 private:
  void push_samples(std::ostringstream* oss);
  void print_progress(const std::string& s3_obj);
  uint64_t get_obj_id(uint64_t left, uint64_t right);

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

  sem_t semaphore;
  int str_version = 0; //XXX comment here
  std::map<int, std::string> list_strings; // strings from s3
  CircularBuffer<std::pair<const void*, int>> minibatches_list;//(100000);
  int to_delete = -1;
  int worker_id = 0;
  
  std::default_random_engine re;
  bool random_access = true;
  uint64_t current = 0;
};

#endif  // _S3_SPARSEITERATOR_H_
