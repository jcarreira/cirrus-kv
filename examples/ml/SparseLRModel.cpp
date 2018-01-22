#include <SparseLRModel.h>
#include <Utils.h>
#include <MlUtils.h>
#include <Eigen/Dense>
#include <utils/Log.h>
#include <Checksum.h>
#include <algorithm>
#include <map>
#include <unordered_map>

#undef DEBUG

SparseLRModel::SparseLRModel(uint64_t d) {
    weights_.resize(d);
}

SparseLRModel::SparseLRModel(const FEATURE_TYPE* w, uint64_t d) {
    weights_.resize(d);
    std::copy(w, w + d, weights_.begin());
}

uint64_t SparseLRModel::size() const {
  return weights_.size();
}

/**
  * Serialization / deserialization routines
  */

/** FORMAT
  * weights
  */
std::unique_ptr<Model> SparseLRModel::deserialize(void* data, uint64_t size) const {
  throw std::runtime_error("not supported");
  uint64_t d = size / sizeof(FEATURE_TYPE);
  std::unique_ptr<SparseLRModel> model = std::make_unique<SparseLRModel>(
      reinterpret_cast<FEATURE_TYPE*>(data), d);
  return model;
}

std::pair<std::unique_ptr<char[]>, uint64_t>
SparseLRModel::serialize() const {
    throw std::runtime_error("Fix. Not implemented1");
    std::pair<std::unique_ptr<char[]>, uint64_t> res;
    uint64_t size = getSerializedSize();
    res.first.reset(new char[size]);

    res.second = size;
    std::memcpy(res.first.get(), weights_.data(), getSerializedSize());

    return res;
}

char* SparseLRModel::serializeTo2(uint64_t /*size*/) const {
  void* mem = new char[getSerializedSize()];
  void *mem_copy = mem;
  store_value<int>(mem, weights_.size());
  std::copy(weights_.data(), weights_.data() + weights_.size(),
      reinterpret_cast<FEATURE_TYPE*>(mem));
  return (char*)mem_copy;
}

void SparseLRModel::serializeTo(void* mem) const {
  std::cout << "weight size: " << weights_.size() << std::endl;
  store_value<int>(mem, weights_.size());
  for (const auto& v : weights_) {
    store_value<FEATURE_TYPE>(mem, v);
  }
}

uint64_t SparseLRModel::getSerializedSize() const {
  auto ret = size() * sizeof(FEATURE_TYPE) + sizeof(int);
  return ret;
}

/** FORMAT
  * number of weights (int)
  * list of weights: weight1 (FEATURE_TYPE) | weight2 (FEATURE_TYPE) | ..
  */
void SparseLRModel::loadSerialized(const void* data) {
  int num_weights = load_value<int>(data);
  //std::cout << "num_weights: " << num_weights << std::endl;
  assert(num_weights > 0 && num_weights < 10000000);

  //int size = num_weights * sizeof(FEATURE_TYPE) + sizeof(int);
  char* data_begin = (char*)data;

  weights_.resize(num_weights);
  std::copy(reinterpret_cast<FEATURE_TYPE*>(data_begin),
      (reinterpret_cast<FEATURE_TYPE*>(data_begin)) + num_weights,
      weights_.data());
}

/***
   *
   */

void SparseLRModel::randomize() {
    for (auto& w : weights_) {
        w = 0.001 + get_rand_between_0_1();
    }
}

std::unique_ptr<Model> SparseLRModel::copy() const {
    std::unique_ptr<SparseLRModel> new_model =
        std::make_unique<SparseLRModel>(weights_.data(), size());
    return new_model;
}

void SparseLRModel::sgd_update(double learning_rate,
        const ModelGradient* gradient) {
    const LRSparseGradient* grad = dynamic_cast<const LRSparseGradient*>(gradient);
    //const LRGradient* grad = dynamic_cast<const LRGradient*>(gradient);

    if (grad == nullptr) {
        throw std::runtime_error("Error in dynamic cast");
    }

    for (const auto& w : grad->weights) {
      int index = w.first;
      FEATURE_TYPE value = w.second;
      weights_[index] += learning_rate * value;
    }
}

std::unique_ptr<ModelGradient> SparseLRModel::minibatch_grad(
        const SparseDataset& dataset,
        double epsilon) const {
#ifdef DEBUG
    std::cout << "<Minibatch grad" << std::endl;
    dataset.check();
    //print();
#endif

#if 0
    const FEATURE_TYPE* dataset_data = dataset.data.get();
    // create Matrix for dataset
    Eigen::Map<Eigen::Matrix<FEATURE_TYPE, Eigen::Dynamic,
        Eigen::Dynamic, Eigen::RowMajor>>
          ds(const_cast<FEATURE_TYPE*>(dataset_data), dataset.rows, dataset.cols);
     create weight vector
    Eigen::Map<Eigen::Matrix<FEATURE_TYPE, -1, 1>> tmp_weights(w.data(), size());
     create vector with labels
    Eigen::Map<Eigen::Matrix<FEATURE_TYPE, -1, 1>> lab(labels, labels_size);
     apply logistic function to matrix multiplication
     between dataset and weights
    auto part1_1 = (ds * tmp_weights);
    auto part1 = part1_1.unaryExpr(std::ptr_fun(mlutils::s_1));
    auto before_1 = get_time_us();
#endif


    //std::vector<FEATURE_TYPE> part1(dataset.num_samples());
    std::vector<FEATURE_TYPE> part2(dataset.num_samples());
    for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
      double part1_i = 0;
      for (const auto& feat : dataset.get_row(i)) {
        int index = feat.first;
        FEATURE_TYPE value = feat.second;
        //std::cout <<"index: " << index << " wsize: " << weights_.size() << std::endl;
        if ((uint64_t)index >= weights_.size()) {
          throw std::runtime_error("Index too high");
        }
        assert(index >= 0 && (uint64_t)index < weights_.size());
        part1_i += value * weights_[index];
        //part1[i] += value * weights_[index];
#ifdef DEBUG
        if (std::isnan(part1_i) || std::isinf(part1_i)) {
        //if (std::isnan(part1[i]) || std::isinf(part1[i])) {
          std::cout << "part1_i: " << part1_i << std::endl;
          //std::cout << "part1[i]: " << part1[i] << std::endl;
          std::cout << "i: " << i << std::endl;
          std::cout << "index: " << index << " value: " << value << std::endl;
          std::cout << "weights_[index]: " << weights_[index] << std::endl;
          exit(-1);
        }
#endif
      }

      part2[i] = dataset.labels_[i] - mlutils::s_1(part1_i);
      //part1[i] = mlutils::s_1(part1[i]);
//#ifdef DEBUG
//        if (std::isnan(part1[i]) || std::isinf(part1[i])) {
//          std::cout << "isnan" << std::endl;
//          exit(-1);
//        }
//#endif
    }
#ifdef DEBUG
    auto after_1 = get_time_us();
#endif


    //Eigen::Map<Eigen::Matrix<FEATURE_TYPE, -1, 1>> lbs(labels, labels_size);

    // compute difference between labels and logistic probability
    //auto part2 = lbs - part1;
//    std::vector<FEATURE_TYPE> part2(dataset.num_samples());
//    for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
//      part2[i] = dataset.labels_[i] - part1[i];
//#ifdef DEBUG
//        if (std::isnan(part2[i]) || std::isinf(part2[i])) {
//          std::cout << "part2 isnan" << std::endl;
//          exit(-1);
//        }
//#endif
//    }

    //auto part3 = ds.transpose() * part2;
    //std::vector<FEATURE_TYPE> part3(weights_.size());
    std::unordered_map<uint64_t, FEATURE_TYPE> part3;
    for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
      for (const auto& feat : dataset.get_row(i)) {
        int index = feat.first;
        FEATURE_TYPE value = feat.second;
        part3[index] += value * part2[i];
#ifdef DEBUG
        if (std::isnan(part3[index]) || std::isinf(part3[index])) {
          std::cout << "part3 isnan" << std::endl;
          std::cout << "part2[i]: " << part2[i] << std::endl;
          std::cout << "i: " << i << std::endl;
          std::cout << "value: " << value << std::endl;
          std::cout << "index: " << index << " value: " << value << std::endl;
          exit(-1);
        }
#endif
      }
    }
#ifdef DEBUG
    auto after_2 = get_time_us();
#endif

    std::vector<std::pair<int, FEATURE_TYPE>> res;
    res.reserve(part3.size());
    //std::vector<FEATURE_TYPE> res(weights_);
    for (const auto& v : part3) {
      uint64_t index = v.first;
      FEATURE_TYPE value = v.second;
      res.push_back(std::make_pair(index, value + weights_[index] * 2 * epsilon));
      //res[index] = res[index] * 2 * epsilon + value;
    }
#ifdef DEBUG
    auto after_3 = get_time_us();
#endif

    //for (uint64_t i = 0; i < weights_.size(); ++i) {
    //  //res[i] = part3[i] + weights_[i] * 2 * epsilon;
    //}
#if 0
    std::vector<FEATURE_TYPE> res(weights_.size());
    for (uint64_t i = 0; i < weights_.size(); ++i) {
      //part4[i] = weights_[i] * 2 * epsilon;
      res[i] = part3[i] + weights_[i] * 2 * epsilon;
#ifdef DEBUG
        if (std::isnan(part3[i]) || std::isinf(part3[i])) {
          std::cout << "part3 isnan" << std::endl;
          std::cout << "weights_[i]: " << weights_[i] << std::endl;
          std::cout << "i: " << i << std::endl;
          std::cout << "epsilon: " << epsilon << std::endl;
          exit(-1);
        }
#endif
    }
#endif
    
//    std::vector<FEATURE_TYPE> res(weights_.size());
//    for (uint64_t i = 0; i < weights_.size(); ++i) {
//      res[i] = part4[i] + part3[i];
//#ifdef DEBUG
//        if (std::isnan(res[i]) || std::isinf(res[i])) {
//          std::cout << "res isnan" << std::endl;
//          exit(-1);
//        }
//#endif
//    }

    //std::vector<FEATURE_TYPE> vec_res;
    //vec_res.resize(res.size());
    //Eigen::Matrix<FEATURE_TYPE, -1, 1>::Map(vec_res.data(), res.size()) = res;

    //std::unique_ptr<LRGradient> ret = std::make_unique<LRGradient>(vec_res);
    std::unique_ptr<LRSparseGradient> ret = std::make_unique<LRSparseGradient>(std::move(res));
#ifdef DEBUG
    auto after_4 = get_time_us();
#endif
    //std::unique_ptr<LRGradient> ret = std::make_unique<LRGradient>(res);

#ifdef DEBUG
    ret->check_values();
    std::cout
      << " Elapsed1: " << (after_1 - before_1)
      << " Elapsed2: " << (after_2 - after_1)
      << " Elapsed3: " << (after_3 - after_2)
      << " Elapsed4: " << (after_4 - after_3)
      << std::endl;
#endif
    return ret;
}

std::pair<double, double> SparseLRModel::calc_loss(SparseDataset& dataset) const {
  double total_loss = 0;
  auto w = weights_;

#ifdef DEBUG
  dataset.check();
#endif

  //const FEATURE_TYPE* ds_data =
  //  reinterpret_cast<const FEATURE_TYPE*>(dataset.samples_.data.get());

  //Eigen::Map<Eigen::Matrix<FEATURE_TYPE, Eigen::Dynamic,
  //  Eigen::Dynamic, Eigen::RowMajor>>
  //    ds(const_cast<FEATURE_TYPE*>(ds_data),
  //        dataset.samples_.rows, dataset.samples_.cols);

  //Eigen::Map<Eigen::Matrix<FEATURE_TYPE, -1, 1>> weights_eig(w.data(), size());

  // count how many samples are wrongly classified
  uint64_t wrong_count = 0;
  for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
    // get labeled class for the ith sample
    FEATURE_TYPE class_i = dataset.labels_[i];

    //auto r1 = ds.row(i) *  weights_eig;

    const auto& sample = dataset.get_row(i);
    FEATURE_TYPE r1 = 0;
    for (const auto& feat : sample) {
      int index = feat.first;
      FEATURE_TYPE value = feat.second;
      r1 += weights_[index] * value;
    }

    FEATURE_TYPE predicted_class = 0;
    if (mlutils::s_1(r1) > 0.5) {
      predicted_class = 1.0;
    }
    if (predicted_class != class_i) {
      wrong_count++;
    }

    //FEATURE_TYPE v1 = mlutils::log_aux(1 - mlutils::s_1(r1));
    //FEATURE_TYPE v2 = mlutils::log_aux(mlutils::s_1(r1));

    FEATURE_TYPE value = class_i *
      mlutils::log_aux(mlutils::s_1(r1)) +
      (1 - class_i) * mlutils::log_aux(1 - mlutils::s_1(r1));

    if (value > 0 && value < 1e-6) {
      // XXX this should be investigated
      //throw std::runtime_error("This code is actually used huh");
      value = 0;
    }

    if (value > 0) {
      //std::cout << "ds row: " << std::endl << ds.row(i) << std::endl;
      //std::cout << "weights: " << std::endl << weights_eig << std::endl;
      //std::cout << "Class: " << class_i << " " << v1 << " " << v2
      //  << std::endl;
      throw std::runtime_error("Error: logistic loss is > 0");
    }

  //std::cout << "value: " << value << std::endl;
    total_loss -= value;
  }

  //std::cout << "wrong_count: " << wrong_count << std::endl;

  if (total_loss < 0) {
    throw std::runtime_error("total_loss < 0");
  }

  FEATURE_TYPE accuracy = (1.0 - (1.0 * wrong_count / dataset.num_samples()));
  if (std::isnan(total_loss) || std::isinf(total_loss))
    throw std::runtime_error("calc_log_loss generated nan/inf");

  return std::make_pair(total_loss, accuracy);
}

uint64_t SparseLRModel::getSerializedGradientSize() const {
    return size() * sizeof(FEATURE_TYPE);
}

std::unique_ptr<ModelGradient> SparseLRModel::loadGradient(void* mem) const {
  throw std::runtime_error("Not supported");
    auto grad = std::make_unique<LRGradient>(size());

    for (uint64_t i = 0; i < size(); ++i) {
        grad->weights[i] = reinterpret_cast<FEATURE_TYPE*>(mem)[i];
    }

    return grad;
}

bool SparseLRModel::is_integer(FEATURE_TYPE n) const {
    return floor(n) == n;
}

double SparseLRModel::checksum() const {
    return crc32(weights_.data(), weights_.size() * sizeof(FEATURE_TYPE));
}

void SparseLRModel::print() const {
    std::cout << "MODEL: ";
    for (const auto& w : weights_) {
        std::cout << " " << w;
    }
    std::cout << std::endl;
}

void SparseLRModel::check() const {
  for (const auto& w : weights_) {
    if (std::isnan(w) || std::isinf(w)) {
      std::cout << "Wrong model weight" << std::endl;
      exit(-1);
    }
  }
}
