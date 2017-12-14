#include <examples/ml/Configuration.h>

#include <utils/Log.h>
#include <fstream>
#include <iostream>
#include <sstream>

Configuration::Configuration() :
        learning_rate(-1),
        epsilon(-1)
{}

/**
  * Read configuration from a specific config file
  * @param path Path to the configuration file
  */
void Configuration::read(const std::string& path) {
    std::ifstream fin(path.c_str(), std::ifstream::in);

    if (!fin) {
        std::cout << "Error opening config file: " << std::endl;
        throw std::runtime_error("Error opening config file: " + path);
    }

    std::string line;
    while (getline(fin, line)) {
        parse_line(line);
    }

    print();
}

void Configuration::print() const {
    std::cout << "Printing configuration: " << std::endl;
    std::cout << "input_path: " << get_input_path() << std::endl;
    std::cout << "Minibatch size: " << get_minibatch_size() << std::endl;
    std::cout << "S3 size: " << get_s3_size() << std::endl;
    std::cout << "learning rate: " << get_learning_rate() << std::endl;
    std::cout << "limit_samples: " << get_limit_samples() << std::endl;
}

/**
  * Parse a specific line in the config file
  * @param line A line from the input file
  */
void Configuration::parse_line(const std::string& line) {
    std::istringstream iss(line);

    std::string s;
    iss >> s;

    std::cout << "Parsing line: " << line << std::endl;

    if (s == "minibatch_size:") {
        iss >> minibatch_size;
    } else if (s == "s3_size:") {
        iss >> s3_size;
    } else if (s == "input_path:") {
        iss >> input_path;
    } else if (s == "samples_path:") {
        iss >> samples_path;
    } else if (s == "labels_path:") {
        iss >> labels_path;
    } else if (s == "n_workers:") {
        iss >> n_workers;
    } else if (s == "prefetching:") {
        iss >> prefetching;
    } else if (s == "epsilon:") {
        iss >> epsilon;
    } else if (s == "input_type:") {
        iss >> input_type;
    } else if (s == "learning_rate:") {
        iss >> learning_rate;
    } else if (s == "num_classes:") {
        iss >> num_classes;
    } else if (s == "limit_cols:") {
        iss >> limit_cols;
    } else if (s == "limit_samples:") {
        iss >> limit_samples;
    } else if (s == "normalize:") {
        int n;
        iss >> n;
        normalize = (n == 1);
    } else if (s == "model_type:") {
        std::string model;
        iss >> model;
        if (model == "LogisticRegression") {
            model_type = LOGISTICREGRESSION;
        } else if (model == "Softmax") {
            model_type = SOFTMAX;
        } else {
            throw std::runtime_error(std::string("Unknown model : ") + model);
        }
    } else {
        throw std::runtime_error("Unrecognized option: " + line);
    }

    if (iss.fail()) {
        throw std::runtime_error("Error parsing configuration file");
    }
}

std::string Configuration::get_input_path() const {
    if (input_path == "")
        throw std::runtime_error("input path not loaded");
    return input_path;
}

std::string Configuration::get_samples_path() const {
    if (samples_path == "")
        throw std::runtime_error("samples path not loaded");
    if (input_type != "double_binary")
        throw std::runtime_error("mismatch between paths and input type");
    return samples_path;
}

std::string Configuration::get_labels_path() const {
    if (labels_path == "")
        throw std::runtime_error("labels path not loaded");
    if (input_type != "double_binary")
        throw std::runtime_error("mismatch between paths and input type");
    return labels_path;
}

double Configuration::get_learning_rate() const {
    if (learning_rate == -1)
        throw std::runtime_error("learning rate not loaded");
    return learning_rate;
}

double Configuration::get_epsilon() const {
    if (epsilon == -1)
        throw std::runtime_error("epsilon not loaded");
    return epsilon;
}

uint64_t Configuration::get_minibatch_size() const {
    if (minibatch_size == 0)
        throw std::runtime_error("Minibatch size not loaded");
    return minibatch_size;
}

uint64_t Configuration::get_s3_size() const {
    if (s3_size == 0)
        throw std::runtime_error("Minibatch size not loaded");
    return s3_size;
}

int Configuration::get_prefetching() const {
    if (prefetching == -1)
        throw std::runtime_error("prefetching not loaded");
    return prefetching;
}

std::string Configuration::get_input_type() const {
    if (input_type == "")
        throw std::runtime_error("input_type not loaded");
    return input_type;
}

/**
  * Get the format of the input dataset from the config file
  */
Configuration::ModelType Configuration::get_model_type() const {
    if (model_type == UNKNOWN) {
        throw std::runtime_error("model_type not loaded");
    }
    return model_type;
}

/**
  * Get the number of classes we use in this workload/dataset/algorithm
  */
uint64_t Configuration::get_num_classes() const {
    if (num_classes == 0) {
        throw std::runtime_error("num_classes not loaded");
    }
    return num_classes;
}

/**
  * Get the maximum number of features/columns to read from each sample
  */
uint64_t Configuration::get_limit_cols() const {
    if (limit_cols == 0) {
        cirrus::LOG<cirrus::INFO>("limit_cols not loaded");
    }
    return limit_cols;
}

/**
  * Get the flag saying whether the dataset should be normalized or not
  */
bool Configuration::get_normalize() const {
    return normalize;
}

/**
  * Get number of training input samples
  */
uint64_t Configuration::get_limit_samples() const {
    return limit_samples;
}

