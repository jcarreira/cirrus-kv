/* Copyright Joao Carreira 2017 */

/**
  * Dataset is a class that is used to manage a dataset
  * where each sample is a vector of doubles and the labels are also doubles
  */

#include <Dataset.h>
#include <algorithm>
#include <Utils.h>

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

uint64_t Dataset::features() const {
    return samples_.cols();
}

uint64_t Dataset::samples() const {
    return samples_.rows();
}

