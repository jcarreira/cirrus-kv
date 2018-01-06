#include <unistd.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>

#include <InputReader.h>
#include <MFModel.h>

#define INPUT_PATH "ml-20m/ratings.csv"

std::unique_ptr<MFModel> mf_model;

double epsilon = 0.00001;
double learning_rate = 0.00000001;

int main() {
    InputReader input;
    SparseDataset dataset = input.read_movielens_ratings(INPUT_PATH);
    dataset.check();
    dataset.print_info();

    uint64_t samples = dataset.num_samples();
    uint64_t cols = dataset.max_cols();

    mf_mode.reset(new MFModel(samples, cols));

    // SGD learning
    for (uint64_t i = 0; 1; ++i) {
        SparseDataset ds = dataset.random_sample(20);

        auto gradient = model->minibatch_grad(ds.samples_,
                const_cast<double*>(ds.labels_.get()),
                ds.num_samples(), epsilon);

        model->sgd_update(learning_rate, gradient.get());
    }

    return 0;
}


