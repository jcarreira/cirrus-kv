#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Input.h"
#include "S3.h"

Dataset LoadingTaskS3::read_dataset(
    const Configuration& config) {
  Input input;
  bool normalize = config.get_normalize();
  Dataset dataset = input.read_input_csv(
      config.get_input_path(),
      " ", 10,
      config.get_limit_samples(),
      config.get_limit_cols(), normalize);
  dataset.check_values();
  return dataset;
}

auto LoadingTaskS3::connect_redis() {
  auto r = redis_connect(REDIS_IP, REDIS_PORT);
  if (r == NULL || r -> err) {
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
  return r;
}

/**
  * Check if loading was well done
  */
void LoadingTaskS3::check_loading(
    auto& s3_client,
    uint64_t s3_obj_entries) {
  std::cout << "[LOADER] Trying to get sample with id: " << 0 << std::endl;

  std::string data = s3_get_object(SAMPLE_BASE, s3_client, "cirrusonlambdas");
  
  c_array_deserializer<double> cad_samples(
      s3_obj_entries,
      "loader samples_store");
  std::shared_ptr<double> sample = cad_samples(data.data(), data.size());

  double label = sample.get()[0];
  if (label != 1.0 && label != 0.0) {
    throw std::runtime_error("Wrong label value");
  }

  std::cout << "[LOADER] "
    << "Got sample with id: " << 0
    << "Added all samples"
    << std::endl;

#ifdef DEBUG
  std::cout << "[LOADER] "
    << "Print sample 0"
    << std::endl;

  for (unsigned int i = 1; i <= features_per_sample; ++i) {
    double val = sample.get()[i];
    std::cout << val << " ";
  }
#endif
  std::cout << std::endl;
}


/**
 * Load the object store with the training dataset
 * It reads from the criteo dataset files and writes to the object store
 * It signals when work is done by changing a bit in the object store
 */
void LoadingTaskS3::run(const Configuration& config) {
  std::cout << "[LOADER] " << "Read criteo input..." << std::endl;

  // number of samples in one s3 object 
  uint64_t s3_obj_num_samples = config.get_s3_size();
  // each object is going to have features and 1 label
  uint64_t s3_obj_sample_entries = features_per_sample + 1;
  uint64_t s3_obj_entries = s3_obj_num_samples * s3_obj_sample_entries;

  Dataset dataset = read_dataset(config);

  c_array_serializer<double> cas_samples(s3_obj_entries);
  s3_initialize_aws();
  auto s3_client = s3_create_client();
  auto r  = connect_redis();

  std::cout << "[LOADER] "
    << "Adding " << dataset.num_samples()
    << " samples in batches of size (s3_obj_entries): "
    << s3_obj_entries << std::endl;

  // For each S3 object (group of s3_obj_num_samples samples)
  uint64_t num_s3_objs = dataset.num_samples() / s3_obj_num_samples;
  for (unsigned int i = 0; i < num_s3_objs; ++i) {
    std::cout << "[LOADER] Building s3 batches" << std::endl;

    uint64_t first_sample = i * s3_obj_num_samples;
    uint64_t right_sample = (i+1) * s3_obj_num_samples;
    std::shared_ptr<double> s3_obj = dataset.build_s3_obj(first_sample, right_sample);

    // serialize and write data into s3
    uint64_t len = cas_samples.size(s3_obj);
    std::unique_ptr<char[]> data = std::unique_ptr<char[]>(new char[len]);
    cas_samples.serialize(s3_obj, data.get());
    s3_put_object(SAMPLE_BASE + i, s3_client, "cirrusonlambdas",
        std::string(data.get(), cas_samples.size(s3_obj)));
  }

  check_loading(s3_client, s3_obj_entries);
  wait_for_start(LOADING_TASK_RANK, r, nworkers);
}

