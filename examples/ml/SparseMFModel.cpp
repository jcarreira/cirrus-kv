#include <SparseMFModel.h>
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

void SparseMFModel::initialize_weights(uint64_t users, uint64_t items, uint64_t nfactors) {
  item_fact_reg_ = 0.01;
  user_fact_reg_ = 0.01;

  user_bias_reg_ = 0.01;
  item_bias_reg_ = 0.01;

  nusers_ = users;
  nitems_ = items;
  nfactors_ = nfactors;

  randomize();
}

SparseMFModel::SparseMFModel(uint64_t users, uint64_t items, uint64_t nfactors) {
    initialize_weights(users , items, nfactors);
}

SparseMFModel::SparseMFModel(const void* data, uint64_t minibatch_size, uint64_t num_items) {
  loadSerialized(data, minibatch_size, num_items);
}

//uint64_t SparseMFModel::size() const {
//  throw std::runtime_error("Not implemented");
//  return 0;
//}

std::unique_ptr<Model> SparseMFModel::deserialize(void* data, uint64_t /*size*/) const {
  throw std::runtime_error("Not implemented");
    uint32_t* data_p = (uint32_t*)data;
    //uint32_t users = *data_p++;
    //uint32_t items = *data_p++;
    //uint32_t nfactors = *data_p++;

    //return std::make_unique<SparseMFModel>(
    //        reinterpret_cast<void*>(data_p), users, items, nfactors);
    return std::make_unique<SparseMFModel>(
            reinterpret_cast<void*>(data_p), 10, 10);
}

std::pair<std::unique_ptr<char[]>, uint64_t>
SparseMFModel::serialize() const {
  throw std::runtime_error("not implemented");
    std::pair<std::unique_ptr<char[]>, uint64_t> res;
    uint64_t size = getSerializedSize();
    
    res.first.reset(new char[size]);
    res.second = size;

    serializeTo(res.first.get());
    return res;
}

void SparseMFModel::serializeTo(void* /*mem*/) const {
  throw std::runtime_error("Not implemented");
#if 0
    uint32_t* data_u = reinterpret_cast<uint32_t*>(mem);

    *data_u = nusers_;
    data_u++;
    *data_u = nitems_;
    data_u++;
    *data_u = nfactors_;
    data_u++;

    FEATURE_TYPE* data_d = reinterpret_cast<FEATURE_TYPE*>(data_u);
    std::copy(user_weights_, user_weights_ + nusers_ * nfactors_, data_d);

    data_d += nusers_ * nfactors_;

    std::copy(item_weights_, item_weights_ + nitems_ * nfactors_, data_d);
#endif
}

/**
  * We probably want to put 0 in the values we don't know
  */
void SparseMFModel::randomize() {
  throw std::runtime_error("Not implemented");
#if 0
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
#endif
}

std::unique_ptr<Model> SparseMFModel::copy() const {
    std::unique_ptr<SparseMFModel> new_model =
        std::make_unique<SparseMFModel>(nusers_, nitems_, nfactors_);
    return new_model;
}

uint64_t SparseMFModel::getSerializedSize() const {
  throw std::runtime_error("Not implemented");
  return 0;
}

void SparseMFModel::loadSerialized(const void* data, uint64_t minibatch_size, uint64_t num_item_ids) {
  std::cout << "loadSerialized nusers: "
    << nusers_
    << " nitems_: " << nitems_
    << " nfactors_: " << nfactors_
    << std::endl;

  // data has minibatch_size vectors of size NUM_FACTORS (user weights)
  // followed by the same (item weights)
  for (uint64_t i = 0; i < minibatch_size; ++i) {
    std::tuple<int, FEATURE_TYPE,
      std::vector<FEATURE_TYPE>> user_model;
    uint32_t user_id = load_value<uint32_t>(data);
    FEATURE_TYPE user_bias = load_value<FEATURE_TYPE>(data);
    std::get<0>(user_model) = user_id;
    std::get<1>(user_model) = user_bias;
    for (uint64_t j = 0; j < NUM_FACTORS; ++j) {
      FEATURE_TYPE user_weight = load_value<FEATURE_TYPE>(data);
      std::get<2>(user_model).push_back(user_weight);
    }
    user_models.push_back(user_model);
  }
  
  // now we read the item vectors
  for (uint64_t i = 0; i < num_item_ids; ++i) {
    std::pair<FEATURE_TYPE,
      std::vector<FEATURE_TYPE>> item_model;
    uint32_t item_id = load_value<uint32_t>(data);
    FEATURE_TYPE item_bias = load_value<FEATURE_TYPE>(data);
    std::get<0>(item_model) = item_bias;
    for (uint64_t j = 0; j < NUM_FACTORS; ++j) {
      FEATURE_TYPE item_weight = load_value<FEATURE_TYPE>(data);
      std::get<1>(item_model).push_back(item_weight);
    }
    item_models[item_id] = item_model;
  }
}

/**
  * userId : 0 to minibatch_size
  */
FEATURE_TYPE SparseMFModel::predict(uint32_t userId, uint32_t itemId) {
  FEATURE_TYPE user_bias = std::get<1>(user_models[userId]);
  FEATURE_TYPE item_bias = item_models[itemId].first;

  FEATURE_TYPE res = global_bias_ + user_bias + item_bias;
  
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

std::unique_ptr<ModelGradient> SparseMFModel::minibatch_grad(
            const SparseDataset& dataset,
            const Configuration& config,
            uint64_t base_user) {

  FEATURE_TYPE learning_rate = config.get_learning_rate();
  auto gradient = std::make_unique<MFSparseGradient>();
  // iterate all pairs user rating
  for (uint64_t user_from_0 = 0; user_from_0 < dataset.data_.size(); ++user_from_0) {
    for (uint64_t j = 0; j < dataset.data_[user_from_0].size(); ++j) {
      // first user matches the model in user_models[0]
      uint64_t real_user_id = base_user + user_from_0;
      uint64_t itemId = dataset.data_[user_from_0][j].first;
      FEATURE_TYPE rating = dataset.data_[user_from_0][j].second;

      FEATURE_TYPE pred = predict(user_from_0, itemId);
      FEATURE_TYPE error = rating - pred;

      // compute gradient for user bias
      FEATURE_TYPE& user_bias = std::get<1>(user_models[user_from_0]);
      gradient->users_bias_grad[user_from_0] += learning_rate * (error - user_bias_reg_ * user_bias);

      // compute gradient for item bias
      FEATURE_TYPE& item_bias = item_models[itemId].first;
      gradient->items_bias_grad[itemId] += learning_rate * (error - item_bias_reg_ * item_bias);

#ifdef DEBUG
      if (std::isnan(user_bias) || std::isnan(item_bias) ||
          std::isinf(user_bias) || std::isinf(item_bias))
        throw std::runtime_error("nan in user_bias or item_bias");
#endif

      // update user latent factors
      std::vector<FEATURE_TYPE> user_weights_grad(NUM_FACTORS);
      for (uint64_t k = 0; k < nfactors_; ++k) {
        FEATURE_TYPE delta_user_w = 
          learning_rate * (error * get_item_weights(itemId, k) - user_fact_reg_ * get_user_weights(user_from_0, k));
        user_weights_grad[k] += delta_user_w;
#ifdef DEBUG
        if (std::isnan(get_user_weights(user_from_0, k)) || std::isinf(get_user_weights(user_from_0, k))) {
          throw std::runtime_error("nan in user weight");
        }
#endif
      }
      gradient->users_weights_grad.push_back(std::make_pair(real_user_id, std::move(user_weights_grad)));

      // update item latent factors
      std::vector<FEATURE_TYPE> item_weights_grad(NUM_FACTORS);
      for (uint64_t k = 0; k < nfactors_; ++k) {
        FEATURE_TYPE delta_item_w =
          learning_rate * (error * get_user_weights(user_from_0, k) - item_fact_reg_ * get_item_weights(itemId, k));
        item_weights_grad[k] += delta_item_w;
        //get_item_weights(itemId, k) += delta_item_w;
#ifdef DEBUG
        if (std::isnan(get_item_weights(itemId, k)) || std::isinf(get_item_weights(itemId, k))) {
          std::cout << "error: " << error << std::endl;
          std::cout << "user weight: " << get_user_weights(user_from_0, k) << std::endl;
          std::cout << "item weight: " << get_item_weights(itemId, k) << std::endl;
          std::cout << "learning_rate: " << learning_rate << std::endl;
          throw std::runtime_error("nan in item weight");
        }
      }
      gradient->items_weights_grad.push_back(std::make_pair(itemId, std::move(item_weights_grad)));
#endif
    }
  }
  return gradient;
}

FEATURE_TYPE& SparseMFModel::get_user_weights(uint64_t userId, uint64_t factor) {
  assert(userId < user_models.size());
  assert(factor < std::get<2>(user_models[userId]).size());
  return std::get<2>(user_models[userId])[factor];
}

FEATURE_TYPE& SparseMFModel::get_item_weights(uint64_t itemId, uint64_t factor) {
  assert(item_models.find(itemId) != item_models.end());
  assert(factor < item_models[itemId].second.size());
  return item_models[itemId].second[factor];
}

#if 0
void SparseMFModel::sgd_update(
            double learning_rate,
            uint64_t base_user,
            const SparseDataset& dataset) {
  // iterate all pairs user rating
  for (uint64_t user_from_0 = 0; user_from_0 < dataset.data_.size(); ++user_from_0) {
    for (uint64_t j = 0; j < dataset.data_[user_from_0].size(); ++j) {
      // first user matches the model in user_models[0]
      //uint64_t real_user_id = base_user + user_from_0;
      uint64_t itemId = dataset.data_[user_from_0][j].first;
      FEATURE_TYPE rating = dataset.data_[user_from_0][j].second;

      FEATURE_TYPE pred = predict(i, itemId);
      FEATURE_TYPE error = rating - pred;

      FEATURE_TYPE& user_bias = std::get<1>(user_models[i]);
      user_bias += learning_rate * (error - user_bias_reg_ * user_bias);
      FEATURE_TYPE& item_bias = item_models[itemId].first;
      item_bias += learning_rate * (error - item_bias_reg_ * item_bias);

#ifdef DEBUG
      if (std::isnan(user_bias) || std::isnan(item_bias) ||
          std::isinf(user_bias) || std::isinf(item_bias))
        throw std::runtime_error("nan in user_bias or item_bias");
#endif

      // update user latent factors
      for (uint64_t k = 0; k < nfactors_; ++k) {
        FEATURE_TYPE delta_user_w = 
          learning_rate * (error * get_item_weights(itemId, k) - user_fact_reg_ * get_user_weights(user_from_0, k));
        //std::cout << "delta_user_w: " << delta_user_w << std::endl;
        get_user_weights(user_from_0, k) += delta_user_w;
#ifdef DEBUG
        if (std::isnan(get_user_weights(user_from_0, k)) || std::isinf(get_user_weights(user_from_0, k))) {
          throw std::runtime_error("nan in user weight");
        }
#endif
      }

      // update item latent factors
      for (uint64_t k = 0; k < nfactors_; ++k) {
        FEATURE_TYPE delta_item_w =
          learning_rate * (error * get_user_weights(user_from_0, k) - item_fact_reg_ * get_item_weights(itemId, k));
        //std::cout << "delta_item_w: " << delta_item_w << std::endl;
        get_item_weights(itemId, k) += delta_item_w;
#ifdef DEBUG
        if (std::isnan(get_item_weights(itemId, k)) || std::isinf(get_item_weights(itemId, k))) {
          std::cout << "error: " << error << std::endl;
          std::cout << "user weight: " << get_user_weights(user_from_0, k) << std::endl;
          std::cout << "item weight: " << get_item_weights(itemId, k) << std::endl;
          std::cout << "learning_rate: " << learning_rate << std::endl;
          throw std::runtime_error("nan in item weight");
        }
      }
#endif
    }
  }
}
#endif

#if 0
std::pair<double, double> SparseMFModel::calc_loss(Dataset& dataset) const {
  throw std::runtime_error("Not implemented");
  return std::make_pair(0.0, 0.0);
}

std::pair<double, double> SparseMFModel::calc_loss(SparseDataset& dataset) const {
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
#endif

uint64_t SparseMFModel::getSerializedGradientSize() const {
  throw std::runtime_error("Not implemented");
  return 0;
  //  return size() * sizeof(FEATURE_TYPE);
}

std::unique_ptr<ModelGradient> SparseMFModel::loadGradient(void* mem) const {
    auto grad = std::make_unique<MFGradient>(10, 10);
    grad->loadSerialized(mem);
    return grad;
}

double SparseMFModel::checksum() const {
  return 0;
    //return crc32(user_weights_, nusers_ * nfactors_ * sizeof(FEATURE_TYPE));
}

void SparseMFModel::print() const {
    //std::cout << "MODEL user weights: ";
    //for (uint64_t i = 0; i < nusers_ * nfactors_; ++i) {
    //  std::cout << " " << user_weights_[i];
    //}
    //std::cout << std::endl;
}


