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

    //uint64_t samples = dataset.num_samples();
    //uint64_t cols = dataset.max_features();

    // Initialize the model with initial values from dataset
    mf_model.reset(new MFModel(dataset));

    // SGD learning
    for (uint64_t i = 0; 1; ++i) {
        SparseDataset ds = dataset.random_sample(20);

        auto gradient = mf_model->minibatch_grad(ds, epsilon);

        mf_model->sgd_update(learning_rate, gradient.get());
    }

    return 0;
}


