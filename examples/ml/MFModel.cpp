#include <MFModel.h>
#include <Utils.h>
#include <MlUtils.h>
#include <Eigen/Dense>
#include <utils/Log.h>
#include <Checksum.h>
#include <algorithm>
#include <ModelGradient.h>

#define DEBUG

// FORMAT
// Number of users (32bits)
// Number of factors (32bits)
// Sample1: factor 1 (FEATURE_TYPE) | factor 2 | factor 3 
// Sample2 : ...
// ....

void MFModel::initialize_weights(uint64_t users, uint64_t items, uint64_t nfactors) {
  user_weights_ = std::shared_ptr<FEATURE_TYPE>(
      new FEATURE_TYPE[users * nfactors], std::default_delete<FEATURE_TYPE[]>());
  item_weights_ = std::shared_ptr<FEATURE_TYPE>(
      new FEATURE_TYPE[items * nfactors], std::default_delete<FEATURE_TYPE[]>());

  user_bias_.resize(users);
  item_bias_.resize(items);

  item_fact_reg_ = 0.01;
  user_fact_reg_ = 0.01;

  user_bias_reg_ = 0.01;
  item_bias_reg_ = 0.01;

  nusers_ = users;
  nitems_ = items;
  nfactors_ = nfactors;

  randomize();
}

MFModel::MFModel(uint64_t users, uint64_t items, uint64_t nfactors) {
    initialize_weights(users , items, nfactors);
}

MFModel::MFModel(const void* /*w*/, uint64_t /*n*/, uint64_t /*d*/) {
  throw std::runtime_error("Not implemented");
}

uint64_t MFModel::size() const {
  throw std::runtime_error("Not implemented");
  return 0;
}

std::unique_ptr<Model> MFModel::deserialize(void* /*data*/, uint64_t /*size*/) const {
  throw std::runtime_error("Not implemented");
  //uint32_t* data_p = (uint32_t*)data;
  ////uint32_t users = *data_p++;
  ////uint32_t items = *data_p++;
  ////uint32_t nfactors = *data_p++;

  ////return std::make_unique<MFModel>(
  ////        reinterpret_cast<void*>(data_p), users, items, nfactors);
  //return std::make_unique<MFModel>(
  //    reinterpret_cast<void*>(data_p), 10, 10);
}

std::pair<std::unique_ptr<char[]>, uint64_t>
MFModel::serialize() const {
    std::pair<std::unique_ptr<char[]>, uint64_t> res;
    uint64_t size = getSerializedSize();
    
    res.first.reset(new char[size]);
    res.second = size;

    serializeTo(res.first.get());
    return res;
}

void MFModel::serializeTo(void* mem) const {
    uint32_t* data_u = reinterpret_cast<uint32_t*>(mem);

    *data_u = nusers_;
    data_u++;
    *data_u = nitems_;
    data_u++;
    *data_u = nfactors_;
    data_u++;

    FEATURE_TYPE* data_d = reinterpret_cast<FEATURE_TYPE*>(data_u);
    std::copy(user_weights_.get(), user_weights_.get() + nusers_ * nfactors_, data_d);

    data_d += nusers_ * nfactors_;

    std::copy(item_weights_.get(), item_weights_.get() + nitems_ * nfactors_, data_d);
}

/**
  * We probably want to put 0 in the values we don't know
  */
void MFModel::randomize() {
  std::default_random_engine generator;
  std::normal_distribution<FEATURE_TYPE> distribution(0, 1.0 / nfactors_); // mean 0 and stddev=1
  for (uint64_t i = 0; i < nusers_; ++i) {
    for (uint64_t j = 0; j < nfactors_; ++j) {
      get_user_weights(i, j) = distribution(generator);
    }
  }
  for (uint64_t i = 0; i < nitems_; ++i) {
    for (uint64_t j = 0; j < nfactors_; ++j) {
      get_item_weights(i, j) = distribution(generator);
    }
  }
}

std::unique_ptr<Model> MFModel::copy() const {
    std::unique_ptr<MFModel> new_model =
        std::make_unique<MFModel>(nusers_, nitems_, nfactors_);
    return new_model;
}

void MFModel::sgd_update(double /*learning_rate*/,
        const ModelGradient* /*gradient*/) {
    throw std::runtime_error("Not implemented");
}

uint64_t MFModel::getSerializedSize() const {
    return size() * sizeof(FEATURE_TYPE);
}

void MFModel::loadSerialized(const void* data) {
  std::cout << "loadSerialized nusers: "
    << nusers_
    << " nitems_: " << nitems_
    << " nfactors_: " << nfactors_
    << std::endl;

    // Read number of samples, number of factors
    uint32_t* m = (uint32_t*)data;
    nusers_ = *m++;
    nitems_ = *m++;
    nfactors_ = *m++;

    // XXX FIX
    throw std::runtime_error("Not implemented");
}

FEATURE_TYPE MFModel::predict(uint32_t userId, uint32_t itemId) const {
  FEATURE_TYPE res = global_bias_ + user_bias_[userId] + item_bias_[itemId];
  
  for (uint32_t i = 0; i < nfactors_; ++i) {
    res += get_user_weights(userId, i) * get_item_weights(itemId, i);
#ifdef DEBUG
    if (std::isnan(res) || std::isinf(res)) {
      std::cout << "userId: " << userId << " itemId: " << itemId 
        << " get_user_weights(userId, i): " << get_user_weights(userId, i)
        << " get_item_weights(itemId, i): " << get_item_weights(itemId, i)
        << std::endl;
      throw std::runtime_error("nan error in predict");
    }
#endif
  }
  return res;
}

std::unique_ptr<ModelGradient> MFModel::minibatch_grad(
        const Matrix&,
        FEATURE_TYPE*,
        uint64_t,
        double) const {
  throw std::runtime_error("Not implemented");
}

FEATURE_TYPE& MFModel::get_user_weights(uint64_t userId, uint64_t factor) const {
  return *(user_weights_.get() + userId * nfactors_ + factor);
}

FEATURE_TYPE& MFModel::get_item_weights(uint64_t itemId, uint64_t factor) const {
  return *(item_weights_.get() + itemId * nfactors_ + factor);
}

void MFModel::sgd_update(
            double learning_rate,
            uint64_t base_user,
            const SparseDataset& dataset,
            double /*epsilon*/) {
  // iterate all pairs user rating
  for (uint64_t i = 0; i < dataset.data_.size(); ++i) {
    for (uint64_t j = 0; j < dataset.data_[i].size(); ++j) {
      uint64_t user = base_user + i;
      uint64_t itemId = dataset.data_[i][j].first;
      FEATURE_TYPE rating = dataset.data_[i][j].second;

      FEATURE_TYPE pred = predict(user, itemId);
      FEATURE_TYPE error = rating - pred;

      //std::cout 
      //  << "rating: " << rating
      //  << " prediction: " << pred
      //  << " error: " << error
      //  << std::endl;

      if (itemId >= nitems_ || user >= nusers_) {
        std::cout
          << "itemId: " << itemId
          << " nitems_: " << nitems_
          << " user: " << user
          << " nusers_: " << nusers_
          << std::endl;
        throw std::runtime_error("Wrong value here");
      }
      user_bias_[user] += learning_rate * (error - user_bias_reg_ * user_bias_[user]);
      item_bias_[itemId] += learning_rate * (error - item_bias_reg_ * item_bias_[itemId]);

#ifdef DEBUG
      if (std::isnan(user_bias_[user]) || std::isnan(item_bias_[itemId]) ||
          std::isinf(user_bias_[user]) || std::isinf(item_bias_[itemId]))
        throw std::runtime_error("nan in user_bias or item_bias");
#endif

      // update user latent factors
      for (uint64_t k = 0; k < nfactors_; ++k) {
        FEATURE_TYPE delta_user_w = 
          learning_rate * (error * get_item_weights(itemId, k) - user_fact_reg_ * get_user_weights(user, k));
        //std::cout << "delta_user_w: " << delta_user_w << std::endl;
        get_user_weights(user, k) += delta_user_w;
#ifdef DEBUG
        if (std::isnan(get_user_weights(user, k)) || std::isinf(get_user_weights(user, k))) {
          throw std::runtime_error("nan in user weight");
        }
#endif
      }

      // update item latent factors
      for (uint64_t k = 0; k < nfactors_; ++k) {
        FEATURE_TYPE delta_item_w =
          learning_rate * (error * get_user_weights(user, k) - item_fact_reg_ * get_item_weights(itemId, k));
        //std::cout << "delta_item_w: " << delta_item_w << std::endl;
        get_item_weights(itemId, k) += delta_item_w;
#ifdef DEBUG
        if (std::isnan(get_item_weights(itemId, k)) || std::isinf(get_item_weights(itemId, k))) {
          std::cout << "error: " << error << std::endl;
          std::cout << "user weight: " << get_user_weights(user, k) << std::endl;
          std::cout << "item weight: " << get_item_weights(itemId, k) << std::endl;
          std::cout << "learning_rate: " << learning_rate << std::endl;
          throw std::runtime_error("nan in item weight");
        }
      }
#endif
    }
  }
}

std::pair<double, double> MFModel::calc_loss(Dataset& /*dataset*/) const {
  throw std::runtime_error("Not implemented");
  return std::make_pair(0.0, 0.0);
}

std::pair<double, double> MFModel::calc_loss(SparseDataset& dataset) const {
  double error = 0;
  uint64_t count = 0;

  for (uint64_t userId = 0; userId < dataset.data_.size(); ++userId) {
    for (uint64_t j = 0; j < dataset.data_[userId].size(); ++j) {
      uint64_t movieId = dataset.data_[userId][j].first;
      FEATURE_TYPE rating = dataset.data_[userId][j].second;

      FEATURE_TYPE prediction = predict(userId, movieId);
      FEATURE_TYPE e = rating - prediction;

      //std::cout << "e: " << e << std::endl;

      error += pow(e, 2);
      if (std::isnan(e) || std::isnan(error)) {
        std::string error = std::string("nan in calc_loss rating: ") + std::to_string(rating) +
          " prediction: " + std::to_string(prediction);
        throw std::runtime_error(error);
      }
      count++;
    }
  }

  error = error / count;
  error = std::sqrt(error);
  return std::make_pair(error, 0);
}

uint64_t MFModel::getSerializedGradientSize() const {
    return size() * sizeof(FEATURE_TYPE);
}

std::unique_ptr<ModelGradient> MFModel::loadGradient(void* mem) const {
    auto grad = std::make_unique<MFGradient>(10, 10);
    grad->loadSerialized(mem);
    return grad;
}

double MFModel::checksum() const {
    return crc32(user_weights_.get(), nusers_ * nfactors_ * sizeof(FEATURE_TYPE));
}

void MFModel::print() const {
    std::cout << "MODEL user weights: ";
    for (uint64_t i = 0; i < nusers_ * nfactors_; ++i) {
      std::cout << " " << user_weights_.get()[i];
    }
    std::cout << std::endl;
}


