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
#include "MurmurHash3.h"
#include "hash.h"

#if !defined(VW_NO_INLINE_SIMD)
#  if defined(__ARM_NEON__)
#include <arm_neon.h>
#  elif defined(__SSE2__)
#include <xmmintrin.h>
#  endif
#endif

#define HASH_BITS (18)

#define TEST_SET_SIZE (10000)

#define DEBUG
#define MAX_INPUT_LINES (40000000)

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
    //std::cout << s << std::endl;

    std::vector<std::string> parts;

    // We tokenize the string
    char* str = strdup(s.c_str());
    while (char* p = strsep(&str, " ")) {
      //std::cout << "s: " << p << std::endl;
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

std::pair<samples_type, samples_type> split_samples(const samples_type& samples) {
  float train_size = samples.size() - TEST_SET_SIZE;

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

/**
  * This code intends to mimic the LR algorithm used by VW
  */
int main() {
  auto samples = read_input("/mnt/ebs/click.train.vw");
  std::cout
    << "Read input. # total lines: " << samples.size()
    << std::endl;
  check_data(samples);
  //normalize(1 << HASH_BITS, samples);

  auto split_data = split_samples(samples);
  auto train_data = split_data.first;
  auto test_data = split_data.second;
  std::cout
    << "# train lines: " << train_data.size()
    << "# test lines: " << test_data.size()
    << std::endl;

  std::vector<float> model, w_normalized, w_adaptive, w_spare;
  model.resize( (1 << HASH_BITS) + 1);
  w_normalized.resize( (1 << HASH_BITS) + 1);
  w_adaptive.resize( (1 << HASH_BITS) + 1);
  w_spare.resize( (1 << HASH_BITS) + 1);
  //weights_hist.resize(HASH_SIZE);

  // do some pochs
  for (uint64_t n = 0; n < 1; ++n) {
    for (uint64_t i = 0; i < train_data.size(); ++i) {
      const auto& train_sample = train_data[i];
      // here before we compute the update we should do the VW normalization
      float update = predict(model, train_sample);
      //std::cout << "update0: " << update << std::endl; // this should be 0 the first time
      clip(update);
      //std::cout << "update1: " << update << std::endl;

      // check loss
      // if loss is 0 we skip
      float this_loss = std::log(1 + std::exp(-train_sample.first * update));
      //std::cout << "this_loss: " << this_loss << std::endl;

      if (this_loss == 0) {
        //std::cout << "Skipping, loss is 0" << std::endl;
        continue;
      } else if ( (i + 1) % 10 == 0) {
        //std::cout << "Skipping. Holdout sample." << std::endl;
        continue;
      }

      for (uint64_t i = 0; i < train_sample.second.size(); ++i) {
        int index = train_sample.second[i].first;
        int value = train_sample.second[i].second;
        //std::cout
        //  << "index: " << index
        //  << " value: " << value
        //  << std::endl;
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
      //std::cout
      //  << i
      //  << " update2: " << update
      //  << " nd.norm_x: " << add
      //  << " normalized_sum_norm: " << normalized_sum_norm
      //  << " update_multiplier: " << update_multiplier << std::endl;

      update *= update_multiplier;

      //double adagrad_epsilon = 10e-8;
      for (const auto& v : train_sample.second) {
        int index = v.first;
        int value = v.second;
        //std::cout 
        //  << "x: " << value
        //  << " w[spare]: " << w_spare[index]
        //  << " update: " << update
        //  << std::endl;
        model[index] += update * (value * w_spare[index]);
      }

      if (i % 10000 == 0) {
        float loss = compute_loss(model, test_data);
        std::cout
          << "total loss: " << loss
          << " average loss: " << (loss / test_data.size())
          << std::endl;
      }
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

