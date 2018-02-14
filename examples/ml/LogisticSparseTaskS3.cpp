#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "S3SparseIterator.h"
#include "async.h"
#include "PSSparseServerInterface.h"

#if !defined(VW_NO_INLINE_SIMD)
#  if defined(__ARM_NEON__)
#include <arm_neon.h>
#  elif defined(__SSE2__)
#include <xmmintrin.h>
#  endif
#endif


#include <pthread.h>

#undef DEBUG

typedef std::vector<float> model_type;
typedef std::vector<std::pair<int, std::vector<std::pair<int, FEATURE_TYPE>>>> samples_type;
typedef std::vector<std::pair<int, FEATURE_TYPE>> sample_type;

float first_derivative(float prediction, float label) {
  float v = - label / (1 + exp(label * prediction));
  std::cout
    << "first_derivative"
    << " label: " << label
    << " prediction: " << prediction
    << " result: " << v
    << std::endl;
  return v;
}


float getSquareGrad(float prediction, float label) {
  float d = first_derivative(prediction, label);
  assert(!(std::isinf(d) || std::isnan(d)));
  return d * d;
}

float compute_normalized_sum_norm(const sample_type& sample, const model_type& w_normalized) {
  float res = 0;
  for (uint64_t i = 0; i < sample.size(); ++i) {
    int index = sample[i].first;
    int value = sample[i].second;
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

void clip(float& update) {
  if (update > 50)
    update = 50;
  if (update < -50)
    update = -50;
}

float predict(const model_type& model, const sample_type& sample) {
  float res = 0;
  for (const auto& v : sample) {
    int index = v.first;
    int value = v.second;
    res += model.at(index) * value;
    std::cout 
      << "res: " << res
      << " index: " << index
      << " value: " << value
      << " model v: " << model.at(index)
      << std::endl;
  }
  return res;
}

float getUnsafeUpdate(float prediction, float label, float update_scale) {
  float d = exp(label * prediction);
  return label * update_scale / (1 + d);
}

class SparseModelGet {
  public:
    SparseModelGet(const std::string& ps_ip, int ps_port) :
      ps_ip(ps_ip), ps_port(ps_port) {
      psi = std::make_unique<PSSparseServerInterface>(ps_ip, ps_port);
    }

    SparseLRModel get_new_model(const SparseDataset& ds) {
      return psi->get_sparse_model(ds);
    }

  private:
    std::unique_ptr<PSSparseServerInterface> psi;
    std::string ps_ip;
    int ps_port;
};

void check_redis(auto r) {
  if (r == NULL || r -> err) {
    std::cout << "[WORKER] "
      << "Error connecting to REDIS"
      << " IP: " << REDIS_IP
      << std::endl;
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
}

// we need global variables because of the static callbacks
namespace LogisticSparseTaskGlobal {
  std::unique_ptr<SparseModelGet> sparse_model_get;
  redisContext* redis_con;
  PSSparseServerInterface* psint;
}

void LogisticSparseTaskS3::push_gradient(LRSparseGradient* lrg) {
#ifdef DEBUG
  auto before_push_us = get_time_us();
  std::cout << "Publishing gradients" << std::endl;
#endif
  LogisticSparseTaskGlobal::psint->send_gradient(*lrg);
#ifdef DEBUG
  std::cout << "Published gradients!" << std::endl;
  auto elapsed_push_us = get_time_us() - before_push_us;
  static uint64_t before = 0;
  if (before == 0)
    before = get_time_us();
  auto now = get_time_us();
  std::cout << "[WORKER] "
      << "Worker task published gradient"
      << " with version: " << lrg->getVersion()
      << " at time (us): " << get_time_us()
      << " took(us): " << elapsed_push_us
      << " bw(MB/s): " << std::fixed <<
         (1.0 * lrg->getSerializedSize() / elapsed_push_us / 1024 / 1024 * 1000 * 1000)
      << " since last(us): " << (now - before)
      << "\n";
  before = now;
#endif
}

// get samples and labels data
bool LogisticSparseTaskS3::get_dataset_minibatch(
    auto& dataset,
    auto& s3_iter) {
#ifdef DEBUG
  auto start = get_time_us();
#endif

  const void* minibatch = s3_iter.get_next_fast();
#ifdef DEBUG
  auto finish1 = get_time_us();
#endif
  dataset.reset(new SparseDataset(reinterpret_cast<const char*>(minibatch),
        config.get_minibatch_size())); // this takes 11 us

#ifdef DEBUG
  auto finish2 = get_time_us();
  double bw = 1.0 * dataset->getSizeBytes() /
    (finish2-start) * 1000.0 * 1000 / 1024 / 1024;
  std::cout << "[WORKER] Get Sample Elapsed (S3) "
    << " minibatch size: " << config.get_minibatch_size()
    << " part1(us): " << (finish1 - start)
    << " part2(us): " << (finish2 - finish1)
    << " BW (MB/s): " << bw
    << " at time: " << get_time_us()
    << "\n";
#endif
  return true;
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


float s_1_float(float x) {
  float res = 1.0 / (1.0 + exp(-x));
  if (std::isnan(res) || std::isinf(res)) {
    throw std::runtime_error(
        std::string("s_1_float generated nan/inf x: " + std::to_string(x)
          + " res: " + std::to_string(res)));
  }

  return res;
}


float compute_loss(model_type& model, std::vector<std::unique_ptr<SparseDataset>>& test_datasets) {
  float res_loss = 0;
  for (uint64_t j = 0; j < test_datasets.size(); ++j) {
    const SparseDataset& test_dataset = *test_datasets[j];
    for (uint64_t i = 0; i < test_dataset.num_samples(); ++i) {
      const std::vector<std::pair<int, FEATURE_TYPE>>& sample =
        test_dataset.get_row(i);
      float prediction = predict(model, sample);
      if (std::isinf(prediction) || std::isnan(prediction)) {
        throw std::runtime_error("Invalid value");
      }
      float s1 = s_1_float(prediction);
      float label = test_dataset.get_label(i);

      if (label == -1) {
        label = 0;
      }

      float loss = label * log_aux(s1) + (1 - label) * log_aux(1 - s1);
      res_loss += loss;
    }
  }
  return res_loss;
}

float compute_loss(model_type& model, SparseDataset& test_dataset) {
  float res_loss = 0;
  for (uint64_t i = 0; i < test_dataset.num_samples(); ++i) {
    const std::vector<std::pair<int, FEATURE_TYPE>>& sample =
      test_dataset.get_row(i);
    float prediction = predict(model, sample);
    if (std::isinf(prediction) || std::isnan(prediction)) {
      throw std::runtime_error("Invalid value");
    }
    float s1 = s_1_float(prediction);
    float label = test_dataset.get_label(i);

    if (label == -1) {
      label = 0;
    }

    float loss = label * log_aux(s1) + (1 - label) * log_aux(1 - s1);
    res_loss += loss;
  }
  return res_loss;
}


static auto connect_redis() {
  std::cout << "[WORKER] "
    << "Worker task connecting to REDIS. "
    << "IP: " << REDIS_IP << std::endl;
  auto redis_con = redis_connect(REDIS_IP, REDIS_PORT);
  check_redis(redis_con);
  return redis_con;
}

void LogisticSparseTaskS3::run(const Configuration& config, int worker) {
  std::cout << "Starting LogisticSparseTaskS3"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();
  this->config = config;

  LogisticSparseTaskGlobal::psint = new PSSparseServerInterface(PS_IP, PS_PORT);

  std::cout << "Connecting to redis.." << std::endl;
  redis_lock.lock();
  LogisticSparseTaskGlobal::redis_con = connect_redis();
  redis_lock.unlock();

  //uint64_t MODEL_BASE = (1000000000ULL);
  LogisticSparseTaskGlobal::sparse_model_get =
    std::make_unique<SparseModelGet>(PS_IP, PS_PORT);

  std::cout << "[WORKER] " << "num s3 batches: " << num_s3_batches
    << std::endl;
  wait_for_start(WORKER_SPARSE_TASK_RANK + worker, LogisticSparseTaskGlobal::redis_con, nworkers);

  // Create iterator that goes from 0 to num_s3_batches
  auto train_range = config.get_train_range();
  S3SparseIterator s3_iter(
      train_range.first, train_range.second,
      config, config.get_s3_size(), config.get_minibatch_size(),
      worker);

  std::cout << "[WORKER] starting loop" << std::endl;

  //uint64_t version = 1;
  //SparseLRModel model(MODEL_GRAD_SIZE);

  // all the VW datastructures
  std::vector<float> model, w_normalized, w_adaptive, w_spare;
  model.resize( (1 << CRITEO_HASH_BITS) + 1); 
  w_normalized.resize( (1 << CRITEO_HASH_BITS) + 1); 
  w_adaptive.resize( (1 << CRITEO_HASH_BITS) + 1); 
  w_spare.resize( (1 << CRITEO_HASH_BITS) + 1); 

  std::vector<std::unique_ptr<SparseDataset>> test_datasets;
  for (int j = 0; j < 100; ++j) {
    std::unique_ptr<SparseDataset> dataset;
    if (!get_dataset_minibatch(dataset, s3_iter)) {
      continue;
    }
    test_datasets.push_back(std::move(dataset));
  }

  uint64_t count = 0;
  while (1) {
    // get data, labels and model
    std::unique_ptr<SparseDataset> dataset;
    if (!get_dataset_minibatch(dataset, s3_iter)) {
      continue;
    }

    if ( (count++ % 10) == 0) {
      float loss = compute_loss(model, test_datasets);
      std::cout
        << "total loss: " << loss
        << " average loss: " << (loss / (100 * config.get_minibatch_size()))
        << std::endl;
    }

    for (uint64_t i_sample = 0; i_sample < config.get_minibatch_size(); ++i_sample) {
      // get ith sample
      const std::vector<std::pair<int, FEATURE_TYPE>>& sample =
        dataset->get_row(i_sample);
      FEATURE_TYPE label = dataset->get_label(i_sample);
      // predict pre-sigmoid value
      float update = predict(model, sample);
      if (std::isinf(update) || std::isnan(update)) {
        throw std::runtime_error("Invalid update value");
      }
      // clip that value
      clip(update);
      // compute loss
      float this_loss = std::log(1 + std::exp(-label * update));
      //std::cout << "this_loss: " << this_loss << std::endl;

      if (this_loss == 0) {
        //std::cout << "Skipping, loss is 0" << std::endl;
        continue;
      }
      //} else if ( (i + 1) % 10 == 0) {
      //std::cout << "Skipping. Holdout sample." << std::endl;
      //continue;
      //}

      for (uint64_t i = 0; i < sample.size(); ++i) {
        int index = sample[i].first;
        int value = sample[i].second;
        std::cout << "index: " << index << " value: " << value << std::endl;
        assert(value != 0);

        float sq_grad = getSquareGrad(update, label) * (value * value);
        std::cout << "sq_grad: " << sq_grad << std::endl;
        w_adaptive[index] += sq_grad;

        // there is a check here that we dont do for now
        float x_abs = fabsf(value);
        if (x_abs > w_normalized[index]) {
          if (w_normalized[index] > 0.) {
            float rescale = w_normalized[index] / x_abs;
            model[index] *= rescale;
          }
          w_normalized[index] = x_abs;
        }

        std::cout << "w_adaptive[index]: " << w_adaptive[index] << std::endl;
        float rate_decay = InvSqrt(w_adaptive[index]);
        std::cout << "rate_decay: " << rate_decay << std::endl;
        float inv_norm = 1.f / w_normalized[index];
        std::cout << "inv_norm: " << inv_norm << std::endl;
        rate_decay *= inv_norm;
        w_spare[index] = rate_decay;
        assert(!(std::isinf(w_spare[index]) || std::isnan(w_spare[index])));
      }
      
      update = getUnsafeUpdate(update, label, 0.5);

      static float normalized_sum_norm = 0;
      float add =  compute_normalized_sum_norm(sample, w_normalized);
      assert(!(std::isinf(add) || std::isnan(add)));
      normalized_sum_norm += add;
      static uint64_t total_weight = 0;
      total_weight++;
      float update_multiplier = std::sqrt((float)(total_weight / normalized_sum_norm));
      assert(!(std::isinf(update_multiplier) || std::isnan(update_multiplier)));
      update *= update_multiplier;

      std::cout
        << "update: " << update
        << " update_multiplier: " << update_multiplier
        << std::endl;

      for (const auto& v : sample) {
        int index = v.first;
        int value = v.second;
        model[index] += update * (value * w_spare[index]);
      }
    }
  }
}

