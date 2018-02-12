#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <utility>
#include <map>
#include <sstream>
#include <cmath>
#include "MurmurHash3.h"
#include "hash.h"

#define HASH_SIZE (1 << 20)

#define TEST_SET_SIZE (2000)

typedef std::vector<float> model_type;
typedef std::vector<std::pair<int, std::vector<std::pair<int, int>>>> samples_type;
typedef std::pair<int, std::vector<std::pair<int, int>>> sample_type;




void normalize(uint64_t hash_size, samples_type& samples) {
  std::vector<float> max_val_feature(hash_size);
  std::vector<float> min_val_feature(hash_size);

  for (const auto& w : samples) {
    for (const auto& v : w.second) {
      int index = v.first;
      float value = v.second;
      max_val_feature[index] = std::max(value, max_val_feature[index]);
      min_val_feature[index] = std::min(value, min_val_feature[index]);
    }
  }
  for (auto& w : samples) {
    for (auto& v : w.second) {
      int index = v.first;
      v.second = (v.second - min_val_feature[index]) / 
        (max_val_feature[index] - min_val_feature[index]);
    }
  }
}

template <class C>
C string_to(const std::string& s) {
  if (s == "") {
    return 0;
  } else {
    std::istringstream iss(s);
    C c;
    iss >> c;
    return c;
  }
}

uint64_t hash_f(const char* s) {
  uint64_t seed = 100;
  uint64_t hash_otpt[2]= {0};
  MurmurHash3_x64_128(s, strlen(s), seed, hash_otpt);
  return hash_otpt[0];
}

samples_type
read_input(const std::string& fname) {
  std::ifstream fin(fname);
  std::string s;

  std::cout << "Reading " << fname << std::endl;

  samples_type result;
  uint64_t line_count = 0;
  while (std::getline(fin, s)) {
    //std::cout << s << std::endl;

    std::vector<std::string> parts;

    char* str = strdup(s.c_str());
    while (char* p = strsep(&str, " ")) {
      //std::cout << "s: " << p << std::endl;
      parts.push_back(p);
      if (str == nullptr) {
        break;
      }
    }

    std::string label = parts[0];
    if (strcmp(parts[2].c_str(), "|i") != 0) {
      throw std::runtime_error("|i not found");
    }

    //std::cout << "Parsing integer values" << std::endl;
    std::map<int,int> feature_values;
    uint64_t i = 3;
    while (1) {
      //std::cout << "parts[i]: " << parts[i] << std::endl;
      if (strcmp(parts[i].c_str(), "|c") == 0) {
        break;
      }
      std::string pair = parts[i].substr(1);
      size_t sep = pair.find(":");
      std::string index = pair.substr(0, sep);
      std::string value = pair.substr(sep + 1);
      feature_values[string_to<int>(index)] = string_to<int>(value);
      ++i;
    }
    ++i; // move to categorical features
    
    //std::cout << "Parsing categorical values" << std::endl;
    for (; i < parts.size(); ++i) {
      uint64_t h = hash_f(parts[i].c_str()) % HASH_SIZE;
      feature_values[h]++;
    }

    sample_type l;
    for (const auto& v : feature_values) {
      auto index = v.first;
      auto value = v.second;
      l.second.push_back(std::make_pair(index, value));
    }
    l.first = string_to<int>(label);
    //for (const auto& pair : l) {
    //  std::cout << pair.first << ":" << pair.second << " ";
    //}
    //std::cout << std::endl;
    result.push_back(l);

    if ((line_count % 10000) == 0) {
      std::cout << line_count << std::endl;
    }
    line_count++;

    if (line_count > 5000000)
      break;
  }
  return result;
}

void clip(float& update) {
  if (update > 50)
    update = 50;
  if (update < -50)
    update = -50;
}

float compute_update(const model_type& model, const sample_type& sample) {
  float res = 0;
  for (const auto& v : sample.second) {
    int index = v.first;
    int value = v.second;
    res += model.at(index) * value;
    //std::cout 
    //  << "res: " << res
    //  << " value: " << value
    //  << " model v: " << model.at(index)
    //  << std::endl;
  }
  return res;
}

float getUnsafeUpdate(float prediction, float label, float update_scale) {
  float d = exp(label * prediction);
  return label * update_scale / (1 + d);
}

float s_1_float(float x) {
  float res = 1.0 / (1.0 + exp(-x));
  if (std::isnan(res) || std::isinf(res)) {
    throw std::runtime_error(
        std::string("s_1_float generated nan/inf x: " + std::to_string(x)
          + " res: " + std::to_string(res)));
  }

  return res;
}

double log_aux(double v) {
  if (v == 0) {
    return -10000;
  }

  double res = log(v);
  if (std::isnan(res) || std::isinf(res)) {
    throw std::runtime_error(
        std::string("log_aux generated nan/inf v: ") +
        std::to_string(v));
  }
  return res;
}

float compute_loss(model_type& model, samples_type& samples) {
  float res_loss = 0;
  for (uint64_t i = 0; i < TEST_SET_SIZE; ++i) {
    float prediction = compute_update(model, samples.at(i));
    float s1 = s_1_float(prediction);
    float label = samples.at(i).first;

    if (label == -1) {
      label = 0;
    }

    float loss = label * log_aux(s1) + (1 - label) * log_aux(1 - s1);
    
    std::cout 
      << "prediction: " << prediction
      << " s1: " << s1
      << " label: " << label
      << " loss: " << loss
      << std::endl;

    res_loss += loss;
  }
  return res_loss;
}

void normalize_sample(model_type& model, model_type& w_normalized, sample_type& sample) {
  std::vector<std::pair<int,int>>& sample_values = sample.second;
  for (uint64_t i = 0; i < sample_values.size(); ++i) {
    int index = sample_values[i].first;
    int value = sample_values[i].second;
    float abs_value = fabsf(value);

    if (abs_value > w_normalized.at(index)) {  // new scale discovered
      if (w_normalized.at(index) > 0.) {
        float rescale = w_normalized.at(index) / abs_value;
        model[index] *= rescale;
      }
      w_normalized[index] = abs_value;
    }
  }
}

std::pair<samples_type, samples_type> split_samples(const samples_type& samples) {
  float train_size = 0.98 * samples.size();

  samples_type train_data;
  samples_type test_data;
  uint64_t i = 0;
  for (; i < train_size; ++i) {
    train_data.push_back(samples[i]);
  }
  for (; i < samples.size(); ++i) {
    test_data.push_back(samples[i]);
  }
  return std::make_pair(train_data, test_data);
}

/**
  * This code intends to mimic the LR algorithm used by VW
  */
int main() {
  auto samples = read_input("/mnt/ebs/click.train.vw");
  normalize(HASH_SIZE, samples);

  auto split_data = split_samples(samples);
  auto train_data = split_data.first;
  auto test_data = split_data.second;

  std::vector<float> model, w_normalized;
  model.resize(HASH_SIZE);
  w_normalized.resize(HASH_SIZE);
  //weights_hist.resize(HASH_SIZE);

  for (uint64_t i = 0; i < train_data.size(); ++i) {
    const auto& train_sample = train_data[i];
    // here before we compute the update we should do the VW normalization
    float update = compute_update(model, train_sample);
    std::cout << "update0: " << update << std::endl;
    //clip(update);

    std::cout << "update1: " << update << std::endl;
  
    update = getUnsafeUpdate(update, train_sample.first, 0.01);
    std::cout << "update2: " << update << std::endl;

    double adagrad_epsilon = 10e-8;
    for (const auto& v : train_sample.second) {
      int index = v.first;
      int value = v.second;
      model[index] += update * value;
    }

    if (i % 1000 == 0) {
      float loss = compute_loss(model, test_data);
      std::cout
        << "total loss: " << loss
        << " average loss: " << (loss / TEST_SET_SIZE)
        << std::endl;
    }
  }
  // compute ret = model x sample
  // clip ret to [-50, 50]

  // compute loss of model on sample

  //compute update=getUnsafeUpdate(ec.pred.scalar, ld.label, update_scale)
  // ec.pred.scalar is the model x sample value
  // ld.label is the label 1 or -1
  // update_scale is the learning rate (0.5)

  // train part
  // update *= update_multiplier
  
  // update_multiplier explained here

  // update model by doing
  // wight_i += update

  //-1 '10000000 |i I9:181 I8:2 I1:1 I3:5 I2:1 I5:1382 I4:0 I7:15 I6:4 I11:2 I10:1 I13:2 |c 21ddcdc9 f54016b9 2824a5f6 37c9c164 b2cb9c98 a8cd5504 e5ba7672 891b62e7 8ba8b39a 1adce6ef a73ee510 1f89b562 fb936136 80e26c9b 68fd1e64 de7995b8 7e0ccccf 25c83c98 7b4723c4 3a171ecb b1252a9d 07b5194c 9727dd16 c5c50484 e8b83407

  return 0;
}

