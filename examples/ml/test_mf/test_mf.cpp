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

#define INPUT_PATH "movielens_data/ratings01.csv"

std::unique_ptr<MFModel> mf_model;


void movielens() {
    InputReader input;
    int number_movies, number_users;

    std::cout << "Reading movie dataset" << std::endl;
    SparseDataset dataset = input.read_movielens_ratings(INPUT_PATH, &number_users, &number_movies);
    dataset.check();
    dataset.print_info();

    // Initialize the model with initial values from dataset
    int nfactors = 200;
    mf_model.reset(new MFModel(number_users, number_movies, nfactors));

    std::cout << "Starting SGD learning" << std::endl;
    
    double learning_rate = 1;

    // SGD learning
    uint64_t batch_size = 200;
    double loss = 0;
    double prev_loss = 0;
    double epsilon = 0.00001;
    for (uint64_t i = 0; 1; i += batch_size) {
        SparseDataset ds = dataset.sample_from(i, batch_size);

        // we update the model here
        auto gradient = mf_model->sgd_update(learning_rate, i, ds, epsilon);

        if (i % 100 == 0) {
          loss = mf_model->calc_loss(dataset);
          std::cout 
            << "Iteration " << i
            << " MSE: " << loss << std::endl;


          if (prev_loss == loss) {
            learning_rate *= 0.8;
            std::cout << "learning_rate: " << learning_rate << std::endl;
          }
          prev_loss = loss;
        }
    }
}

void netflix() {
    InputReader input;
    int number_movies, number_users;

    std::cout << "Reading movie dataset" << std::endl;
    SparseDataset dataset = input.read_netflix_ratings("nf_parsed_1M", &number_users, &number_movies);
    dataset.check();
    dataset.print_info();

    // Initialize the model with initial values from dataset
    int nfactors = 100;
    mf_model.reset(new MFModel(number_users, number_movies, nfactors));

    std::cout << "Starting SGD learning" << std::endl;
    
    double learning_rate = 0.5;

    // SGD learning
    uint64_t batch_size = 20;
    double loss = 0;
    double prev_loss = 0;
    double epsilon = 0.00001;
    for (uint64_t i = 0; 1; i += batch_size) {
        SparseDataset ds = dataset.sample_from(i, batch_size);

        // we update the model here
        auto gradient = mf_model->sgd_update(learning_rate, i, ds, epsilon);

        if (i % 1000 == 0) {
          loss = mf_model->calc_loss(dataset);
          std::cout 
            << "Iteration " << i
            << " MSE: " << loss << std::endl;


          if (prev_loss == loss) {
            learning_rate *= 0.8;
            std::cout << "learning_rate: " << learning_rate << std::endl;
          }
          prev_loss = loss;
        }
    }
}


int main() {
  netflix();
  return 0;
}


