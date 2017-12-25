#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"

#define DEBUG

auto PSTask::connect_redis() {
  auto r  = redis_connect(REDIS_IP, REDIS_PORT);
  if (r == NULL || r -> err) { 
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
  
//  auto model_r  = redis_async_connect(REDIS_IP, REDIS_PORT);
//  if (model_r == NULL || model_r->err) { 
//    throw std::runtime_error(
//        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
//  }
  return r;
}

PSTask::PSTask(const std::string& redis_ip, uint64_t redis_port,
    uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
    uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
    uint64_t SAMPLE_BASE, uint64_t START_BASE,
    uint64_t batch_size, uint64_t samples_per_batch, 
    uint64_t features_per_sample, uint64_t nworkers,
    uint64_t worker_id) :
  MLTask(redis_ip, redis_port, MODEL_GRAD_SIZE, MODEL_BASE,
      LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
      batch_size, samples_per_batch, features_per_sample,
      nworkers, worker_id)
{
#if defined(USE_REDIS)
  r = connect_redis();
#endif
  gradientVersions.resize(nworkers, 0);
  worker_clocks.resize(nworkers, 0);
}


#ifndef USE_REDIS_CHANNEL
void PSTask::put_model(LRModel model) {
  lr_model_serializer lms(MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(model)]);
  lms.serialize(model, data.get());
  redis_put_binary_numid(r, MODEL_BASE, data.get(), lms.size(model));
}
#else
void PSTask::put_model(LRModel model) {
  lr_model_serializer lms(MODEL_GRAD_SIZE);
  auto data = std::unique_ptr<char[]>(
      new char[lms.size(model)]);
  lms.serialize(model, data.get());

  redisReply* reply = (redisReply*)redisCommand(r, "PUBLISH model %b", data.get(), lms.size(model));

  std::cout << "Redis publish"
    << " reply type: " << reply->type
    << std::endl;
}
#endif

void PSTask::get_gradient(auto r, auto& gradient, auto gradient_id) {
  int len_grad;
  lr_gradient_deserializer lgd(MODEL_GRAD_SIZE);

  char* data = redis_get_numid(r, gradient_id, &len_grad);
  if (data == nullptr) {
    throw cirrus::NoSuchIDException("");
  }
  gradient = lgd(data, len_grad);
  free(data);
}

void PSTask::publish_model(const LRModel& model) {
#ifdef USE_CIRRUS
  model_store.put(MODEL_BASE, model);
#elif defined(USE_REDIS)
  put_model(model);
#endif
}

/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void PSTask::run(const Configuration& config) {
  std::cout << "[PS] " << "PS connecting to store" << std::endl; config.print();

  lr_model_serializer lms(MODEL_GRAD_SIZE);
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);
  lr_gradient_serializer lgs(MODEL_GRAD_SIZE);
  lr_gradient_deserializer lgd(MODEL_GRAD_SIZE);

#ifdef USE_CIRRUS
  cirrus::TCPClient client;
  cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
    model_store(IP, PORT, &client, lms, lmd);
  cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
    gradient_store(IP, PORT, &client, lgs, lgd);
#endif

  std::cout << "[PS] " << "PS task initializing model" << std::endl;
  // initialize model
  LRModel model(MODEL_GRAD_SIZE);
  model.randomize();
  std::cout << "[PS] "
    << "PS publishing model at id: " << MODEL_BASE
    << " csum: " << model.checksum() << std::endl;

  publish_model(model);
#ifdef USE_CIRRUS    
  wait_for_start(PS_TASK_RANK, client, nworkers);
#elif defined(USE_REDIS)
  wait_for_start(PS_TASK_RANK, r, nworkers);
#endif
  publish_model(model);
  

  while (1) {
    // for every worker, check for a new gradient computed
    // if there is a new gradient, get it and update the model
    // once model is updated publish it
    for (int worker = 0; worker < static_cast<int>(nworkers); ++worker) {
      int gradient_id = GRADIENT_BASE + worker;
#ifdef DEBUG
      std::cout << "[PS] " << "PS task checking gradient id: " << gradient_id
        << std::endl;
#endif

      // get gradient from store
      LRGradient gradient(MODEL_GRAD_SIZE);
      try {
#ifdef USE_CIRRUS
        gradient = std::move(gradient_store.get(gradient_id));
#elif defined(USE_REDIS)
        get_gradient(r, gradient, gradient_id);
#endif
      } catch(const cirrus::NoSuchIDException& e) {
        if (!first_time) {
          std::cout
            << "PS task not able to get gradient: "
            << std::to_string(gradient_id)
            << "\n";
        }
        // this happens because the worker task
        // has not uploaded the gradient yet
        //worker--;
        continue;
      }
      first_time = false;

#ifdef DEBUG
      std::cout << "[PS] "
        << "PS task received gradient with #version: "
        << gradient.getVersion()
        << " from worker: " << worker
        << " current gradient version: " << gradientVersions[worker]
        << "\n";
#endif

      // check if this is a gradient we haven't used before
      if (gradient.getVersion() > gradientVersions[worker]) {
        update_gradient_version(gradient, worker, model, config);
      } else {
#ifdef DEBUG
        std::cout << "[PS] "
          << "Gradient from worker: " << worker
          << " is old. version: " << gradient.getVersion()
          << " tracked version: " << gradientVersions[worker]
          << std::endl;
#endif
      }
    }
  }
}

void PSTask::update_gradient_version(
    auto& gradient, int worker, LRModel& model, Configuration config ) {
#ifdef DEBUG
  std::cout << "[PS] " << "PS task received new gradient version: "
    << gradient.getVersion() << std::endl;
#endif
  // if it's new
  gradientVersions[worker] = gradient.getVersion();

  // do a gradient step and update model
#ifdef DEBUG
  std::cout << "[PS] " << "Updating model" << std::endl;
#endif

  model.sgd_update(config.get_learning_rate(), &gradient);

  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " checksum: " << model.checksum()
    << std::endl;
  // publish the model back to the store so workers can use it
#ifdef USE_CIRRUS
  model_store.put(MODEL_BASE, model);
#elif defined(USE_REDIS)
  put_model(model);
#endif
}

