#ifndef EXAMPLES_ML_SPARSEDATASET_H_
#define EXAMPLES_ML_SPARSEDATASET_H_

#include <vector>
#include <cstdint>
#include <memory>
#include <config.h>

/**
  * This class is used to hold a sparse dataset
  * Each sample is a variable size list of pairs <int, FEATURE_TYPE>
  */
class SparseDataset {
  public:
  /**
   * Construct empty dataset
   */
  SparseDataset();

  /**
   * Construct a dataset given a vector of samples and a vector of labels
   * This method copies all the inputs
   * @param samples Vector of samples
   * @param labels Vector of labels
   */
  SparseDataset(const std::vector<std::vector<std::pair<int, FEATURE_TYPE>>>& samples);

  /**
   * Get the number of samples in this dataset
   * @return Number of samples in the dataset
   */
  uint64_t num_samples() const;

  /**
   * Get the number of features in this dataset
   * @return Number of features in the dataset
   */
  //uint64_t num_features() const;

  /**
   * Get the max number of features of any single user
   * @return Max number of features of any single user
   */
  uint64_t max_features() const;

  /**
   * Returns pointer to specific sample in this dataset
   * @param sample Sample index
   * @returns Pointer to dataset sample
   */
  //const FEATURE_TYPE* sample(uint64_t sample) const {
  //  return samples_.row(sample);
  //}

  /**
   * Sanity check values in the dataset
   */
  void check() const;

  /**
   * Compute checksum of values in the dataset
   * @return crc32 checksum
   */
  double checksum() const;

  /**
   * Print this dataset
   */
  void print() const;

  /**
   * Print some information about the dataset
   */
  void print_info() const;

  /** Build data for S3 object
   * from feature and label data from samples in range [l,r)
   */
  //std::shared_ptr<FEATURE_TYPE> build_s3_obj(uint64_t, uint64_t);

  /**
   * Return random subset of samples
   * @param n_samples Number of samples to return
   * @return Random subset of samples
   */
  SparseDataset random_sample(uint64_t n_samples) const;
  
  SparseDataset sample_from(uint64_t start, uint64_t n_samples) const;

  public:
  std::vector<std::vector<std::pair<int, FEATURE_TYPE>>> data_;
  uint64_t max_features_; // largest number of featuers of any single user
};

#endif  // EXAMPLES_ML_SPARSEDATASET_H_
