#include <examples/ml/LRModel.h>
#include <Utils.h>
#include <MlUtils.h>
#include <Eigen/Dense>
#include <utils/Log.h>
#include <Checksum.h>
#include <algorithm>

LRModel::LRModel(uint64_t d) :
    d(d) {
    weights.resize(d);
}

LRModel::LRModel(const double* w, uint64_t d) :
    d(d) {
    weights.resize(d);
    std::copy(w, w + d, weights.begin());
}

std::unique_ptr<Model> LRModel::deserialize(void* data, uint64_t size) const {
    uint64_t d = size / sizeof(double);
    std::unique_ptr<LRModel> model = std::make_unique<LRModel>(
            reinterpret_cast<double*>(data), d);
    return model;
}

void check_dataset(Dataset& dataset) {
    for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
        for (uint64_t j = 0; j < dataset.num_features(); ++j) {
            const double* s = dataset.sample(i);
            if (std::isnan(s[j]) || std::isinf(s[j])) {
                throw std::runtime_error("Invalid dataset");
            }
        }
    }
}

std::pair<std::unique_ptr<char[]>, uint64_t>
LRModel::serialize() const {
    std::pair<std::unique_ptr<char[]>, uint64_t> res;
    uint64_t size = getSerializedSize();
    res.first.reset(new char[size]);

    res.second = size;
    std::memcpy(res.first.get(), weights.data(), getSerializedSize());

    return res;
}

void LRModel::serializeTo(void* mem) const {
    std::memcpy(mem, weights.data(), getSerializedSize());
}

void LRModel::randomize() {
    for (uint64_t i = 0; i < d; ++i) {
        weights[i] = get_rand_between_0_1();
    }
}

std::unique_ptr<Model> LRModel::copy() const {
    std::unique_ptr<LRModel> new_model =
        std::make_unique<LRModel>(weights.data(), d);
    return new_model;
}

void LRModel::sgd_update(double learning_rate,
        const ModelGradient* gradient) {
    const LRGradient* grad = dynamic_cast<const LRGradient*>(gradient);

    if (grad == nullptr) {
        throw std::runtime_error("Error in dynamic cast");
    }

    for (uint64_t i = 0; i < d; ++i) {
       weights[i] += learning_rate * grad->weights[i];
    }
}

uint64_t LRModel::getSerializedSize() const {
    return d * sizeof(double);
}

void LRModel::loadSerialized(const void* data) {
    cirrus::LOG<cirrus::INFO>("loadSerialized d: ", d);
    const double* v = reinterpret_cast<const double*>(data);
    std::copy(v, v + d, weights.begin());
}

std::unique_ptr<ModelGradient> LRModel::minibatch_grad(
        const Matrix& dataset,
        double* labels,
        uint64_t labels_size,
        double epsilon) const {
    auto w = weights;
#ifdef DEBUG
    dataset.check_values();
#endif

    const double* dataset_data = dataset.data.get();
    // create Matrix for dataset
    Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic,
        Eigen::Dynamic, Eigen::RowMajor>>
          ds(const_cast<double*>(dataset_data), dataset.rows, dataset.cols);

    // create weight vector
    Eigen::Map<Eigen::VectorXd> weights(w.data(), d);

    // create vector with labels
    Eigen::Map<Eigen::VectorXd> lab(labels, labels_size);

    // apply logistic function to matrix multiplication
    // between dataset and weights
    auto part1_1 = (ds * weights);
    auto part1 = part1_1.unaryExpr(std::ptr_fun(mlutils::s_1));

    Eigen::Map<Eigen::VectorXd> lbs(labels, labels_size);

    // compute difference between labels and logistic probability
    auto part2 = lbs - part1;
    auto part3 = ds.transpose() * part2;
    auto part4 = weights * 2 * epsilon;
    auto res = part4 + part3;

    std::vector<double> vec_res;
    vec_res.resize(res.size());
    Eigen::VectorXd::Map(vec_res.data(), res.size()) = res;

    std::unique_ptr<LRGradient> ret = std::make_unique<LRGradient>(vec_res);

#ifdef DEBUG
    ret->check_values();
#endif

    return ret;
}

double LRModel::calc_loss(Dataset& dataset) const {
    double total_loss = 0;

    auto w = weights;

#ifdef DEBUG
    dataset.check_values();
#endif
#ifdef DEBUG
    check_dataset(dataset);  // make sure dataset is valid
#endif

    const double* ds_data =
        reinterpret_cast<const double*>(dataset.samples_.data.get());

    Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic,
        Eigen::Dynamic, Eigen::RowMajor>>
            ds(const_cast<double*>(ds_data),
                    dataset.samples_.rows, dataset.samples_.cols);

    Eigen::Map<Eigen::VectorXd> weights_eig(w.data(), d);

    // count how many samples are wrongly classified
    uint64_t wrong_count = 0;
    for (uint64_t i = 0; i < dataset.num_samples(); ++i) {
        // get labeled class for the ith sample
        double class_i =
            reinterpret_cast<const double*>(dataset.labels_.get())[i];

        assert(is_integer(class_i));

        int predicted_class = 0;

        auto r1 = ds.row(i) *  weights_eig;
        if (mlutils::s_1(r1) > 0.5) {
            predicted_class = 1;
        }
        if (predicted_class != class_i) {
            wrong_count++;
        }

        double v1 = mlutils::log_aux(1 - mlutils::s_1(ds.row(i) * weights_eig));
        double v2 = mlutils::log_aux(mlutils::s_1(ds.row(i) *  weights_eig));

        double value = class_i *
            mlutils::log_aux(mlutils::s_1(ds.row(i) *  weights_eig)) +
            (1 - class_i) * mlutils::log_aux(1 - mlutils::s_1(
                        ds.row(i) * weights_eig));

        // XXX not sure this check is necessary
        if (value > 0 && value < 1e-6)
            value = 0;

        if (value > 0) {
            std::cout << "ds row: " << std::endl << ds.row(i) << std::endl;
            std::cout << "weights: " << std::endl << weights_eig << std::endl;
            std::cout << "Class: " << class_i << " " << v1 << " " << v2
                << std::endl;
            throw std::runtime_error("Error: logistic loss is > 0");
        }

        total_loss -= value;
    }

    if (total_loss < 0) {
        throw std::runtime_error("total_loss < 0");
    }

    std::cout
        << "Accuracy: " << (1.0 - (1.0 * wrong_count / dataset.num_samples()))
        << std::endl;

    if (std::isnan(total_loss) || std::isinf(total_loss))
        throw std::runtime_error("calc_log_loss generated nan/inf");

    return total_loss;
}

uint64_t LRModel::getSerializedGradientSize() const {
    return d * sizeof(double);
}

std::unique_ptr<ModelGradient> LRModel::loadGradient(void* mem) const {
    auto grad = std::make_unique<LRGradient>(d);

    for (uint64_t i = 0; i < d; ++i) {
        grad->weights[i] = reinterpret_cast<double*>(mem)[i];
    }

    return grad;
}

bool LRModel::is_integer(double n) const {
    return floor(n) == n;
}

double LRModel::checksum() const {
    return crc32(weights.data(), weights.size() * sizeof(double));
}

void LRModel::print() const {
    std::cout << "MODEL: ";
    for (const auto& w : weights) {
        std::cout << " " << w;
    }
    std::cout << std::endl;
}


