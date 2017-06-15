#include <math.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <string>
#include <random>
#include <vector>

#include "src/object_store/FullBladeObjectStore.h"
#include "src/utils/StringUtils.h"
#include "src/utils/logging.h"

// TODO: Remove hardcoded IP and PORT
const char PORT[] = "12345";
const char IP[] = "10.10.49.83";

struct sample_struct {
    double data[6];
};

/*
 * Print vector contents
 */
template <typename T>
void print_vector(T w, int d) {
    for (int i = 0; i < d; ++i)
        std::cout << w[i] << " ";
    std::cout << std::endl;
}


/**
  * Read input dataset in csv format
  * and load it into samples and labels vectors
  */
void read_input_csv(const std::string& input_file,
        std::vector<double>* labels,
        std::vector<std::vector<double>>* samples) {
    std::cout << "Reading input file: " << input_file << std::endl;

    std::ifstream fin(input_file, std::ifstream::in);
    if (!fin)
        throw std::runtime_error("Error opening input file");

    std::string line;
    while (getline(fin, line)) {
        char* nl = strdup(line.c_str());
        char* s = nl;

        std::vector<double> sample;
        while (char* l = strsep(&s, ",")) {
            double v = cirrus::string_to<double>(l);
            sample.push_back(v);
        }
        free(nl);

        if (sample.back() != 1 && sample.back() != 0)
            throw std::runtime_error("Label must be 0 or 1");

        labels->push_back(sample.back());
        sample.pop_back();
        samples->push_back(sample);

        print_vector(&sample[0], sample.size());

        if (samples->size() % 1000000 == 0)
            std::cout << "Read line: " << samples->size() << std::endl;
    }
}

template<typename T, typename C>
void load_input_data(
        cirrus::ostore::FullBladeObjectStoreTempl<T>* samples_store,
        cirrus::ostore::FullBladeObjectStoreTempl<C>* labels_store,
        std::vector<double>* labels,
        std::vector<std::vector<double>>* samples) {
    int d = (*samples)[0].size();
    // 0 to nsamples-1 is the data
    // nsamples to 2*nsamples is the labels

    for (uint64_t i = 0; i < samples->size(); ++i) {
        if (sizeof(sample_struct) != sizeof(double) * d) {
            std::cout << sizeof(sample_struct) << " "
                << d << " " << sizeof(double) * d << std::endl;
            throw std::runtime_error("Size mismatch");
        }

        samples_store->put(&(*samples)[i][0], sizeof(sample_struct), i);
        labels_store->put(&(*labels)[i], sizeof(double), samples->size() + i);
    }
}

template<typename T, typename C>
double sdot(T a, C b, int d) {
    double x = 0;
    for (int i = 0; i < d; ++i)
        x += a[i] * b[i];

    double exp_res = exp(-x);
    if (isnan(exp_res)) {
        throw std::runtime_error("NaN error in sdot");
    }

    return 1.0 / (1.0 + exp_res);
}

template<typename T, typename C>
void train(
        cirrus::ostore::FullBladeObjectStoreTempl<T>* samples_store,
        cirrus::ostore::FullBladeObjectStoreTempl<C>* labels_store,
        std::vector<double>* labels,
        std::vector<std::vector<double>>* samples,
        uint64_t nsamples,
        uint64_t d) {
    std::cout << "nsamples: " << nsamples << " d: " << d << std::endl;

    std::vector<double> w;
    w.resize(d + 1);

    unsigned int seed = 42;
    for (int i = 0; i < d + 1; ++i)
        w[i] = 1.0 * rand_r(&seed) / RAND_MAX - 0.5;

    uint64_t iteration = 1;
    while (1) {
        double alpha = 1 / sqrt(iteration);  // learning rate
        double lambda = 0.1;  // regularization rate
        // 0 to nsamples-1 is the data
        // nsamples to 2*nsamples is the labels

        int minibatch = 5;

        for (int m = 0; m < minibatch; ++m) {
            double sample[d + 1] = {0};  // + 1 for the intercept
            double label = 0;
            int r = rand_r(&seed);
            int sample_idx = r % nsamples;  // random value in [0, nsamples-1]

            samples_store->get(sample_idx, reinterpret_cast<T*>(sample));
            sample[d] = 1;

            labels_store->get(nsamples + sample_idx, static_cast<C*>(&label));

            // w = w + alpha * (y_i - s(w^T x_i)) * x_i
            //                       ----A-------
            //                  ------B----------
            double A = sdot(w, sample, d + 1);
            double B = label - A;

            for (int i = 0; i < d + 1; ++i) {
                double wi = w[i];
                w[i] = w[i] + alpha * (sample[i] * B - 2 * lambda * wi);
            }
        }

        print_vector(w, d + 1);

        std::cerr << "Iteration: " << iteration++ << std::endl;

        //  we use the raw in-memory data to calculate accuracy
        if (iteration % 1000 == 0) {
            std::cout << "Checking accuracy: " << std::endl;
            print_vector(w, d + 1);
            int error = 0;
            for (int k = 0; k < samples->size(); ++k) {
                std::vector<double> sample = (*samples)[k];
                (*samples)[k].push_back(1);  // add intercept

                double res = sdot(w, (*samples)[k], d+1);

                if ( (res > 0.5 && (*labels)[k] == 0) ||
                     (res < 0.5 && (*labels)[k] == 1))
                    error++;
            }
            std::cout << "Accuracy: "
                << 1 - 1.0 * error / samples->size()
                << std::endl;
        }
    }
}

void normalize_data(std::vector<std::vector<double>>* samples) {
    int d = (*samples)[0].size();
    double mean[d] = {0};
    for (int i = 0; i < samples->size(); ++i) {
        for (int j = 0; j < d; ++j) {
            mean[j] = (*samples)[i][j] / samples->size();
        }
    }

    double var[d] = {0};
    for (int i = 0; i < samples->size(); ++i) {
        for (int j = 0; j < d; ++j) {
            var[j] += powl((*samples)[i][j] - mean[j], 2) / samples->size();
        }
    }

    for (int i = 0; i < samples->size(); ++i) {
        for (int j = 0; j < d; ++j) {
            assert(var[j] != 0);
            (*samples)[i][j] = ( (*samples)[i][j] - mean[j]) / sqrt(var[j]);
        }
    }
}

auto main(int argc, char** argv) -> int {
    if (argc != 2) {
        throw std::runtime_error("./log_reg <input_data>");
    }

    std::vector<double> labels;
    std::vector<std::vector<double>> samples;
    read_input_csv(argv[1], &labels, &samples);
    normalize_data(&samples);

    cirrus::LOG<cirrus::INFO>("Processed csv input");
    cirrus::LOG<cirrus::INFO>("#Samples/Labels: ", samples.size());

    cirrus::ostore::FullBladeObjectStoreTempl<sample_struct>
        samples_store(IP, PORT);
    cirrus::ostore::FullBladeObjectStoreTempl<double>
        labels_store(IP, PORT);
    load_input_data(&samples_store, &labels_store, &labels, &samples);

    train(&samples_store, &labels_store,
          &labels, &samples,
          samples.size(), samples[0].size());

    return 0;
}
