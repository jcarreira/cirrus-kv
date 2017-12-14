#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Input.h"
#include "S3.h"

/**
  * Load the object store with the training dataset
  * It reads from the criteo dataset files and writes to the object store
  * It signals when work is done by changing a bit in the object store
  */
void LoadingTask::run(const Configuration& config) {
  std::cout << "[LOADER] " << "Read criteo input..." << std::endl;

  Input input;
  // auto dataset = input.read_input_csv(
  //        config.get_input_path(),
  //        " ", 3,
  //        config.get_limit_cols(), false);  // data is already normalized
  bool normalize = config.get_normalize();
  auto dataset = input.read_input_csv(
      config.get_input_path(),
      " ", 10,
      config.get_limit_samples(),
      config.get_limit_cols(), normalize);

  dataset.check_values();
#ifdef DEBUG
  dataset.print();
#endif

  c_array_serializer<double> cas_samples(batch_size);
  c_array_deserializer<double> cad_samples(batch_size,
      "loader samples_store");
  c_array_serializer<double> cas_labels(samples_per_batch);
  c_array_deserializer<double> cad_labels(samples_per_batch,
      "loader labels_store", false);


#ifdef DATASET_IN_S3
  s3_initialize_aws();
  auto s3_client = s3_create_client();
#endif
#if defined(USE_CIRRUS)
  cirrus::TCPClient client;
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
    samples_store(IP, PORT, &client, cas_samples, cad_samples);
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
    labels_store(IP, PORT, &client, cas_labels, cad_labels);
#elif defined(USE_REDIS)
  auto r  = redis_connect(REDIS_IP, REDIS_PORT);
  if (r == NULL || r -> err) {
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
#endif

  std::cout << "[LOADER] "
    << "Adding " << dataset.num_samples()
    << " samples in batches of size (samples*features): "
    << batch_size << std::endl;

  // We put in batches of N samples
  for (unsigned int i = 0;
      i < dataset.num_samples() / samples_per_batch; ++i) {
    std::cout << "[LOADER] "
      << "Building samples batch. Batch size: " << batch_size << std::endl;
    /** Build sample object
    */
    auto sample = std::shared_ptr<double>(
        new double[batch_size],
        std::default_delete<double[]>());
    // this memcpy can be avoided with some trickery
    std::memcpy(sample.get(), dataset.sample(i * samples_per_batch),
        sizeof(double) * batch_size);

    /**
     * Build label object
     */
    auto label = std::shared_ptr<double>(
        new double[samples_per_batch],
        std::default_delete<double[]>());
    // this memcpy can be avoided with some trickery
    std::memcpy(label.get(), dataset.label(i * samples_per_batch),
        sizeof(double) * samples_per_batch);

    try {
      std::cout << "[LOADER] "
        << "Adding sample batch id: "
        << (SAMPLE_BASE + i)
        << " samples with size (bytes): " << sizeof(double) * batch_size
        << std::endl;
#ifdef DATASET_IN_S3
      {
        std::cout << "Adding sample to s3" << std::endl;
        uint64_t len = cas_samples.size(sample);
        auto data = std::unique_ptr<char[]>(
            new char[len]);
        cas_samples.serialize(sample, data.get());
        s3_put_object(SAMPLE_BASE + i, s3_client,
            "cirrusonlambdas",
            std::string(data.get(), cas_samples.size(sample)));
      }
#elif defined(USE_CIRRUS)
      samples_store.put(SAMPLE_BASE + i, sample);
#elif defined(USE_REDIS)
      {
        uint64_t len = cas_samples.size(sample);
        auto data = std::unique_ptr<char[]>(
            new char[len]);
        cas_samples.serialize(sample, data.get());
        redis_put_binary_numid(r, SAMPLE_BASE + i,
            data.get(), cas_samples.size(sample));
      }
#endif

#ifdef DATASET_IN_S3
      {
        std::cout << "Adding label to s3" << std::endl;
        auto data = std::unique_ptr<char>(
            new char[cas_labels.size(label)]);
        cas_labels.serialize(label, data.get());
        s3_put_object(LABEL_BASE + i, s3_client,
            "cirrusonlambdas",
            std::string(data.get(), cas_labels.size(label)));
      }
#elif defined(USE_CIRRUS)
      std::cout << "[LOADER] "
        << "Putting label"
        << std::endl;
      labels_store.put(LABEL_BASE + i, label);
#elif defined(USE_REDIS)
      std::cout << "[LOADER] "
        << "Storing labels on REDIS with size: "
        << cas_labels.size(label)
        << std::endl;
      {
        auto data = std::unique_ptr<char>(
            new char[cas_labels.size(label)]);
        cas_labels.serialize(label, data.get());
        redis_put_binary_numid(r, LABEL_BASE + i,
                         data.get(), cas_labels.size(label));
      }
#endif
    } catch(...) {
      std::cout << "[LOADER] "
        << "Caught exception" << std::endl;
      exit(-1);
    }
  }

  std::cout << "[LOADER] "
    << "Trying to get sample with id: " << 0
    << std::endl;

#ifdef USE_CIRRUS
  auto sample = samples_store.get(0);
#elif defined(USE_REDIS)
  int len_samples;
  char* data = redis_get_numid(r, 0, &len_samples);
  auto sample = cad_samples(data, len_samples);
  free(data);
  int len_label;

  data = nullptr;
  while (data == nullptr) {
    data = redis_get_numid(r, LABEL_BASE, &len_label);
  }
  free(data);
  std::cout << "[LOADER] "
    << "Got label with size: " << len_label << std::endl;
#endif

  std::cout << "[LOADER] "
    << "Got sample with id: " << 0
    << "Added all samples"
    << std::endl;

#ifdef DEBUG
  std::cout << "[LOADER] "
    << "Print sample 0"
    << std::endl;

  for (unsigned int i = 0; i < batch_size; ++i) {
    double val = sample.get()[i];
    std::cout << val << " ";
  }
#endif
  std::cout << std::endl;

#ifdef USE_CIRRUS
  wait_for_start(LOADING_TASK_RANK, client, nworkers);
#elif defined(USE_REDIS)
  wait_for_start(LOADING_TASK_RANK, r, nworkers);
#endif
}

