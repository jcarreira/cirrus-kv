#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"

void LogisticTask::run(const Configuration& config, int worker) {
  std::cout << "[WORKER] "
    << "Worker task connecting to store" << std::endl;

  lr_gradient_serializer lgs(MODEL_GRAD_SIZE);
  lr_gradient_deserializer lgd(MODEL_GRAD_SIZE);
  lr_model_serializer lms(MODEL_GRAD_SIZE);
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);
  c_array_serializer<FEATURE_TYPE> cas_samples(batch_size);
  c_array_deserializer<FEATURE_TYPE> cad_samples(batch_size,
      "worker samples_store");
  c_array_serializer<FEATURE_TYPE> cas_labels(samples_per_batch);
  c_array_deserializer<FEATURE_TYPE> cad_labels(samples_per_batch,
      "worker labels_store", false);

#ifdef USE_CIRRUS
  std::cout << "[WORKER] "
    << "Worker task using CIRRUS" << std::endl;
  cirrus::TCPClient client;
  // used to publish the gradient
  cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
    gradient_store(IP, PORT, &client, lgs, lgd);

  // this is used to access the most up to date model
  cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
    model_store(IP, PORT, &client, lms, lmd);
#elif defined(USE_REDIS)
  std::cout << "[WORKER] "
    << "Worker task connecting to REDIS. "
    << "IP: " << REDIS_IP << std::endl;
  auto r  = redis_connect(REDIS_IP, REDIS_PORT);

  if (r == NULL || r -> err) {
    std::cout << "[WORKER] "
      << "Error connecting to REDIS"
      << " IP: " << REDIS_IP
      << std::endl;
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
#endif


#ifdef PRELOAD_DATA
  std::cout << "PRELOADING is set. Should use other task code" << std::endl;
  exit(-1);
#endif

  uint64_t num_batches = config.get_limit_samples() /
    config.get_minibatch_size();
  std::cout << "[WORKER] "
    << "num_batches: " << num_batches << std::endl;

#ifdef USE_PREFETCH
  RedisIterator rit_samples(
      SAMPLE_BASE, SAMPLE_BASE + 121, REDIS_IP, REDIS_PORT);
  RedisIterator rit_labels(
      LABEL_BASE, LABEL_BASE + 121, REDIS_IP, REDIS_PORT);
#endif

#ifdef USE_CIRRUS
  // this is used to access the training data sample
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<FEATURE_TYPE>>
    samples_store(IP, PORT, &client, cas_samples, cad_samples);
  cirrus::LRAddedEvictionPolicy samples_policy(6);
  cirrus::CacheManager<std::shared_ptr<FEATURE_TYPE>> samples_cm(
      &samples_store, &samples_policy, 6);
  cirrus::CirrusIterable<std::shared_ptr<FEATURE_TYPE>> s_iter(
      &samples_cm, READ_AHEAD, SAMPLE_BASE,
      SAMPLE_BASE + num_batches - 1);

  auto samples_iter = s_iter.begin();

  // this is used to access the training labels
  // we configure this store to return shared_ptr that do not free memory
  // because these objects will be owned by the Dataset
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<FEATURE_TYPE>>
    labels_store(IP, PORT, &client,
        cas_labels, cad_labels);
  cirrus::LRAddedEvictionPolicy labels_policy(6);
  cirrus::CacheManager<std::shared_ptr<FEATURE_TYPE>> labels_cm(
      &labels_store, &labels_policy, 6);
  cirrus::CirrusIterable<std::shared_ptr<FEATURE_TYPE>> l_iter(
      &labels_cm, READ_AHEAD, LABEL_BASE,
      LABEL_BASE + num_batches - 1);
  auto labels_iter = l_iter.begin();
#elif defined(USE_REDIS)
#endif

#ifdef USE_CIRRUS
  wait_for_start(WORKER_TASK_RANK + worker, client, nworkers);
#elif defined(USE_REDIS)
  wait_for_start(WORKER_TASK_RANK + worker, r, nworkers);
#endif

  bool first_time = true;

  uint64_t batch_id = 0;
  int gradient_id = GRADIENT_BASE + worker;
  uint64_t version = 0;
  while (1) {
    // maybe we can wait a few iterations to get the model
    std::shared_ptr<FEATURE_TYPE> samples;
    std::shared_ptr<FEATURE_TYPE> labels;
    LRModel model(MODEL_GRAD_SIZE);
    try {
#ifdef DEBUG
      std::cout << "[WORKER] "
        << "Worker task getting the model at id: "
        << MODEL_BASE
        << "\n";
#endif

      auto before_model = get_time_ns();
#ifdef USE_CIRRUS
      model = model_store.get(MODEL_BASE);
#elif defined(USE_REDIS)
      int len;
      char* data = redis_get_numid(r, MODEL_BASE, &len);
      if (data == nullptr) {
        throw cirrus::NoSuchIDException("");
      }
      model = lmd(data, len);
      free(data);
#endif
      auto after_model = get_time_ns();
      std::cout << "[WORKER] "
        << "model get (ns): " << (after_model - before_model)
        << std::endl;

#ifdef DEBUG
      std::cout << "[WORKER] "
        << "Worker task received model csum: " << model.checksum()
        << std::endl
        << "Worker task getting the training data with id: "
        << (SAMPLE_BASE + batch_id)
        << "\n";
#endif
      std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

#ifdef USE_CIRRUS
      samples = *samples_iter;
#elif defined USE_PREFETCH
      int len_samples = batch_size * sizeof(FEATURE_TYPE);
      data = rit_samples.get_next();
      samples = cad_samples(data, len_samples);
      free(data);
#elif defined(USE_REDIS)
      int len_samples;
      data = redis_get_numid(r, SAMPLE_BASE + batch_id, &len_samples);
      if (data == nullptr) {
        throw cirrus::NoSuchIDException("");
      }
      samples = cad_samples(data, len_samples);
      free(data);
#endif
      std::chrono::steady_clock::time_point finish =
        std::chrono::steady_clock::now();
      uint64_t elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            finish-start).count();
      double bw = 1.0 * batch_size * sizeof(FEATURE_TYPE) /
        elapsed_ns * 1000.0 * 1000 * 1000 / 1024 / 1024;
#ifdef USE_CIRRUS
      std::cout << "Get Sample " << batch_id << " Elapsed (CIRRUS) "
#elif defined(USE_REDIS)
        std::cout << "Get Sample " << batch_id << " Elapsed (REDIS) "
#endif
        << " batch size: " << batch_size
        << " len samples: " << len_samples
        << " ns: " << elapsed_ns
        << " BW (MB/s): " << bw
        << "at time: " << get_time_us()
        << "\n";
#ifdef DEBUG
      std::cout << "[WORKER] "
        << "Worker task received training data with id: "
        << (SAMPLE_BASE + batch_id)
        << " and checksum: " << checksum(samples, batch_size)
        << "\n";
#endif

      auto before_labels = get_time_ns();
#ifdef USE_CIRRUS
      labels = *labels_iter;
#elif defined USE_PREFETCH
      data = rit_labels.get_next();
      int len_labels = batch_size * sizeof(FEATURE_TYPE) / 10;
      labels = cad_labels(data, len_labels);
      free(data);
#elif defined(USE_REDIS)
      int len_labels;
      data = redis_get_numid(r, LABEL_BASE + batch_id, &len_labels);
      if (data == nullptr) {
        throw cirrus::NoSuchIDException("");
      }
      labels = cad_labels(data, len_labels);
      free(data);
#endif
      auto after_labels = get_time_ns();
      std::cout << "[WORKER] "
        << "labels get (ns): " << (after_labels - before_labels)
        << "at time: " << get_time_us()
        << "\n";

#ifdef DEBUG
      std::cout << "[WORKER] "
        << "Worker task received label data with id: "
        << batch_id
        << " and checksum: " << checksum(labels, samples_per_batch)
        << "\n";
#endif
    } catch(const cirrus::NoSuchIDException& e) {
      if (!first_time) {
        std::cout << "[WORKER] "
          << "Exiting"
          << std::endl;
        exit(0);

        // XXX fix this. We should be able to distribute
        // how many samples are there

        // wrap around
        batch_id = 0;
        continue;
      }
      // this happens because the ps task
      // has not uploaded the model yet
      std::cout << "[WORKER] "
        << "Model could not be found at id: "
        << MODEL_BASE
        << "\n";
      continue;
    }

    first_time = false;

#ifdef DEBUG
    std::cout << "[WORKER] "
      << "Building and printing dataset"
      << " with checksums: "
      << checksum(samples, batch_size) << " "
      << checksum(labels, samples_per_batch)
      << "\n";
#endif

    // Big hack. Shame on me
    // auto now = get_time_us();
    std::shared_ptr<FEATURE_TYPE> l(new FEATURE_TYPE[samples_per_batch],
        ml_array_nodelete<FEATURE_TYPE>);
    std::copy(labels.get(), labels.get() + samples_per_batch, l.get());
    // std::cout << "Elapsed: " << get_time_us() - now << "\n";

    Dataset dataset(samples.get(), l.get(),
        samples_per_batch, features_per_sample);
#ifdef DEBUG
    dataset.print();

    std::cout << "[WORKER] "
      << "Worker task checking dataset with csum: " << dataset.checksum()
      << "\n";

    dataset.check_values();

    std::cout << "[WORKER] "
      << "Worker task computing gradient"
      << "\n";
#endif

    auto now = get_time_us();
    // compute mini batch gradient
    std::unique_ptr<ModelGradient> gradient;
    try {
      gradient = model.minibatch_grad(dataset.samples_,
          labels.get(), samples_per_batch, config.get_epsilon());
    } catch(...) {
      std::cout << "There was an error here" << std::endl;
      exit(-1);
    }
    auto elapsed_us = get_time_us() - now;
    std::cout << "[WORKER] "
      << "Gradient compute time (us): " << elapsed_us
      << "at time: " << get_time_us()
      << " version " << version
      << "\n";
    gradient->setVersion(version++);

#ifdef DEBUG
    std::cout << "[WORKER] "
      << "Checking and Printing gradient: "
      << "\n";
#endif

#ifdef DEBUG
    gradient->check_values();
    gradient->print();

    std::cout << "[WORKER] "
      << "Worker task storing gradient at id: "
      << gradient_id
      << "\n";
#endif
    try {
      auto lrg = dynamic_cast<LRGradient*>(gradient.get());
      if (lrg == nullptr) {
        throw std::runtime_error("Wrong dynamic cast");
      }
#ifdef USE_CIRRUS
      gradient_store.put(
          gradient_id, *lrg);
#elif defined(USE_REDIS)
      auto data = std::unique_ptr<char[]>(
          new char[lgs.size(*lrg)]);
      lgs.serialize(*lrg, data.get());
      redis_put_binary_numid(r, gradient_id, data.get(), lgs.size(*lrg));
#endif
    } catch(...) {
      std::cout << "[WORKER] "
        << "Worker task error doing put of gradient"
        << "\n";
      exit(-1);
    }
    std::cout << "[WORKER] "
      << "Worker task stored gradient at id: " << gradient_id
      << " at time (us): " << get_time_us()
      << "\n";

    // move to next batch of samples
    batch_id++;
#ifdef USE_CIRRUS
    samples_iter++;
    labels_iter++;
#endif

    // Wrap around
    if (batch_id == SAMPLE_BASE + num_batches) {
      batch_id = 0;
#ifdef USE_CIRRUS
      samples_iter = s_iter.begin();
      labels_iter = l_iter.begin();
#endif
    }
  }
}

