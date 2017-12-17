#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

typedef float FEATURE_TYPE;
const std::string INPUT_PATH = "criteo_data/day_1_100k_filtered";

template<typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

void print_info(const auto& samples) {
  std::cout << "Number of samples: " << samples.size() << std::endl;
  std::cout << "Number of cols: " << samples[0].size() << std::endl;
}

int main() {
  Input input;


  //read_input(INPUT_PATH);
  //std::ifstream fin(INPUT_PATH);
  //std::string line;
  //Std::vector<std::vector<FEATURE_TYPE>> samples;
  //Std::vector<std::vector<FEATURE_TYPE>> labels;
  //Std::vector<FEATURE_TYPE> labels;

  //While (getline(fin, line)) {
  //  std::cout << "line: " << line << std::endl;
  //  std::vector<std::string> tokens = split(line, '\t');
  //  std::vector<FEATURE_TYPE> sample_features;

  //  for (const auto& t : tokens) {
  //    FEATURE_TYPE v = std::atof(t.c_str());
  //    sample_features.push_back(v);
  //  }
  //  samples.push_back(sample_features);
  //}

  //Print_info(samples);

  uint64_t num_cols = 10;
  LRModel model(num_cols);

  Dataset dataset(samples, labels, num_samples, num_features);
  gradient = model.minibatch_grad(dataset.samples_,
      labels, samples_per_batch, config.get_epsilon());

  return 0;

}
