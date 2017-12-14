/**
  * Dataset is a class that is used to manage a dataset
  * where each sample is a vector of doubles and the labels are also doubles
  */

#include <examples/ml/Dataset.h>
#include <algorithm>
#include <Utils.h>
#include <Checksum.h>

#include <cassert>

Dataset::Dataset() {
}

Dataset::Dataset(const std::vector<std::vector<double>>& samples,
        const std::vector<double>& labels) :
    samples_(samples) {
    double* l = new double[labels.size()];
    std::copy(labels.data(), labels.data() + labels.size(), l);
    labels_.reset(l, array_deleter<double>);
}

Dataset::Dataset(const double* samples,
                 const double* labels,
                 uint64_t n_samples,
                 uint64_t n_features) :
    samples_(samples, n_samples, n_features) {
    double* l = new double[n_samples];
    std::copy(labels, labels + n_samples, l);
    labels_.reset(l, array_deleter<double>);
}

Dataset::Dataset(std::vector<std::shared_ptr<double>> samples,
                 std::vector<std::shared_ptr<double>> labels,
                 uint64_t samples_per_batch,
                 uint64_t features_per_sample) :
    samples_(samples, samples_per_batch, features_per_sample) {
  assert(labels.size() == samples.size());

  uint64_t num_labels = samples.size() * samples_per_batch; 
  double* all_labels = new double[num_labels];

  // copy labels in each minibatch sequentially
  for (uint64_t i = 0; i < labels.size(); ++i) {
    std::memcpy(
        all_labels + i * samples_per_batch,
        labels[i].get(),
        samples_per_batch * sizeof(double));
  }

  labels_.reset(all_labels, array_deleter<double>);
}

uint64_t Dataset::num_features() const {
    return samples_.cols;
}

uint64_t Dataset::num_samples() const {
    return samples_.rows;
}

void Dataset::check_values() const {
    const double* l = labels_.get();
    for (uint64_t i = 0; i < num_samples(); ++i) {
        if (std::isnan(l[i]) || std::isinf(l[i])) {
            throw std::runtime_error(
                    "Dataset::check_values nan/inf error in labels");
        }
    }
    samples_.check_values();
}

double Dataset::checksum() const {
    return crc32(labels_.get(), num_samples()) + samples_.checksum();
}

void Dataset::print() const {
    samples_.print();
}

std::shared_ptr<double> Dataset::build_s3_obj(uint64_t l, uint64_t r) {
  uint64_t num_samples = r - l;
  uint64_t entries_per_sample = samples_.cols + 1;

  std::shared_ptr<double> s3_obj = std::shared_ptr<double>(
      new double[num_samples * entries_per_sample],
      std::default_delete<double[]>());

  for (uint64_t i = 0; i < num_samples; ++i) {
    double* d = s3_obj.get() + i * entries_per_sample;

    // copy label
    *d = labels_.get()[i];
    d++; // move to features

    // copy features
    const double* start = samples_.row(l + i);
    const double* end = samples_.row(l + i) + samples_.cols;
    std::copy(start, end, d);
  }

  return s3_obj;
}

