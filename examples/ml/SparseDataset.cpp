/**
  * SparseDataset is a class that is used to manage a sparse dataset
  */

#include <SparseDataset.h>
#include <algorithm>
#include <Utils.h>
#include <Checksum.h>

#include <cassert>

SparseDataset::SparseDataset() {
}

SparseDataset::SparseDataset(const std::vector<std::vector<std::pair<int, double>>>& samples) :
    data_(samples), max_features_(0) {

    max_features_ = std::max_element(samples.begin(), samples.end(),
        [](const std::vector<std::pair<int, double>>& a,
           const std::vector<std::pair<int, double>>& b) { return a.size() < b.size(); })->size();
}

uint64_t SparseDataset::num_samples() const {
    return data_.size();
}

void SparseDataset::check() const {
  for (const auto& w : data_) {
    for (const auto& v : w) {
      if (v.first < 0 || v.first > 1000000) {
        throw std::runtime_error("Input error");
      }

      double rating = v.second;
      if (rating < 0 || rating > 100) {
        throw std::runtime_error(
            "Dataset::check_values wrong label value: " + std::to_string(rating));
      }
      if (std::isnan(rating) || std::isinf(rating)) {
        throw std::runtime_error(
            "Dataset::check_values nan/inf error in labels");
      }
    }
  }
}

double SparseDataset::checksum() const {
  throw std::runtime_error("Not implemented");
  return 0;
}

void SparseDataset::print() const {
  std::cout << "SparseDataset" << std::endl;
  for (const auto& w : data_) {
    for (const auto& v : w) {
      std::cout << v.first << ":" << v.second << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void SparseDataset::print_info() const {
  std::cout << "SparseDataset #samples: " << data_.size() << std::endl;
  std::cout << "SparseDataset max features: " << max_features_ << std::endl;
}

//std::shared_ptr<Dataset::FEATURE_TYPE>
//Dataset::build_s3_obj(uint64_t l, uint64_t r) {
//  uint64_t num_samples = r - l;
//  uint64_t entries_per_sample = samples_.cols + 1;
//
//  std::shared_ptr<FEATURE_TYPE> s3_obj = std::shared_ptr<FEATURE_TYPE>(
//      new FEATURE_TYPE[num_samples * entries_per_sample],
//      std::default_delete<FEATURE_TYPE[]>());
//
//  std::cout << "entries_per_sample: " << entries_per_sample << std::endl;
//  for (uint64_t i = 0; i < num_samples; ++i) {
//    FEATURE_TYPE* d = s3_obj.get() + i * entries_per_sample;
//
//
//    // copy label
//    *d = labels_.get()[i];
//    if (!FLOAT_EQ(*d, 0.0) && !FLOAT_EQ(*d, 1.0))
//      throw std::runtime_error("Erorr in build_s3_obj");
//    d++;  // move to features
//
//    // copy features
//    const FEATURE_TYPE* start = samples_.row(l + i);
//    const FEATURE_TYPE* end = samples_.row(l + i) + samples_.cols + 1;
//    std::copy(start, end, d);
//  }
//
//  return s3_obj;
//}

SparseDataset SparseDataset::random_sample(uint64_t n_samples) const {
  std::random_device rd;
  std::default_random_engine re(rd());
  std::uniform_int_distribution<int> sampler(0, num_samples());

  std::vector<std::vector<std::pair<int, double>>> samples;

  for (uint64_t i = 0; i < n_samples; ++i) {
    int index = sampler(re);

    samples.push_back(data_[index]);
  }

  return SparseDataset(samples);
}

SparseDataset SparseDataset::sample_from(uint64_t start, uint64_t n_samples) const {
  std::vector<std::vector<std::pair<int, double>>> samples;

  if (start + n_samples > data_.size()) {
    throw std::runtime_error("Start goes over size of dataset");
  }

  for (uint64_t i = start; i < start + n_samples; ++i) {
    samples.push_back(data_[i]);
  }

  return SparseDataset(samples);
}

uint64_t SparseDataset::max_features() const {
  return max_features_;
}

