#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "config.h"
#include "S3Iterator.h"
#include "Utils.h"

#define ERROR_INTERVAL_USEC (500) // time between error checks


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

#ifdef DATASET_IN_S3
  uint64_t num_s3_batches = config.get_limit_samples() / config.get_s3_size();
  S3Iterator s3_iter(0, num_s3_batches, config,
      config.get_s3_size(), features_per_sample,
      config.get_minibatch_size());
#endif

#if defined(USE_CIRRUS)
  cirrus::TCPClient client;
  lr_model_serializer lms(MODEL_GRAD_SIZE);
  // this is used to access the most up to date model
  cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
    model_store(IP, PORT, &client, lms, lmd);

  c_array_serializer<double> cas_samples(batch_size);
  // this is used to access the training data sample
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
    samples_store(IP, PORT, &client, cas_samples, cad_samples);

  c_array_serializer<double> cas_labels(samples_per_batch);
  // this is used to access the training labels
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
    labels_store(IP, PORT, &client, cas_labels, cad_labels);

  wait_for_start(ERROR_TASK_RANK, client, nworkers);
#elif defined(USE_REDIS)
  auto r  = redis_connect(REDIS_IP, REDIS_PORT);
  if (r == NULL || r -> err) { 
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }

  wait_for_start(ERROR_TASK_RANK, r, nworkers);
#endif

  // get data first
  // we get up to 10K samples
  // what we are going to use as a test set
  std::cout << "[ERROR_TASK] getting 10k minibatches"
    << "\n";
start:
  std::vector<std::shared_ptr<double>> labels_vec;
  std::vector<std::shared_ptr<double>> samples_vec;
  for (int i = 0; i < 10000; ++i) {
    std::shared_ptr<double> samples;
    std::shared_ptr<double> labels;
    try {
#ifdef DATASET_IN_S3
    std::shared_ptr<double> minibatch = s3_iter.get_next();
    unpack_minibatch(minibatch, samples, labels);
#elif defined(USE_CIRRUS)
      samples = samples_store.get(SAMPLE_BASE + i);
      labels = labels_store.get(LABEL_BASE + i);
#elif defined(USE_REDIS)
      get_samples_labels_redis(r, i, samples, labels, cad_samples, cad_labels);
#endif
    } catch(const cirrus::NoSuchIDException& e) {
      if (i == 0)
        goto start;  // no loss to be computed
      else
        goto loop_accuracy;  // we looked at all minibatches
    }
    labels_vec.push_back(labels);
    samples_vec.push_back(samples);
  }

  std::cout << "[ERROR_TASK] Got " << labels_vec.size() << " minibatches"
    << "\n";
  std::cout << "[ERROR_TASK] Building dataset"
    << "\n";

  //check_labels(labels_vec, samples_per_batch);

loop_accuracy:
  Dataset dataset(samples_vec, labels_vec,
      samples_per_batch, features_per_sample);

  std::cout << "[ERROR_TASK] Computing accuracies"
    << "\n";
  while (1) {
    usleep(ERROR_INTERVAL_USEC); //

    //double loss = 0;
    try {
      // first we get the model
#ifdef DEBUG
      std::cout << "[ERROR_TASK] getting the model at id: "
        << MODEL_BASE
        << "\n";
#endif
      LRModel model(MODEL_GRAD_SIZE);
#ifdef USE_CIRRUS
      model = model_store.get(MODEL_BASE);
#elif defined(USE_REDIS)
      model = get_model_redis(r, lmd);
#endif

#ifdef DEBUG
      std::cout << "[ERROR_TASK] received the model with id: "
        << MODEL_BASE << "\n";
#endif
      std::cout << "[ERROR_TASK] computing loss time(ms): " << get_time_ms() 
        << std::endl;
      model.calc_loss(dataset);
    } catch(const cirrus::NoSuchIDException& e) {
      std::cout << "run_compute_error_task unknown id" << std::endl;
    }
  }

  //std::cout << "Learning loss (avg per sample): " << loss << std::endl;
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

    //std::cout << "j: " << j << std::endl;
    //std::cout << "features_per_sample: " << features_per_sample << std::endl;
    if (!FLOAT_EQ(*data, 1.0) && !FLOAT_EQ(*data, 0.0))
      throw std::runtime_error(
          "Wrong label in unpack_minibatch " + std::to_string(*data));

    data++;
    std::copy(data,
        data + features_per_sample,
        samples.get() + j * features_per_sample);
  }
}
