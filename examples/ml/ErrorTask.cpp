#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "config.h"
#include "S3Iterator.h"
#include "Utils.h"
#include "async.h"
#include "adapters/libevent.h"

//#define DEBUG
#define ERROR_INTERVAL_USEC (50) // time between error checks

/**
  * Ugly but necessary for now
  * both onMessage and ErrorTask need access to this
  */
namespace ErrorTaskGlobal {
  std::mutex model_lock;
  std::unique_ptr<LRModel> model;
  std::unique_ptr<lr_model_deserializer> lmd;
  volatile bool model_is_here = true;
  uint64_t start_time;
  std::mutex mp_start_lock;
}
/**
  * Works as a cache for remote model
  */
class ModelProxyErrorTask {
  public:
    ModelProxyErrorTask(auto redis_ip, auto redis_port, uint64_t mgs) :
      redis_ip(redis_ip), redis_port(redis_port), base(event_base_new())
    { 
      ErrorTaskGlobal::model.reset(new LRModel(mgs));
      ErrorTaskGlobal::lmd.reset(new lr_model_deserializer(mgs));
      ErrorTaskGlobal::mp_start_lock.lock();
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "connectCallback" << std::endl;
      ErrorTaskGlobal::mp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    LRModel get_model() {
      std::cout << "get_model" << std::endl;
      // wait until first model has arrived
      while (ErrorTaskGlobal::model_is_here == true) {
      }

      ErrorTaskGlobal::model_lock.lock();
      LRModel ret = *ErrorTaskGlobal::model;
      ErrorTaskGlobal::model_lock.unlock();
      return ret;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      redisReply *r = reinterpret_cast<redisReply*>(reply);

#ifdef DEBUG
      std::cout << "onMessage" << std::endl;
#endif
      if (r->type == REDIS_REPLY_ARRAY) {
        const char* str = r->element[2]->str;
        uint64_t len = r->element[2]->len;

#ifdef DEBUG
        printf("len: %lu\n", len);
#endif

        char* str_1 = r->element[1]->str;
        char* str_0 = r->element[0]->str;
        if (str_0 && strcmp(str_0, "message") == 0 &&
            str_1 && strcmp(str_1, "model") == 0) {
#ifdef DEBUG
          std::cout << "Updating model at time: " << get_time_us() << std::endl;
#endif
          ErrorTaskGlobal::model_lock.lock();
          *ErrorTaskGlobal::model = ErrorTaskGlobal::lmd->operator()(str, len);
          ErrorTaskGlobal::model_lock.unlock();
          ErrorTaskGlobal::model_is_here = false;
        }
      } else {
        throw std::runtime_error("Not an array");
      }
    }

    void thread_fn() {
      std::cout << "connecting to redis.." << std::endl;
      redisAsyncContext* model_r = redis_async_connect(redis_ip.c_str(), redis_port);
      std::cout << "connected to redis.." << model_r << std::endl;
      if (!model_r) {
        throw std::runtime_error("ModelProxyErrorTask::error connecting to redis");
      }
      
      std::cout << "ErrorTask: libevent attach" << std::endl;
      redisLibeventAttach(model_r, base);
      redis_connect_callback(model_r, connectCallback);
      redis_disconnect_callback(model_r, disconnectCallback);
      redis_subscribe_callback(model_r, onMessage, "model");
      
      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&ModelProxyErrorTask::thread_fn, this));
    }

  private:
    std::string redis_ip;
    int redis_port;

    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};

#if defined(USE_REDIS)
LRModel ErrorTask::get_model_redis(auto r, auto lmd) {
  int len_model;
  char* data = redis_get_numid(r, MODEL_BASE, &len_model);
  LRModel model = lmd(data, len_model);
  free(data);
  return model;
}
          
void ErrorTask::get_samples_labels_redis(
    auto r, auto i, auto& samples, auto& labels,
    auto cad_samples, auto cad_labels) {
  int len_samples;
  char* data = redis_get_numid(r, SAMPLE_BASE + i, &len_samples);
  if (data == NULL)
    throw cirrus::NoSuchIDException("Error getting samples");
  samples = cad_samples(data, len_samples);
  free(data);

  int len_labels;
  data = redis_get_numid(r, LABEL_BASE + i, &len_labels);
  if (data == NULL)
    throw cirrus::NoSuchIDException("Error getting labels");
  labels = cad_labels(data, len_labels);
  free(data);
}
#endif

void check_labels(auto labels, auto samples_per_batch) {
  for (uint64_t i = 0; i < labels.size(); ++i) {
    for (uint64_t j = 0; j < samples_per_batch; ++j) {
      std::cout << "label " << *(labels[i].get() + j) << std::endl;
    }
  }
}

void ErrorTask::run(const Configuration& config) {
  std::cout << "Compute error task connecting to store" << std::endl;

  // declare serializers
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);
  c_array_deserializer<double> cad_samples(batch_size, "error samples_store");
  c_array_deserializer<double> cad_labels(samples_per_batch,
      "error labels_store", false);

  std::cout << "[ERROR_TASK] connecting to redis" << std::endl;
  auto r  = redis_connect(REDIS_IP, REDIS_PORT);
  if (r == NULL || r -> err) { 
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }

#ifdef DATASET_IN_S3
  std::cout << "Creating S3Iterator" << std::endl;
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();
  S3Iterator s3_iter(0, num_s3_batches, config,
      config.get_s3_size(), features_per_sample,
      config.get_minibatch_size());
#endif

  // get data first
  // we get up to 10K samples
  // what we are going to use as a test set
  std::cout << "[ERROR_TASK] getting 10k minibatches"
    << std::endl;
start:
  std::vector<std::shared_ptr<double>> labels_vec;
  std::vector<std::shared_ptr<double>> samples_vec;
  for (int i = 0; i < 10000; ++i) {
    std::cout << "[ERROR_TASK] getting iteration: " << i
      << std::endl;
    std::shared_ptr<double> samples;
    std::shared_ptr<double> labels;
    try {
#ifdef DATASET_IN_S3
    std::shared_ptr<double> minibatch = s3_iter.get_next();
    std::cout << "[ERROR_TASK] unpacking"
      << std::endl;
    unpack_minibatch(minibatch, samples, labels);
#elif defined(USE_REDIS)
      get_samples_labels_redis(r, i, samples, labels, cad_samples, cad_labels);
#endif
    } catch(const cirrus::NoSuchIDException& e) {
      if (i == 0)
        goto start;  // no loss to be computed
      else
        break;  // we looked at all minibatches
    }
    labels_vec.push_back(labels);
    samples_vec.push_back(samples);
  }

  std::cout << "[ERROR_TASK] Got " << labels_vec.size() << " minibatches"
    << "\n";
  std::cout << "[ERROR_TASK] Building dataset"
    << "\n";
  
  ModelProxyErrorTask mp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE);
  mp.run();
  ErrorTaskGlobal::mp_start_lock.lock();

  wait_for_start(ERROR_TASK_RANK, r, nworkers);
  ErrorTaskGlobal::start_time = get_time_us();

  //check_labels(labels_vec, samples_per_batch);

  Dataset dataset(samples_vec, labels_vec,
      samples_per_batch, features_per_sample);

  std::cout << "[ERROR_TASK] Computing accuracies"
    << "\n";
  while (1) {
    usleep(ERROR_INTERVAL_USEC); //

    try {
      // first we get the model
#ifdef DEBUG
      std::cout << "[ERROR_TASK] getting the model at id: "
        << MODEL_BASE
        << "\n";
#endif
      LRModel model(MODEL_GRAD_SIZE);
      model = mp.get_model();

#ifdef DEBUG
      std::cout << "[ERROR_TASK] received the model with id: "
        << MODEL_BASE << "\n";
#endif
      std::cout << "[ERROR_TASK] computing loss" << std::endl;
      std::pair<double, double> ret = model.calc_loss(dataset);
      std::cout
        << "Loss: " << ret.first << " Accuracy: " << ret.second
        << "Avg Loss: " << (ret.first / dataset.num_samples())
        << " time(us): " << get_time_us()
        << " time from start (us): " << (get_time_us() - ErrorTaskGlobal::start_time)
        << std::endl;
    } catch(const cirrus::NoSuchIDException& e) {
      std::cout << "run_compute_error_task unknown id" << std::endl;
    }
  }
}

void ErrorTask::unpack_minibatch(
    std::shared_ptr<double> minibatch,
    auto& samples, auto& labels) {
  uint64_t num_samples_per_batch = batch_size / features_per_sample;

  samples = std::shared_ptr<double>(
      new double[batch_size], std::default_delete<double[]>());
  labels = std::shared_ptr<double>(
      new double[num_samples_per_batch], std::default_delete<double[]>());

  for (uint64_t j = 0; j < num_samples_per_batch; ++j) {
    double* data = minibatch.get() + j * (features_per_sample + 1);
    labels.get()[j] = *data;

    if (!FLOAT_EQ(*data, 1.0) && !FLOAT_EQ(*data, 0.0))
      throw std::runtime_error(
          "Wrong label in unpack_minibatch " + std::to_string(*data));

    data++;
    std::copy(data,
        data + features_per_sample,
        samples.get() + j * features_per_sample);
  }
}
