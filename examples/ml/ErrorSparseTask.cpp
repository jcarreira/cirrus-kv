#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "config.h"
#include "S3SparseIterator.h"
#include "Utils.h"
#include "async.h"
//#include "adapters/libevent.h"
#include "SparseLRModel.h"

//#define DEBUG
#define ERROR_INTERVAL_USEC (50) // time between error checks

/**
  * Ugly but necessary for now
  * both onMessage and ErrorSparseTask need access to this
  */
namespace ErrorSparseTaskGlobal {
  std::mutex model_lock;
  std::unique_ptr<SparseLRModel> model;
  std::unique_ptr<lr_model_deserializer> lmd;
  volatile bool model_is_here = true;
  uint64_t start_time;
  std::mutex mp_start_lock;
}
/**
  * Works as a cache for remote model
  */
#if 0
class ModelProxyErrorSparseTask {
  public:
    ModelProxyErrorSparseTask(auto redis_ip, auto redis_port, uint64_t mgs) :
      redis_ip(redis_ip), redis_port(redis_port), base(event_base_new())
    { 
      ErrorSparseTaskGlobal::model.reset(new SparseLRModel(mgs));
      ErrorSparseTaskGlobal::lmd.reset(new lr_model_deserializer(mgs));
      ErrorSparseTaskGlobal::mp_start_lock.lock();
    }

    static void connectCallback(const redisAsyncContext*, int) {
      std::cout << "connectCallback" << std::endl;
      ErrorSparseTaskGlobal::mp_start_lock.unlock();
    }

    static void disconnectCallback(const redisAsyncContext*, int) {
      std::cout << "disconnectCallback" << std::endl;
    }

    SparseLRModel get_model() {
      std::cout << "get_model" << std::endl;
      // wait until first model has arrived
      while (ErrorSparseTaskGlobal::model_is_here == true) {
      }

      ErrorSparseTaskGlobal::model_lock.lock();
      SparseLRModel ret = *ErrorSparseTaskGlobal::model;
      ErrorSparseTaskGlobal::model_lock.unlock();
      return ret;
    }

    static void onMessage(redisAsyncContext*, void *reply, void*) {
      redisReply *r = reinterpret_cast<redisReply*>(reply);

#ifdef DEBUG
      std::cout << "onMessage" << std::endl;
#endif
      if (r->type == REDIS_REPLY_ARRAY) {

#ifdef DEBUG
        printf("len: %lu\n", len);
#endif

        char* str_1 = r->element[1]->str;
        char* str_0 = r->element[0]->str;
        if (str_0 && strcmp(str_0, "message") == 0 &&
            str_1 && strcmp(str_1, "model") == 0) {
          const char* str = r->element[2]->str;
          //uint64_t len = r->element[2]->len;
#ifdef DEBUG
          std::cout << "Updating model at time: " << get_time_us() << std::endl;
#endif
          ErrorSparseTaskGlobal::model_lock.lock();
          ErrorSparseTaskGlobal::model->loadSerialized(str);
          //*ErrorSparseTaskGlobal::model = ErrorSparseTaskGlobal::lmd->operator()(str, len);
          ErrorSparseTaskGlobal::model_lock.unlock();
          ErrorSparseTaskGlobal::model_is_here = false;
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
        throw std::runtime_error("ModelProxyErrorSparseTask::error connecting to redis");
      }
      
      std::cout << "ErrorSparseTask: libevent attach" << std::endl;
      redisLibeventAttach(model_r, base);
      redis_connect_callback(model_r, connectCallback);
      redis_disconnect_callback(model_r, disconnectCallback);
      redis_subscribe_callback(model_r, onMessage, "model");
      
      std::cout << "eventbase dispatch" << std::endl;
      event_base_dispatch(base);
    }

    void run() {
      thread = std::make_unique<std::thread>(
          std::bind(&ModelProxyErrorSparseTask::thread_fn, this));
    }

  private:
    std::string redis_ip;
    int redis_port;

    struct event_base *base;
    std::unique_ptr<std::thread> thread;
};
#endif

void ErrorSparseTask::run(const Configuration& config) {
  std::cout << "Compute error task connecting to store" << std::endl;

  // declare serializers
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);

  std::cout << "[ERROR_TASK] connecting to redis" << std::endl;
  auto redis_con  = redis_connect(REDIS_IP, REDIS_PORT);
  if (redis_con == NULL || redis_con -> err) { 
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }

  std::cout << "Creating S3Iterator" << std::endl;
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();
  S3SparseIterator s3_iter(0, num_s3_batches, config,
      config.get_s3_size(), config.get_minibatch_size());

  // get data first
  // we get up to 10K samples
  // what we are going to use as a test set
  std::cout << "[ERROR_TASK] getting 10k minibatches"
    << std::endl;
start:
  std::vector<SparseDataset> minibatches_vec;
  auto test_range = config.get_test_range();
  for (int i = test_range.first; i < test_range.second; ++i) {
    try {
      const void* minibatch_data = s3_iter.get_next_fast();
      SparseDataset ds(reinterpret_cast<const char*>(minibatch_data),
          config.get_minibatch_size());
      minibatches_vec.push_back(ds);
    } catch(const cirrus::NoSuchIDException& e) {
      if (i == 0)
        goto start;  // no loss to be computed
      else
        break;  // we looked at all minibatches
    }
  }

  std::cout << "[ERROR_TASK] Got " << minibatches_vec.size() << " minibatches"
    << "\n";
  std::cout << "[ERROR_TASK] Building dataset"
    << "\n";
  
  //ModelProxyErrorSparseTask mp(REDIS_IP, REDIS_PORT, MODEL_GRAD_SIZE);
  //mp.run();
  ErrorSparseTaskGlobal::mp_start_lock.lock();

  wait_for_start(ERROR_SPARSE_TASK_RANK, redis_con, nworkers);
  ErrorSparseTaskGlobal::start_time = get_time_us();

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
      SparseLRModel model(MODEL_GRAD_SIZE);
      int len_model;
      std::string str_id = std::to_string(MODEL_BASE);
      char* data = redis_binary_get(redis_con, str_id.c_str(), &len_model);
      model.loadSerialized(data);
      free(data);

#ifdef DEBUG
      std::cout << "[ERROR_TASK] received the model with id: "
        << MODEL_BASE << "\n";
#endif

      int nb = mp.get_number_batches();
      std::cout 
        << "[ERROR_TASK] computing loss."
        << " number_batches: " << nb
        << std::endl;
      FEATURE_TYPE total_loss = 0;
      FEATURE_TYPE total_accuracy = 0;
      uint64_t total_num_samples = 0;
      for (auto& ds : minibatches_vec) {
        std::pair<FEATURE_TYPE, FEATURE_TYPE> ret = model.calc_loss(ds);
        total_loss += ret.first;
        total_accuracy += ret.second;
        total_num_samples += ds.num_samples();
      }
      std::cout
        << "Loss (Total/Avg): " << total_loss << "/" << (total_loss / total_num_samples)
        << " Accuracy: " << (total_accuracy / minibatches_vec.size())
        << " time(us): " << get_time_us()
        << " time from start (sec): "
        << (get_time_us() - ErrorSparseTaskGlobal::start_time) / 1000000.0
        << std::endl;
    } catch(const cirrus::NoSuchIDException& e) {
      std::cout << "run_compute_error_task unknown id" << std::endl;
    }
  }
}
