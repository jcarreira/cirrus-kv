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
  c_array_serializer<double> cas_samples(batch_size);
  c_array_deserializer<double> cad_samples(batch_size,
      "worker samples_store");
  c_array_serializer<double> cas_labels(samples_per_batch);
  c_array_deserializer<double> cad_labels(samples_per_batch,
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
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
    samples_store(IP, PORT, &client, cas_samples, cad_samples);
  cirrus::LRAddedEvictionPolicy samples_policy(6);
  cirrus::CacheManager<std::shared_ptr<double>> samples_cm(
      &samples_store, &samples_policy, 6);
  cirrus::CirrusIterable<std::shared_ptr<double>> s_iter(
      &samples_cm, READ_AHEAD, SAMPLE_BASE,
      SAMPLE_BASE + num_batches - 1);

  auto samples_iter = s_iter.begin();

  // this is used to access the training labels
  // we configure this store to return shared_ptr that do not free memory
  // because these objects will be owned by the Dataset
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
    labels_store(IP, PORT, &client,
        cas_labels, cad_labels);
  cirrus::LRAddedEvictionPolicy labels_policy(6);
  cirrus::CacheManager<std::shared_ptr<double>> labels_cm(
      &labels_store, &labels_policy, 6);
  cirrus::CirrusIterable<std::shared_ptr<double>> l_iter(
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
    std::shared_ptr<double> samples;
    std::shared_ptr<double> labels;
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
      int len_samples = batch_size * sizeof(double);
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
      double bw = 1.0 * batch_size * sizeof(double) /
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
      int len_labels = batch_size * sizeof(double) / 10;
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
    std::shared_ptr<double> l(new double[samples_per_batch],
        ml_array_nodelete<double>);
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

void LogisticTaskPreloaded::get_data_samples(
    auto r, uint64_t left_id, uint64_t right_id, auto& samples, auto& labels) {
  std::cout << "get_data_samples" << std::endl;

  c_array_serializer<double> cas_samples(batch_size);
  c_array_deserializer<double> cad_samples(batch_size,
      "worker samples_store");
  c_array_serializer<double> cas_labels(samples_per_batch);
  c_array_deserializer<double> cad_labels(samples_per_batch,
      "error labels_store", false);

  uint64_t size = right_id - left_id;
  samples.resize(size);
  labels.resize(size);
  for (uint64_t i = left_id; i < right_id; ++i) {
    // get samples
    int len_samples;
    std::cout << "Getting batch i: " << i << std::endl;
    char* data = redis_get_numid(r, SAMPLE_BASE + i, &len_samples);
    if (data == nullptr) {
      std::cout << "Sample batch " << i << " not found" << std::endl;
      i--;
      continue;
    }
    std::cout << "Got batch i: " << i << std::endl;
    auto sample = cad_samples(data, len_samples);
    std::cout << "Deserialized sample i: " << i << std::endl;
    samples[i - left_id] = sample;
    std::cout << "Assigned sample i: " << i << std::endl;
    free(data);
    std::cout << "Freed dat i: " << i << std::endl;

    int len_labels;
    std::cout << "Getting label i: " << i << std::endl;
    data = redis_get_numid(r, LABEL_BASE + i, &len_labels);
    if (data == nullptr) {
      std::cout << "Label batch " << i << " not found" << std::endl;
      i--;
      continue;
    }
    auto label = cad_labels(data, len_labels);
    labels[i - left_id] = label;
    free(data);
  }
  std::cout << "get_data_samples done" << std::endl;
}

void LogisticTaskPreloaded::run(const Configuration& config, int worker) {
  std::cout << "[WORKER-PRELOADED] "
    << "Worker task id: " << worker
    << "connecting to store" << std::endl;

  lr_gradient_serializer lgs(MODEL_GRAD_SIZE);
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);

#if defined(USE_REDIS)
  std::cout << "[WORKER-PRELOADED] "
    << "Worker task using REDIS" << std::endl;
  auto r  = redis_connect(REDIS_IP, REDIS_PORT);
  if (r == NULL || r -> err) {
    std::cout << "[WORKER-PRELOADED] "
      << "Error connecting to REDIS" << std::endl;
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
#else
  std::cout << "USE_S3 not supported" << std::endl;
  exit(-1);
#endif

  uint64_t num_batches = config.get_limit_samples() /
    config.get_minibatch_size();
  std::cout << "[WORKER-PRELOADED] "
    << "num_batches: " << num_batches << std::endl;

  std::vector<std::shared_ptr<double>> samples_preloaded;
  std::vector<std::shared_ptr<double>> labels_preloaded;

  wait_for_start(WORKER_TASK_RANK + worker, r, nworkers);

  uint64_t left_id = (worker_id - 4) * (num_batches / nworkers);
  uint64_t right_id = (worker_id - 4 + 1) * (num_batches / nworkers);
  std::cout << "[WORKER-PRELOADED] "
    << "preloading data.."
    << " left_id: " << left_id
    << " right_id: " << right_id
    << " num_batches: " << num_batches
    << " nworkers: " << nworkers
    << std::endl;
  std::cout << "[WORKER-PRELOADED] "
    << "preloading2 data.."
    <<std::endl;
  get_data_samples(r, left_id, right_id, samples_preloaded, labels_preloaded);
  std::cout << "[WORKER-PRELOADED] "
    << "preloaded data"
    << std::endl;

  bool first_time = true;

  uint64_t batch_id = SAMPLE_BASE + left_id;
  int gradient_id = GRADIENT_BASE + worker;
  uint64_t version = 0;
  while (1) {
    // maybe we can wait a few iterations to get the model
    LRModel model(MODEL_GRAD_SIZE);
    try {
      std::cout << "[WORKER] "
        << "Worker task getting the model at id: "
        << MODEL_BASE
        << "\n";

      auto before_model = get_time_ns();
      // get model
      int len;
      char* data = redis_get_numid(r, MODEL_BASE, &len);
      std::cout << "[WORKER] "
        << "Worker task got the model at id: "
        << MODEL_BASE
        << " data: " << reinterpret_cast<uint64_t>(data)
        << std::endl;
      if (data == nullptr) {
        throw cirrus::NoSuchIDException("");
      }
      std::cout << "[WORKER] "
        << "Worker task deserializing the model at id: "
        << MODEL_BASE
        << std::endl;
      model = lmd(data, len);
      std::cout << "[WORKER] "
        << "Worker task freeing the model at id: "
        << MODEL_BASE
        << std::endl;
      free(data);
      //-----------
      auto after_model = get_time_ns();
      std::cout << "[WORKER] "
        << "model get (ns): " << (after_model - before_model)
        << std::endl;
    } catch(const cirrus::NoSuchIDException& e) {
      if (!first_time) {
        std::cout << "[WORKER] "
          << "Exiting"
          << std::endl;
        exit(0);

        // wrap around
        batch_id = SAMPLE_BASE + left_id;
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

    std::cout << "[WORKER] "
      << "Getting sample and label"
      << std::endl;
    auto samples = samples_preloaded[batch_id - SAMPLE_BASE - left_id];
    auto labels = labels_preloaded[batch_id - SAMPLE_BASE - left_id];

    first_time = false;

    // Big hack. Shame on me
    std::shared_ptr<double> l(new double[samples_per_batch],
        ml_array_nodelete<double>);
    std::copy(labels.get(), labels.get() + samples_per_batch, l.get());

    std::cout << "[WORKER] "
      << "Building dataset"
      << std::endl;
    Dataset dataset(samples.get(), l.get(),
        samples_per_batch, features_per_sample);

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
      << " version " << version
      << "\n";
    gradient->setVersion(version++);

    try {
      auto lrg = dynamic_cast<LRGradient*>(gradient.get());
      if (lrg == nullptr) {
        throw std::runtime_error("Wrong dynamic cast");
      }
#ifdef USE_REDIS
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

    // Wrap around
    //if (samples_iter == s_iter.end()) {
    if (batch_id == SAMPLE_BASE + right_id) {
      batch_id = SAMPLE_BASE + left_id;
    }
  }
}

