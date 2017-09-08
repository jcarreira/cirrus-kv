/**
  * Dataset is a class that is used to manage a dataset
  * where each sample is a vector of doubles and the labels are also doubles
  */

#include <examples/ml/Dataset.h>
#include <algorithm>
#include <Utils.h>
#include <Checksum.h>

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
    labels_.reset(labels);
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

