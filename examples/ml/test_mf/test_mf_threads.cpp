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
int number_movies, number_users;


void thread_func(SparseDataset& dataset) {
  double learning_rate = 0.01;

  // SGD learning
  uint64_t batch_size = 20;
  double epsilon = 0.00001;
  while (1) {
    for (uint64_t i = 0; i + batch_size < number_users; i += batch_size) {
      SparseDataset ds = dataset.sample_from(i, batch_size);

      // we update the model here
      mf_model->sgd_update(learning_rate, i, ds, epsilon);

    }
  }
}

void netflix() {
    InputReader input;

    std::cout << "Reading movie dataset" << std::endl;
    SparseDataset dataset = input.read_netflix_ratings("nf_parsed", &number_users, &number_movies);
    dataset.check();
    dataset.print_info();

    // Initialize the model with initial values from dataset
    int nfactors = 10;
    mf_model.reset(new MFModel(number_users, number_movies, nfactors));

    std::cout << "Starting SGD learning" << std::endl;

    std::vector<std::thread*> threads;
    for (uint64_t i = 0; i < 10; ++i) {
      threads.push_back(new std::thread(thread_func, std::ref(dataset)));
    }

    while (1) {
      sleep(1);
      double loss = mf_model->calc_loss(dataset);
      std::cout 
        << " MSE: " << loss << std::endl;
    }

    for (auto thread : threads) {
      thread->join();
    }

}


int main() {
  netflix();
  return 0;
}


