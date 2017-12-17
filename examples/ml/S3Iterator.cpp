#include "S3Iterator.h"
#include <unistd.h>
#include <vector>
#include <iostream>

#define S3_BUCKET "cirrusonlambdas"

// s3_cad_size nmber of samples times features per sample
S3Iterator::S3Iterator(
        uint64_t left_id, uint64_t right_id,
        const Configuration& c,
        uint64_t s3_rows, uint64_t s3_cols,
        uint64_t minibatch_rows) :
    left_id(left_id), right_id(right_id),
    conf(c), s3_rows(s3_rows), s3_cols(s3_cols),
    minibatch_rows(minibatch_rows) {
      
  std::cout << "Creating S3Iterator"
    << " left_id: " << left_id
    << " right_id: " << right_id
    << std::endl;

  // initialize s3
  s3_initialize_aws();
  s3_client.reset(s3_create_client_ptr());

  //cur = left_id;
  last = left_id;  // last is exclusive

  for (uint64_t i = 0; i < read_ahead; ++i) {
    pref_sem.signal();
  }

  thread = new std::thread(std::bind(&S3Iterator::thread_function, this));
}

std::shared_ptr<double> S3Iterator::get_next() {
  //std::cout << "Get next "
  //  //<< " cur: " << cur
  //  << " last: " << last
  //  << "\n";
  while (1) {
    ring_lock.lock();
    if (ring.empty()) {
      ring_lock.unlock();
      usleep(1000);
    } else {
      break;
    }
  }

  std::shared_ptr<double> ret = ring.front();
  ring.pop_front();
  //cur++;
  //if (cur == right_id) {
  //  cur = left_id;
  //}
  
  uint64_t ring_size = ring.size();
  ring_lock.unlock();

  if (ring_size < 10000 && pref_sem.getvalue() < (int)read_ahead) {
    //std::cout << "Signal semaphore" << std::endl;
    pref_sem.signal();
  }

  //std::cout << "Returning prefetched batch"
  //  //<< " cur: " << cur
  //  << " last: " << last
  //  << " ring size: " << ring_size
  //  << std::endl;
  return ret;
}

void S3Iterator::push_samples(const std::shared_ptr<double>& samples) {
  uint64_t n_minibatches = s3_rows / minibatch_rows;
  uint64_t minibatch_n_entries = minibatch_rows * (s3_cols + 1);

  std::list<std::shared_ptr<double>> minibatches;
  for (uint64_t i = 0; i < n_minibatches; ++i) {
    double* data = samples.get() + i * minibatch_n_entries;
    auto minibatch_copy = std::shared_ptr<double>(
        new double[minibatch_n_entries], std::default_delete<double[]>());
    std::copy(data, data + minibatch_n_entries, minibatch_copy.get());
    minibatches.push_back(minibatch_copy);
  }

  ring_lock.lock();
  for (std::shared_ptr<double> d : minibatches) {
    ring.push_back(d);
  }
  ring_lock.unlock();
}

void S3Iterator::thread_function() {

  std::cout << "Building S3 deser. with size: "
    << s3_rows << " x " << (s3_cols + 1) << " = " << (s3_rows * (s3_cols + 1))
    << std::endl;
  c_array_deserializer<double> cad_samples(
      s3_rows * (s3_cols + 1), // also count labels
      "S3 deserializer");

  uint64_t count = 0;
  while (1) {
    // if we can go it means there is a slot
    // in the ring
    pref_sem.wait();
    std::cout << "Getting object. count: " << count++ << std::endl;

    std::string s3_obj;
    try {
      std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
      s3_obj = s3_get_object(last, *s3_client, S3_BUCKET);
      std::chrono::steady_clock::time_point finish =
        std::chrono::steady_clock::now();
      uint64_t elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            finish-start).count();

      double MBps = (1.0 * s3_obj.size() / elapsed_ns) / 1024 / 1024 * 1000 * 1000 * 1000;
      std::cout << "Get s3 obj took (us): " << (elapsed_ns / 1000.0)
        << " size (KB): " << (s3_obj.size() / 1024.0)
        << " bandwidth (MB/s): " << MBps
        << std::endl;
    } catch(...) {
      std::cout << "S3Iterator: error in s3_get_object" << std::endl;
      exit(-1);
    }
    
    // update index
    last++;
    //if (last == cur)
    //  throw std::runtime_error("Error in iterator");
    if (last == right_id)
      last = left_id;

    auto samples = cad_samples(s3_obj.c_str(), s3_obj.size());
    push_samples(samples);
  }
}

