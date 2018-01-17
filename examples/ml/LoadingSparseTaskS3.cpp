#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "InputReader.h"
#include "S3.h"
#include "Utils.h"
#include "config.h"

#define READ_INPUT_THREADS (10)
SparseDataset LoadingSparseTaskS3::read_dataset(
    const Configuration& config) {
  InputReader input;

  std::string delimiter;
  if (config.get_input_type() == "csv_space") {
    delimiter = "";
  } else if (config.get_input_type() == "csv_tab") {
    delimiter = "\t";
  } else {
    throw std::runtime_error("unknown input type");
  }

  bool normalize = config.get_normalize();
  SparseDataset dataset = input.read_input_criteo_sparse(
      config.get_input_path(),
      delimiter,
      config.get_limit_samples(),
      //config.get_limit_cols(),
      normalize);
  return dataset;
}

void LoadingSparseTaskS3::check_label(FEATURE_TYPE label) {
  if (label != 1.0 && label != 0.0) {
    throw std::runtime_error("Wrong label value");
  }
}

/**
  * Check if loading was well done
  */
void LoadingSparseTaskS3::check_loading(
    auto& s3_client) {
  std::cout << "[LOADER] Trying to get sample with id: " << 0 << std::endl;

  std::string data = s3_get_object(SAMPLE_BASE, s3_client, S3_SPARSE_BUCKET);
  
  SparseDataset dataset(data.data(), data.size());
  dataset.check();
  dataset.check_labels();

  //std::cout << "[LOADER] Checking label values.." << std::endl;
  //check_label(sample.get()[0]);

  const auto& s = dataset.get_row(0);
  std::cout << "[LOADER] " << "Print sample 0 with size: " << s.size() << std::endl;
  for (const auto& feature : s) {
    int index = feature.first;
    FEATURE_TYPE value = feature.second;
    std::cout << index << "/" << value << " ";
  }

  for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
    //const auto& s = dataset.get_row(i);
    const auto& label = dataset.labels_[i];
    if (label != 0.0 && label != 1.0) {
      throw std::runtime_error("Wrong label");
    }
  }
}

/**
 * Load the object store with the training dataset
 * It reads from the criteo dataset files and writes to the object store
 * It signals when work is done by changing a bit in the object store
 */
void LoadingSparseTaskS3::run(const Configuration& config) {
  std::cout << "[LOADER-SPARSE] " << "Read criteo input..." << std::endl;

  Configuration new_config = config;
  //new_config.limit_samples = 1000000; // XXX fix this
  new_config.s3_size = 100000; // XXX fix this
  // number of samples in one s3 object 
  uint64_t s3_obj_num_samples = new_config.get_s3_size();
  // each object is going to have features and 1 label
  uint64_t s3_obj_sample_entries = features_per_sample + 1;
  uint64_t s3_obj_entries = s3_obj_num_samples * s3_obj_sample_entries;

  s3_initialize_aws();
  auto s3_client = s3_create_client();
 
  SparseDataset dataset = read_dataset(new_config);
  dataset.check();

  std::cout << "[LOADER-SPARSE] "
    << "Adding " << dataset.num_samples()
    << " samples in batches of size (s3_obj_entries): " << s3_obj_entries
    << std::endl;

  // For each S3 object (group of s3_obj_num_samples samples)
  uint64_t num_s3_objs = dataset.num_samples() / s3_obj_num_samples;
  for (unsigned int i = 0; i < num_s3_objs; ++i) {
    std::cout << "[LOADER-SPARSE] Building s3 batches" << std::endl;

    uint64_t first_sample = i * s3_obj_num_samples;
    uint64_t last_sample = (i + 1) * s3_obj_num_samples;

    uint64_t len;
    // this function already returns a nicely packed object
    std::shared_ptr<char> s3_obj = dataset.build_serialized_s3_obj(first_sample, last_sample, &len);

    std::cout << "Putting object in S3 with size: " << len << std::endl;
    s3_put_object(SAMPLE_BASE + i, s3_client, S3_SPARSE_BUCKET,
        std::string(s3_obj.get(), len));
  }

  check_loading(s3_client);

  std::cout << "LOADER-SPARSE terminated successfully" << std::endl;
}

