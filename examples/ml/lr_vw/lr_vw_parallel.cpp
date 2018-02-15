#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <utility>
#include <map>
#include <sstream>
#include <cassert>
#include <cmath>
#include <thread>
#include <unistd.h>
#include "MurmurHash3.h"
#include "hash.h"

#if !defined(VW_NO_INLINE_SIMD)
#  if defined(__ARM_NEON__)
#include <arm_neon.h>
#  elif defined(__SSE2__)
#include <xmmintrin.h>
#  endif
#endif

#define HASH_BITS (19)

#define TEST_SET_SIZE (10000)

#define DEBUG
#define MAX_INPUT_LINES (6000000)

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
      //std::cout << "index: " << index << std::endl;
      max_val_feature.at(index) = std::max(value, max_val_feature[index]);
      min_val_feature.at(index) = std::min(value, min_val_feature[index]);
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

static inline float InvSqrt(float x) {
#if !defined(VW_NO_INLINE_SIMD)
#  if defined(__ARM_NEON__)
  // Propagate into vector
  float32x2_t v1 = vdup_n_f32(x);
  // Estimate
  float32x2_t e1 = vrsqrte_f32(v1);
  // N-R iteration 1
  float32x2_t e2 = vmul_f32(e1, vrsqrts_f32(v1, vmul_f32(e1, e1)));
  // N-R iteration 2
  float32x2_t e3 = vmul_f32(e2, vrsqrts_f32(v1, vmul_f32(e2, e2)));
  // Extract result
  return vget_lane_f32(e3, 0);
#  elif (defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64))
  __m128 eta = _mm_load_ss(&x);
  eta = _mm_rsqrt_ss(eta);
  _mm_store_ss(&x, eta);
#else
  x = quake_InvSqrt(x);
#  endif
#else
  x = quake_InvSqrt(x);
#endif

  return x;
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

uint64_t hash(const std::string& s, uint64_t seed, uint64_t mask) {
  // uint64_t h = hash_f(s.c_str()) % (mask + 1);
  uint64_t original_hash = hashstring(s, seed);
  return (original_hash & mask);
}

samples_type
read_input(const std::string& fname) {
  std::ifstream fin(fname);
  std::string s;

  std::cout << "Reading " << fname << std::endl;

  samples_type result;
  uint64_t line_count = 0;
  while (std::getline(fin, s)) {
    std::vector<std::string> parts;

    // We tokenize the string
    char* str = strdup(s.c_str());
    while (char* p = strsep(&str, " ")) {
      parts.push_back(p);
      if (str == nullptr) {
        break;
      }
    }
    free(str);

    std::string label = parts[0];
    if (strcmp(parts[2].c_str(), "|i") != 0) {
      throw std::runtime_error("|i not found");
    }

    uint64_t parse_mask = ((1 << HASH_BITS) - 1);
    std::map<int,int> feature_values;
    feature_values[(1<<HASH_BITS)] = 1; // constant bias
    // Parsing integer values
    uint64_t i = 3;
    uint64_t i_seed = hashstring("i", 0);
    //std::cout << "i_seed: " << i_seed << std::endl;
    while (1) {
      //std::cout << "int parts[i]: " << parts[i] << std::endl;
      if (strcmp(parts[i].c_str(), "|c") == 0) {
        break;
      }
      std::string pair = parts[i].substr(0);
      size_t sep = pair.find(":");
      std::string index = pair.substr(0, sep);
      std::string value = pair.substr(sep + 1);
      uint64_t h = hash(index, i_seed, parse_mask);
      uint64_t value_int = string_to<int>(value);
      //std::cout << index << " - - " << value << "\n";
      if (value_int == 0) {
        ++i;
        continue;
      }
      feature_values[h] = value_int;
#ifdef DEBUG
      //std::cout
      //  << "index: " << index
      //  << " parse_mask: " << parse_mask
      //  << "hash: " << h
      //  << " value: " << value
      //  << std::endl;
#endif
      ++i;
    }
    ++i; // move to categorical features
    
     // Hashing c values
    uint64_t c_seed = hashstring("c", 0);
    //std::cout << "c_seed: " << c_seed << std::endl;
    for (; i < parts.size(); ++i) {
      //std::cout << "cat parts[i]: " << parts[i] << std::endl;
      uint64_t h = hash(parts[i], c_seed, parse_mask);
      //std::cout << h << " / " << feature_values[h] << "\n";
      feature_values[h]++;
    }

    //
    sample_type l;
    for (const auto& v : feature_values) {
      auto index = v.first;
      auto value = v.second;
      //std::cout << line_count << " index: " << index << " value: " << value << std::endl;
      if (v.second == 0) {
        continue; // I think this doesn't matter too much
      }
      //assert(v.second);
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

    if (line_count > MAX_INPUT_LINES)
      break;
    //std::cout << "sample size: " << result.back().second.size() << std::endl;
  }

  assert(result[0].second.size() == 37);

  return result;
}

void clip(float& update) {
  if (update > 50)
    update = 50;
  if (update < -50)
    update = -50;
}

float predict(const model_type& model, const sample_type& sample) {
  float res = 0;
  for (const auto& v : sample.second) {
    int index = v.first;
    int value = v.second;
    res += model.at(index) * value;
    //std::cout 
    //  << "res: " << res
    //  << " index: " << index
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

float compute_loss(model_type& model, samples_type& test_samples) {
  float res_loss = 0;
  for (uint64_t i = 0; i < test_samples.size(); ++i) {
    float prediction = predict(model, test_samples.at(i));
    if (std::isinf(prediction) || std::isnan(prediction)) {
      throw std::runtime_error("Invalid value");
    }
    float s1 = s_1_float(prediction);
    float label = test_samples.at(i).first;

    if (label == -1) {
      label = 0;
    }

    float loss = label * log_aux(s1) + (1 - label) * log_aux(1 - s1);
    
    //std::cout 
    //  << "prediction: " << prediction
    //  << " s1: " << s1
    //  << " label: " << label
    //  << " loss: " << loss
    //  << std::endl;

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

std::pair<samples_type, samples_type> split_samples(const samples_type& samples, uint64_t first_set_size) {
  samples_type first_set;
  samples_type second_set;
  uint64_t i = 0;
  for (; i < first_set_size; ++i) {
    first_set.push_back(samples[i]);
  }
  for (; i < samples.size(); ++i) {
    second_set.push_back(samples[i]);
  }
  return std::make_pair(first_set, second_set);
}

float compute_normalized_sum_norm(const sample_type& sample, const model_type& w_normalized) {
  float res = 0;
  for (uint64_t i = 0; i < sample.second.size(); ++i) {
    int index = sample.second[i].first;
    int value = sample.second[i].second;
    //std::cout
    //  << "index: " << index
    //  << " value: " << value
    //  << std::endl;
    //std::cout
    //  << "x: " << value
    //  << " w_normalized[index]: " << w_normalized[index]
    //  << std::endl;

    float add = (1.0 * value * value) / (1.0 * w_normalized[index] * w_normalized[index]);
    res += add;
  }
  return res;
}

float first_derivative(float prediction, float label) {
  float v = - label / (1 + exp(label * prediction));
  return v;
}

float getSquareGrad(float prediction, float label) {
  float d = first_derivative(prediction, label);
  return d * d;
}

void check_data(const samples_type& samples) {
  for (const auto& v : samples) {
    if (v.first != -1 && v.first != 1) {
      throw std::runtime_error("wrong sample");
    }
    for (uint64_t i = 0; i < v.second.size(); ++i) {
      //int index = v.second[i].first;
      int value = v.second[i].second;
      if (value == 0)
        throw std::runtime_error("Value == 0");
    }
  }
}

class Worker {
  public:
  Worker(samples_type& train_data) :
    train_data(train_data)
  {
    model.resize( (1 << HASH_BITS) + 1);
    w_normalized.resize( (1 << HASH_BITS) + 1);
    w_adaptive.resize( (1 << HASH_BITS) + 1);
    w_spare.resize( (1 << HASH_BITS) + 1);
  }

  void run() {
    for (uint64_t n = 0; n < 10; ++n) {
      std::cout << "Starting epoch: " << (n + 1) << std::endl;

      for (uint64_t i = 0; i < train_data.size(); ++i) {
        const auto& train_sample = train_data[i];
        // here before we compute the update we should do the VW normalization
        float update = predict(model, train_sample);
        clip(update);

        // check loss
        // if loss is 0 we skip
        float this_loss = std::log(1 + std::exp(-train_sample.first * update));
        if (this_loss == 0) {
          continue;
        } else if ( (i + 1) % 10 == 0) {
          continue;
        }

        for (uint64_t i = 0; i < train_sample.second.size(); ++i) {
          int index = train_sample.second[i].first;
          int value = train_sample.second[i].second;
          assert(value != 0);

          w_adaptive[index] += getSquareGrad(update, train_sample.first) * (value * value); 

          // there is a check here that we dont do for now
          float x_abs = fabsf(value);
          if (x_abs > w_normalized[index]) {
            if (w_normalized[index] > 0.) {      
              float rescale = w_normalized[index] / x_abs;
              model[index] *= rescale;
            }
            w_normalized[index] = x_abs;
          }

          float rate_decay = InvSqrt(w_adaptive[index]);
          float inv_norm = 1.f / w_normalized[index];
          rate_decay *= inv_norm;
          w_spare[index] = rate_decay;
        }

        update = getUnsafeUpdate(update, train_sample.first, 0.5);

        static float normalized_sum_norm = 0;
        float add =  compute_normalized_sum_norm(train_sample, w_normalized);
        normalized_sum_norm += add;
        static uint64_t total_weight = 0;
        total_weight++;
        float update_multiplier = std::sqrt((float)(total_weight / normalized_sum_norm));
        update *= update_multiplier;

        //double adagrad_epsilon = 10e-8;
        for (const auto& v : train_sample.second) {
          int index = v.first;
          int value = v.second;
          float upd = update * (value * w_spare[index]);
          model[index] += upd;
        }

      }
    }
  }

  void launch() {
    thr = new std::thread(
        std::bind(&Worker::run, this));
  }

public:
  samples_type& train_data;
  std::thread* thr;
  std::vector<float> model, w_normalized, w_adaptive, w_spare;
};

class ParallelWorker {
public:
  Worker(samples_type& train_data,
      std::vector<float>& shared_model,
      std::vector<float>& shared_w_normalized,
      std::mutex& ps_lock) :
    train_data(train_data), shared_model(shared_model),
    shared_w_normalized(shared_w_normalized), ps_lock(ps_lock) {
    model.resize( (1 << HASH_BITS) + 1);
    w_normalized.resize( (1 << HASH_BITS) + 1);
    w_adaptive.resize( (1 << HASH_BITS) + 1);
    w_spare.resize( (1 << HASH_BITS) + 1);
  }

  void run() {
    for (uint64_t n = 0; n < 10; ++n) {
      std::cout << "Starting epoch: " << (n + 1) << std::endl;
      
      // index, update, w_normalized
      std::vector<std::pair<int, std::pair<float, float>> grads;

      for (uint64_t i = 0; i < train_data.size(); ++i) {
        const auto& train_sample = train_data[i];
        // here before we compute the update we should do the VW normalization
        float update = predict(model, train_sample);
        clip(update);

        // check loss
        // if loss is 0 we skip
        float this_loss = std::log(1 + std::exp(-train_sample.first * update));
        if (this_loss == 0) {
          continue;
        } else if ( (i + 1) % 10 == 0) {
          continue;
        }

        for (uint64_t i = 0; i < train_sample.second.size(); ++i) {
          int index = train_sample.second[i].first;
          int value = train_sample.second[i].second;
          assert(value != 0);

          w_adaptive[index] += getSquareGrad(update, train_sample.first) * (value * value); 

          // there is a check here that we dont do for now
          float x_abs = fabsf(value);
          if (x_abs > w_normalized[index]) {
            if (w_normalized[index] > 0.) {      
              float rescale = w_normalized[index] / x_abs;
              model[index] *= rescale;
            }
            w_normalized[index] = x_abs;
          }

          float rate_decay = InvSqrt(w_adaptive[index]);
          float inv_norm = 1.f / w_normalized[index];
          rate_decay *= inv_norm;
          w_spare[index] = rate_decay;
        }

        update = getUnsafeUpdate(update, train_sample.first, 0.5);

        static float normalized_sum_norm = 0;
        float add =  compute_normalized_sum_norm(train_sample, w_normalized);
        normalized_sum_norm += add;
        static uint64_t total_weight = 0;
        total_weight++;
        float update_multiplier = std::sqrt((float)(total_weight / normalized_sum_norm));
        update *= update_multiplier;

        //double adagrad_epsilon = 10e-8;
        for (const auto& v : train_sample.second) {
          int index = v.first;
          int value = v.second;
          model[index] += update * (value * w_spare[index]);
          grads.push_back(std::make_pair(index, std::make_pair(upd, w_normalized[index])));
        }

        if (i > 0 && i % 10 == 0) {
          ps_lock.lock();
          for (const auto& v : grads) {
            int index = grads.first;
            float upd = grad.second.first;
            float w_normalized = grads.second.second;
            if (w_normalized > shared_w_normalized[index]) {
              shared_model[index] *= (shared_w_normalized[index] / w_normalized);
              shared_w_normalized[index] = w_normalized;
            }
            shared_model[index] += upd;
          }
          ps_lock.unlock();
        }
      }
    }
  }

  void launch() {
    thr = new std::thread(
        std::bind(&Worker::run, this));
  }

public:
  std::mutex& ps_lock;
  std::vector<float>& shared_model;
  std::vector<float>& shared_w_normalized;
  samples_type& train_data;
  std::thread* thr;
  std::vector<float> model, w_normalized, w_adaptive, w_spare;
};

/**
  * This code intends to mimic the LR algorithm used by VW
  */
int main() {
  auto samples = read_input("/mnt/ebs/click.train.vw");
  std::cout
    << "Read input. # total lines: " << samples.size()
    << std::endl;
  check_data(samples);

  auto split_data = split_samples(samples, samples.size() - TEST_SET_SIZE);
  auto train_data = split_data.first;
  auto test_data = split_data.second;

  auto two_train_sets = split_samples(train_data, train_data.size() / 2);
  auto train_data1 = two_train_sets.first;
  auto train_data2 = two_train_sets.second;
  std::cout
    << "# train1 lines: " << train_data1.size()
    << "# train2 lines: " << train_data2.size()
    << "# test lines: " << test_data.size()
    << std::endl;

  std::mutex ps_lock;
  std::vector<float> shared_model, shared_w_normalized;
  shared_model.resize( (1 << HASH_BITS) + 1);
  shared_w_normalized.resize( (1 << HASH_BITS) + 1);
  ParallelWorker w1(train_data1, shared_model, shared_w_normalized, ps_lock);
  //ParallelWorker w2(train_data2, shared_model, shared_w_normalized, ps_lock);
  //Worker w1(train_data1);
  //Worker w2(train_data2);
  w1.launch();
  //w2.launch();
  
  while (1) {
    sleep(1);
    std::vector<float>& model = w1.model;
    float loss = compute_loss(shared_model, test_data);
    std::cout
      << "total loss: " << loss
      << " average loss: " << (loss / test_data.size())
      << std::endl;
  }

  return 0;
}

