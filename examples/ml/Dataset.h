#ifndef EXAMPLES_ML_DATASET_H_
#define EXAMPLES_ML_DATASET_H_

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
      * This method copies all the inputs
      * @param samples Vector of samples
      * @param labels Vector of labels
      */
    Dataset(const std::vector<std::vector<double>>& samples,
            const std::vector<double>& labels);
    /**
      * Construct a dataset given a vector of samples and a vector of labels
      * This method copies all the inputs
      * @param samples Vector of samples
      * @param labels Vector of labels
      * @param n_samples Number of samples in the dataset
      * @param n_features Number of features on each sample
      */
    Dataset(const double* samples,
            const double* labels,
            uint64_t n_samples,
            uint64_t n_features);

    /**
      * Construct a dataset given a vector of minibatches (samples and labels)
      * This method copies all the inputs
      * @param samples Vector of minibatches samples
      * @param labels Vector of minibatches labels
      * @param n_samples Number of samples in the dataset
      * @param n_features Number of features on each sample
      */
    Dataset(std::vector<std::shared_ptr<double>> samples,
            std::vector<std::shared_ptr<double>> labels,
            uint64_t n_samples,
            uint64_t n_features);

    /**
      * Get the number of samples in this dataset
      * @return Number of samples in the dataset
      */
    uint64_t num_samples() const;

    /**
      * Get the number of features in this dataset
      * @return Number of features in the dataset
      */
    uint64_t num_features() const;

    /**
      * Returns pointer to specific sample in this dataset
      * @param sample Sample index
      * @returns Pointer to dataset sample
      */
    const double* sample(uint64_t sample) const {
        return samples_.row(sample);
    }

    /**
      * Get pointer to label
      * @return point to label data index by input label
      */
    const double* label(uint64_t label_index) const {
        return labels_.get() + label_index;
    }

    /**
      * Sanity check values in the dataset
      */
    void check_values() const;

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
    std::shared_ptr<double> build_s3_obj(uint64_t, uint64_t);

 public:
    Matrix samples_;  //< dataset in matrix format
    std::shared_ptr<const double> labels_;  //< vector of labels
};

#endif  // EXAMPLES_ML_DATASET_H_
