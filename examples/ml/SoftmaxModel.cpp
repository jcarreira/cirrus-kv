#include <examples/ml/SoftmaxModel.h>
#include <Utils.h>
#include <Eigen/Dense>
#include <cmath>
#include <algorithm>
#include <Checksum.h>

SoftmaxModel::SoftmaxModel(uint64_t classes, uint64_t d) :
    nclasses(classes), d(d) {
    weights.resize(d);
    for (auto& v : weights)
        v.resize(nclasses);
}

// Row major order
SoftmaxModel::SoftmaxModel(const double* data, uint64_t nclasses, uint64_t d) :
    nclasses(nclasses), d(d) {
    weights.resize(d);

    for (auto& v : weights) {
        v.resize(nclasses);
    }

    loadSerialized(data);
}

SoftmaxModel::SoftmaxModel(
        std::vector<std::vector<double>> data, uint64_t nclasses, uint64_t d) :
    nclasses(nclasses), d(d) {
    weights.resize(d);

    int i = 0;
    for (auto& v : weights) {
        v.resize(nclasses);
        std::copy(data[i].begin(), data[i].end(), v.begin());
    }
}

void SoftmaxModel::randomize() {
    for (uint64_t i = 0; i < d; ++i) {
        for (uint64_t j = 0; j < nclasses; ++j) {
            weights[i][j] = 0.01 * (get_rand_between_0_1() - 0.5);
        }
    }
}

/** Serialization Format:
  * d (uint64_t)
  * n_classes (uint64_t)
  * d * nclasses weights (double)
  */
std::pair<std::unique_ptr<char[]>, uint64_t>
SoftmaxModel::serialize() const {
    std::pair<std::unique_ptr<char[]>, uint64_t> res;

    res.second = getSerializedSize();
    res.first.reset(new char[res.second]);

    uint64_t* uint_ptr = reinterpret_cast<uint64_t*>(res.first.get());
    *uint_ptr++ = d;
    *uint_ptr++ = nclasses;

    // copy contents from weights vector to the serialized buffer
    double* w = reinterpret_cast<double*>(uint_ptr);
    for (uint64_t i = 0; i < d; ++i) {
        for (uint64_t j = 0; j < nclasses; ++j) {
            w[i * nclasses + j] = weights[i][j];
        }
    }
    return res;
}

void SoftmaxModel::serializeTo(void* mem) const {
    uint64_t* uint_ptr = reinterpret_cast<uint64_t*>(mem);
    *uint_ptr++ = d;
    *uint_ptr++ = nclasses;

    double* w = reinterpret_cast<double*>(uint_ptr);
    for (uint64_t i = 0; i < d; ++i) {
        for (uint64_t j = 0; j < nclasses; ++j) {
            w[i * nclasses + j] = weights[i][j];
        }
    }
}

void SoftmaxModel::loadSerialized(const void* data) {
    const uint64_t* uint_ptr = reinterpret_cast<const uint64_t*>(data);
    uint64_t dim = *uint_ptr++;
    uint64_t classes = *uint_ptr++;

    weights.resize(dim);
    for (auto& v : weights) {
        v.resize(classes);
    }

    const double* w = reinterpret_cast<const double*>(uint_ptr);

    for (uint64_t i = 0; i < d; ++i) {
        std::copy(w + i * nclasses, w + (i + 1) * nclasses, weights[i].begin());
    }
}

std::unique_ptr<Model> SoftmaxModel::deserialize(void* data,
            uint64_t /* size */) const {
    uint64_t* uint_ptr = reinterpret_cast<uint64_t*>(data);
    uint64_t dim = *uint_ptr++;
    uint64_t classes = *uint_ptr++;

    std::unique_ptr<SoftmaxModel> model =
        std::make_unique<SoftmaxModel>(
                reinterpret_cast<double*>(uint_ptr), classes, dim);
    return model;
}

std::unique_ptr<Model> SoftmaxModel::copy() const {
    std::unique_ptr<SoftmaxModel> new_model =
        std::make_unique<SoftmaxModel>(weights, nclasses, d);
    return new_model;
}

void SoftmaxModel::sgd_update(
        double learning_rate, const ModelGradient* gradient) {
    const SoftmaxGradient* grad =
        dynamic_cast<const SoftmaxGradient*>(gradient);

    if (grad == nullptr) {
        throw std::runtime_error("Error in dynamic cast");
    }

    for (uint64_t i = 0; i < d; ++i) {
        for (uint64_t j = 0; j < nclasses; ++j) {
            weights[i][j] -= learning_rate * grad->weights[i][j];
        }
    }
}

uint64_t SoftmaxModel::getSerializedSize() const {
    return sizeof(double) * d * nclasses + sizeof(uint64_t) * 2;
}

std::unique_ptr<ModelGradient> SoftmaxModel::minibatch_grad(
            int /* rank */, const Matrix& m,
            double* labels,
            uint64_t labels_size,
            double epsilon) const {
    assert(labels_size == m.rows);

    const double* m_data = reinterpret_cast<const double*>(m.data.get());
    Eigen::MatrixXd dataset(m.rows, m.cols);
    for (unsigned int row = 0; row < m.rows; ++row) {
        for (unsigned int col = 0; col < m.cols; ++col) {
            dataset(row, col) = m_data[row * m.cols + col];
        }
    }

    Eigen::MatrixXd W(dataset.cols(), nclasses);
    for (unsigned int d = 0; d < dataset.cols(); ++d) {
        for (unsigned int k = 0; k < nclasses; ++k) {
            W(d, k) = weights[d][k];
        }
    }

    auto scores = dataset * W;

    // we exponentiate those scores
    // [N * K]
    auto exp_scores = scores.unaryExpr([](double v) {
        double new_v = std::exp(v);
        if (std::isnan(new_v) || std::isinf(new_v)) {
            throw std::runtime_error("Invalid value after exp");
        }

        return new_v;
    });

    // [N * K]
    auto exp_scores_sum = exp_scores.sum();

    // [N * K]
    auto probs = exp_scores.unaryExpr([exp_scores_sum](double v) {
            return v / exp_scores_sum;
            });

    std::vector<double> logprobs(dataset.rows());
    double sum = 0;

    for (unsigned int i = 0; i < dataset.rows(); ++i) {
        if (probs(i, labels[i]) < 1e-10) {
            logprobs[i] -= 1e-10;
        } else {
            logprobs[i] -= std::log(probs(i, labels[i]));
        }
        if (std::isnan(logprobs[i]) || std::isinf(logprobs[i])) {
            std::cerr << probs(i, labels[i]) << " " << labels[i] << std::endl;
            throw std::runtime_error("Invalid logprob");
        }
        sum += logprobs[i];
    }

    // [N * K]
    Eigen::MatrixXd dscores;
    dscores.noalias() = probs;
    for (unsigned int i = 0; i < dataset.rows(); ++i) {
        dscores(i, labels[i]) -= 1;
        dscores(i, labels[i]) /= dataset.rows();
    }

    Eigen::MatrixXd dW;
    // [D * N] * [N * K] = [D * K]
    dW.noalias() = dataset.transpose() * dscores;
    dW += epsilon * W;

    assert(static_cast<uint64_t>(dW.rows()) == d &&
           static_cast<uint64_t>(dW.cols()) == nclasses);

    // transform dW eigen matrix into std::vector
    std::vector<std::vector<double>> ret_gradient;
    ret_gradient.resize(d);
    for (uint64_t i = 0; i < d; ++i) {
        ret_gradient[i].resize(nclasses);
        for (uint64_t j = 0; j < nclasses; ++j) {
            ret_gradient[i][j] = dW(i, j);
        }
    }

    return std::make_unique<SoftmaxGradient>(ret_gradient);
}

double SoftmaxModel::calc_loss(Dataset& data) const {
    const Matrix& m = data.samples_;
    // XXX Fix, there is some code repetition here
    const double* m_data = reinterpret_cast<const double*>(m.data.get());
    Eigen::MatrixXd dataset(m.rows, m.cols);
    for (unsigned int row = 0; row < m.rows; ++row) {
        for (unsigned int col = 0; col < m.cols; ++col) {
            dataset(row, col) = m_data[row * m.cols + col];
        }
    }

    Eigen::MatrixXd W(dataset.cols(), nclasses);
    for (unsigned int d = 0; d < dataset.cols(); ++d) {
        for (unsigned int k = 0; k < nclasses; ++k) {
            W(d, k) = weights[d][k];
        }
    }

    auto scores = dataset * W;

    // we exponentiate those scores
    // [N * K]
    auto exp_scores = scores.unaryExpr([](double v) {
        double new_v = std::exp(v);
        if (std::isnan(new_v) || std::isinf(new_v)) {
            throw std::runtime_error("Invalid value after exp");
        }

        return new_v;
    });

    // [N * K]
    auto exp_scores_sum = exp_scores.sum();

    // [N * K]
    auto probs = (exp_scores.unaryExpr([exp_scores_sum](double v) {
        return v / exp_scores_sum;
    })).eval();

    std::vector<double> logprobs(dataset.rows());
    double sum = 0;
    uint64_t count_wrong = 0;  // how many samples are wrongly classified

    for (unsigned int i = 0; i < dataset.rows(); ++i) {
        double class_i = reinterpret_cast<const double*>(data.labels_.get())[i];
        for (int c = 0; c < probs.cols(); ++c) {
            // if there is a different class with higher probability
            // we count it as wrong
            if (probs(i, c) > probs(i, class_i)) {
                count_wrong++;
                break;
            }
        }

        if (probs(i, class_i) < 1e-10) {
            logprobs[i] = 1e-10;
        } else {
            logprobs[i] = -std::log(probs(i, class_i));
        }
        if (std::isnan(logprobs[i]) || std::isinf(logprobs[i])) {
            std::cerr
                << probs(i, class_i) << " "
                << class_i << std::endl;
            throw std::runtime_error("Invalid logprob");
        }
        sum += logprobs[i];
    }

    std::cout
        << "Accuracy: " << (1.0 - (1.0 * count_wrong / dataset.rows()))
        << " wrong: " << count_wrong << " samples: " << dataset.rows()
        << std::endl;

    // constant
    double data_loss = sum / dataset.rows();

    return data_loss;
}

/**
 * Return the size of the gradient when serialized
 */
uint64_t SoftmaxModel::getSerializedGradientSize() const {
    return nclasses * d * sizeof(double);
}

std::unique_ptr<ModelGradient> SoftmaxModel::loadGradient(void* mem) const {
    auto grad = std::make_unique<SoftmaxGradient>(nclasses, d);
    grad->loadSerialized(mem);
    return grad;
}

double SoftmaxModel::checksum() const {
    double sum = 0;
    for (uint64_t i = 0; i < weights.size(); ++i) {
        sum += crc32(weights[i].data(), weights[i].size() * sizeof(double));
    }
    return sum;
}

