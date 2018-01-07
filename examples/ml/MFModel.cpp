#include <MFModel.h>
#include <Utils.h>
#include <MlUtils.h>
#include <Eigen/Dense>
#include <utils/Log.h>
#include <Checksum.h>
#include <algorithm>

// XXX matrix can be very sparse


// Support both sparse and non sparse representations

// Sparse:
// Number of users (32bits)
// Number of dimensions (32bits)
// Is_sparse (bool)
// Sample1: number of ratings (32bits) | rating movie1 (double) | rating movie 2 (double) | ...
// Sample 2: .......
// ......

// Non-sparse
// Number of users (32bits)
// Number of dimensions (32bits)
// Is_sparse(bool)
// Sample1: rating movie 10 | rating movie 15 | ......
// Sample2 : ...
// ....


MFModel::MFModel(uint64_t n, uint64_t d) {
  weights_ = new double[n * d];
  n_ = n;
  d_ = d;

  memset(weights_, 0, n_ * d_ * sizeof(double));
}

MFModel::MFModel(const void* w, bool is_sparse, uint64_t n, uint64_t d) {
  weights_ = new double[n * d];
  memset(weights_, 0, n * d * sizeof(double));

  void* data = reinterpret_cast<void*>(weights_);

  if (!is_sparse) {
    const double* input_data = reinterpret_cast<const double*>(w);
    for (uint64_t i = 0; i < n; ++i) {
      std::copy(input_data, input_data + d, data);
      input_data += d;
    }
  } else {
    for (uint64_t i = 0; i < n; ++i) {
      const uint32_t* numRatings = reinterpret_cast<const uint32_t*>(w);
      const uint32_t* movieId = (numRatings + 1);
      const double* rating = reinterpret_cast<const double*>(movieId + 1);
      // set user rating
      weights_[i * d + *movieId] = *rating;
      w = reinterpret_cast<const void*>(rating + 1);
    }
  }
}

MFModel::MFModel(const SparseDataset& dataset) {
  throw std::runtime_error("Not implemented");
}

uint64_t MFModel::size() const {
  return n_ * d_;
}

/** Format
  * Number of users (32bits)
  * Number of factors (32bits)
  * Weights in row order
  */
//std::unique_ptr<Model> MFModel::deserialize(void* data, uint64_t size) const {
// XXX fix this
int MFModel::deserialize(void* data, uint64_t size) const {
    uint32_t* data_p = (uint32_t*)data;
    uint32_t n = *data_p++;
    uint32_t d = *data_p++;
    bool* is_sparse = reinterpret_cast<bool*>(data_p);
      
    double* ratings_data = reinterpret_cast<double*>(is_sparse + 1);

    if (*is_sparse) {
      //return std::make_unique<MFModel>(
      //    reinterpret_cast<double*>(ratings_data), *is_sparse, n, d);
    } else {
      //return std::make_unique<MFModel>(
      //    reinterpret_cast<double*>(ratings_data), *is_sparse, n, d);
    }
    return 0;
}

std::pair<std::unique_ptr<char[]>, uint64_t>
MFModel::serialize() const {
    std::pair<std::unique_ptr<char[]>, uint64_t> res;
    uint64_t size = getSerializedSize();
    
    res.first.reset(new char[size]);
    res.second = size;

    serializeTo(res.first.get(), true); // use sparse
    return res;
}

void MFModel::serializeTo(void* mem, bool is_sparse) const {
    uint32_t* data_u = reinterpret_cast<uint32_t*>(mem);

    *data_u = n_;
    data_u++;
    *data_u = d_;
    data_u++;
    bool* data_bool = reinterpret_cast<bool*>(data_u);
    *data_bool = is_sparse;
    data_bool++;


    if (is_sparse) {
      uint32_t* num_ratings_in = reinterpret_cast<uint32_t*>(data_bool);
      for (uint32_t i = 0; i < n_; ++i) {

      }
    } else {
      std::copy(weights_, weights_ + n_ * d_, reinterpret_cast<double*>(data_bool));
    }
}

/**
  * We probably want to put 0 in the values we don't know
  */
void MFModel::randomize() {
  for (uint32_t i = 0; i < n_ * d_; ++i) {
    weights_[i] = 0.001 + get_rand_between_0_1();
  }
}

#if 0
std::unique_ptr<Model> MFModel::copy() const {
    std::unique_ptr<MFModel> new_model =
        std::make_unique<MFModel>(weights_, n_, d_);
    return new_model;
}
#endif

void MFModel::sgd_update(double learning_rate,
        const ModelGradient* gradient) {
    const MFGradient* grad = dynamic_cast<const MFGradient*>(gradient);

    if (grad == nullptr) {
        throw std::runtime_error("Error in dynamic cast");
    }

    throw std::runtime_error("Not implemented");
    //for (uint64_t i = 0; i < size(); ++i) {
    //   weights_[i] += learning_rate * grad->weights[i];
    //}
}

uint64_t MFModel::getSerializedSize() const {
    return size() * sizeof(double);
}

void MFModel::loadSerialized(const void* data) {
    cirrus::LOG<cirrus::INFO>("loadSerialized n: ", n_, " d: ", d_);

    uint32_t* m = (uint32_t*)data;
    n_ = *m++;
    d_ = *m++;
    bool *is_sparse = reinterpret_cast<bool*>(m);
    is_sparse++;

    //data = re

    weights_ = new double[n_ * d_];

    for (uint64_t i = 0; i < n_; ++i) {
        std::copy(w, w + d_, weights_[i].data());
        w += d_;
    }
}

std::unique_ptr<ModelGradient> MFModel::minibatch_grad(
        const SparseDataset& dataset,
        double epsilon) const {
    auto w = weights_;
#ifdef DEBUG
    dataset.check();
#endif

#if 0

    if (dataset.cols != size() || labels_size != dataset.rows) {
      throw std::runtime_error("Sizes don't match");
    }

    const double* dataset_data = dataset.data.get();
    // create Matrix for dataset
    Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic,
        Eigen::Dynamic, Eigen::RowMajor>>
          ds(const_cast<double*>(dataset_data), dataset.rows, dataset.cols);

    // create weight vector
    Eigen::Map<Eigen::VectorXd> tmp_weights(w.data(), size());

    // create vector with labels
    Eigen::Map<Eigen::VectorXd> lab(labels, labels_size);

    // apply logistic function to matrix multiplication
    // between dataset and weights
    auto part1_1 = (ds * tmp_weights);
    auto part1 = part1_1.unaryExpr(std::ptr_fun(mlutils::s_1));

    Eigen::Map<Eigen::VectorXd> lbs(labels, labels_size);

    // compute difference between labels and logistic probability
    auto part2 = lbs - part1;
    auto part3 = ds.transpose() * part2;
    auto part4 = tmp_weights * 2 * epsilon;
    auto res = part4 + part3;

    std::vector<double> vec_res;
    vec_res.resize(res.size());
    Eigen::VectorXd::Map(vec_res.data(), res.size()) = res;

    std::unique_ptr<LRGradient> ret = std::make_unique<LRGradient>(vec_res);

#ifdef DEBUG
    ret->check_values();
#endif
#endif

    return ret;
}

double MFModel::calc_loss(Dataset& dataset) const {
  double total_loss = 0;

  auto w = weights_;

#ifdef DEBUG
  dataset.check();
#endif

//  const double* ds_data =
//    reinterpret_cast<const double*>(dataset.samples_.data.get());
//
//  Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic,
//    Eigen::Dynamic, Eigen::RowMajor>>
//      ds(const_cast<double*>(ds_data),
//          dataset.samples_.rows, dataset.samples_.cols);
//
//  Eigen::Map<Eigen::VectorXd> weights_eig(w.data(), size());
//
//  // count how many samples are wrongly classified
//  uint64_t wrong_count = 0;
//  for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
//    // get labeled class for the ith sample
//    double class_i =
//      reinterpret_cast<const double*>(dataset.labels_.get())[i];
//
//    assert(is_integer(class_i));
//
//    int predicted_class = 0;
//
//    auto r1 = ds.row(i) *  weights_eig;
//    if (mlutils::s_1(r1) > 0.5) {
//      predicted_class = 1;
//    }
//    if (predicted_class != class_i) {
//      wrong_count++;
//    }
//
//    double v1 = mlutils::log_aux(1 - mlutils::s_1(ds.row(i) * weights_eig));
//    double v2 = mlutils::log_aux(mlutils::s_1(ds.row(i) *  weights_eig));
//
//    double value = class_i *
//      mlutils::log_aux(mlutils::s_1(ds.row(i) *  weights_eig)) +
//      (1 - class_i) * mlutils::log_aux(1 - mlutils::s_1(
//            ds.row(i) * weights_eig));
//
//    // XXX not sure this check is necessary
//    if (value > 0 && value < 1e-6)
//      value = 0;
//
//    if (value > 0) {
//      std::cout << "ds row: " << std::endl << ds.row(i) << std::endl;
//      std::cout << "weights: " << std::endl << weights_eig << std::endl;
//      std::cout << "Class: " << class_i << " " << v1 << " " << v2
//        << std::endl;
//      throw std::runtime_error("Error: logistic loss is > 0");
//    }
//
//    total_loss -= value;
//  }
//
//  if (total_loss < 0) {
//    throw std::runtime_error("total_loss < 0");
//  }
//
//  std::cout
//    << "Accuracy: " << (1.0 - (1.0 * wrong_count / dataset.num_samples()))
//    << std::endl;
//
//  if (std::isnan(total_loss) || std::isinf(total_loss))
//    throw std::runtime_error("calc_log_loss generated nan/inf");

  return total_loss;
}

uint64_t MFModel::getSerializedGradientSize() const {
    return size() * sizeof(double);
}

std::unique_ptr<ModelGradient> MFModel::loadGradient(void* mem) const {
    auto grad = std::make_unique<MFGradient>(size());

    for (uint64_t i = 0; i < size(); ++i) {
        grad->weights[i] = reinterpret_cast<double*>(mem)[i];
    }

    return grad;
}

double MFModel::checksum() const {
    return crc32(weights_, size() * sizeof(double));
}

void MFModel::print() const {
    std::cout << "MODEL: ";
    for (uint64_t i = 0; i < n_ * d_; ++i) {
      std::cout << " " << weights_[i];
    }
    std::cout << std::endl;
}


