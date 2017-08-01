/* Copyright Joao Carreira 2017 */

#ifndef SRC_DATASET_H_
#define SRC_DATASET_H_

#include <vector>
#include <cstdint>
#include <memory>
#include <Matrix.h>

/**
  * This class is used to hold a dataset (both labels and samples)
  */
class Dataset {
 public:
    /**
      * Construct empty dataset
      */
    Dataset();

    /**
      * Construct a dataset given a vector of samples and a vector of labels
      * @param samples Vector of samples
      * @param labels Vector of labels
      */
    Dataset(const std::vector<std::vector<double>>& samples,
            const std::vector<double>& labels);
    /**
      * Construct a dataset given a vector of samples and a vector of labels
      * @param samples Vector of samples
      * @param labels Vector of labels
      * @param n_samples Nunber of samples in the dataset
      * @param n_labels Number of labels in the dataset
      */
    Dataset(const double* samples,
            const double* labels,
            uint64_t n_samples,
            uint64_t n_features);

    /**
      * Get the number of samples in this dataset
      * @return Number of samples in the dataset
      */
    uint64_t samples() const;

    /**
      * Get the number of features in this dataset
      * @return Number of features in the dataset
      */
    uint64_t features() const;

    /**
      * Returns pointer to specific sample in this dataset
      * @param sample Sample index
      * @returns Pointer to dataset sample
      */
    const double* sample(uint64_t sample) {
        return samples_.row(sample);
    }

 public:
    Matrix samples_;  //< dataset in matrix format
    std::shared_ptr<const double> labels_;  //< vector of labels
};

#endif  // SRC_DATASET_H_

