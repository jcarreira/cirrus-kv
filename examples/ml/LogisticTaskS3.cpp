#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3Iterator.h"

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

auto LogisticTaskS3::get_model(auto r, auto lmd) {
  int len;
  char* data = redis_get_numid(r, MODEL_BASE, &len);
  if (data == nullptr) {
    throw cirrus::NoSuchIDException("");
  }
  auto model = lmd(data, len);
  free(data);
  return model;
}

void LogisticTaskS3::push_gradient(auto r, int worker, LRGradient* lrg) {
  lr_gradient_serializer lgs(MODEL_GRAD_SIZE);

  uint64_t gradient_size = lgs.size(*lrg);
  auto data = std::unique_ptr<char[]>(new char[gradient_size]);
  lgs.serialize(*lrg, data.get());
  
  int gradient_id = GRADIENT_BASE + worker;
  redis_put_binary_numid(r, gradient_id, data.get(), gradient_size);
    
  std::cout << "[WORKER] "
      << "Worker task stored gradient at id: " << gradient_id
      << " at time (us): " << get_time_us() << "\n";
}

/** We unpack each minibatch into samples and labels
  */
void unpack_minibatch(
    std::shared_ptr<double> /*minibatch*/,
    auto& samples, auto& labels) {
  samples = std::shared_ptr<double>(new double[0], std::default_delete<double[]>());
  labels = std::shared_ptr<double>(new double[0], std::default_delete<double[]>());
}

// get samples and labels data
bool LogisticTaskS3::run_phase1(
    auto& samples, auto& labels, auto& model, auto r,
    auto& s3_iter, uint64_t /*features_per_sample*/) {
  
  static bool first_time = true;
  c_array_deserializer<double> cad_samples(batch_size,
      "worker samples_store");
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);
  c_array_deserializer<double> cad_labels(samples_per_batch,
      "worker labels_store", false);

  try {
    auto before_model = get_time_ns();
    model = get_model(r, lmd);
    auto after_model = get_time_ns();
    std::cout << "[WORKER] "
      << "model get (ns): " << (after_model - before_model)
      << std::endl;

    std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();

    std::shared_ptr<double> minibatch = s3_iter.get_next();
    unpack_minibatch(minibatch, samples, labels);

    std::chrono::steady_clock::time_point finish =
      std::chrono::steady_clock::now();
    uint64_t elapsed_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          finish-start).count();
    double bw = 1.0 * batch_size * sizeof(double) /
      elapsed_ns * 1000.0 * 1000 * 1000 / 1024 / 1024;
    std::cout << "Get Sample Elapsed (S3) "
      << " batch size: " << batch_size
      << " ns: " << elapsed_ns
      << " BW (MB/s): " << bw
      << "at time: " << get_time_us()
      << "\n";
  } catch(const cirrus::NoSuchIDException& e) {
    if (!first_time) {
      std::cout << "[WORKER] Exiting" << std::endl;
      exit(0);
    }
    // this happens because the ps task
    // has not uploaded the model yet
    std::cout << "[WORKER] "
      << "Model could not be found at id: " << MODEL_BASE << "\n";
    return false;
  }
  return true;
}

auto connect_redis() {
  std::cout << "[WORKER] "
    << "Worker task connecting to REDIS. "
    << "IP: " << REDIS_IP << std::endl;
  auto r = redis_connect(REDIS_IP, REDIS_PORT);
  check_redis(r);
  return r;
}

void LogisticTaskS3::run(const Configuration& config, int worker) {
  std::cout << "[WORKER] " << "Worker task connecting to store" << std::endl;

  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();

  // Create iterator that goes from 0 to num_s3_batches
  S3Iterator s3_iter(0, num_s3_batches, config,
      config.get_s3_size(), features_per_sample,
      config.get_minibatch_size());

  // we use redis
  auto r = connect_redis();
  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches << std::endl;
  wait_for_start(WORKER_TASK_RANK + worker, r, nworkers);

  uint64_t version = 0;
  while (1) {
    // maybe we can wait a few iterations to get the model
    std::shared_ptr<double> samples;
    std::shared_ptr<double> labels;
    LRModel model(MODEL_GRAD_SIZE);

    // get data, labels and model
    if (!run_phase1(samples, labels, model, r, s3_iter, features_per_sample))
      continue;

    Dataset dataset(samples.get(), labels.get(),
        samples_per_batch, features_per_sample);

    auto now = get_time_us();
    // compute mini batch gradient
    std::unique_ptr<ModelGradient> gradient;
    try {
      gradient = model.minibatch_grad(0, dataset.samples_,
          labels.get(), samples_per_batch, config.get_epsilon());
    } catch(...) {
      std::cout << "There was an error here" << std::endl;
      exit(-1);
    }
    auto elapsed_us = get_time_us() - now;
    std::cout << "[WORKER] Gradient compute time (us): " << elapsed_us
      << "at time: " << get_time_us()
      << " version " << version << "\n";
    gradient->setVersion(version++);

    try {
      LRGradient* lrg = dynamic_cast<LRGradient*>(gradient.get());
      if (lrg == nullptr) {
        throw std::runtime_error("Wrong dynamic cast");
      }

      push_gradient(r, worker, lrg);

    } catch(...) {
      std::cout << "[WORKER] "
        << "Worker task error doing put of gradient" << "\n";
      exit(-1);
    }
  }
}

