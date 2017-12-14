#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"

void ErrorTask::run(const Configuration& /* config */) {
  std::cout << "Compute error task connecting to store" << std::endl;

  cirrus::TCPClient client;

  lr_model_serializer lms(MODEL_GRAD_SIZE);
  lr_model_deserializer lmd(MODEL_GRAD_SIZE);
  c_array_serializer<double> cas_samples(batch_size);
  c_array_deserializer<double> cad_samples(batch_size,
      "error samples_store");
  c_array_serializer<double> cas_labels(samples_per_batch);
  c_array_deserializer<double> cad_labels(samples_per_batch,
      "error labels_store", false);

#ifdef USE_CIRRUS
  // this is used to access the most up to date model
  cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
    model_store(IP, PORT, &client, lms, lmd);

  // this is used to access the training data sample
  cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
    samples_store(IP, PORT, &client, cas_samples, cad_samples);

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

  bool first_time = true;
  while (1) {
    sleep(2);

    double loss = 0;
    try {
      // first we get the model
      std::cout << "[ERROR_TASK] getting the model at id: "
        << MODEL_BASE
        << "\n";
      LRModel model(MODEL_GRAD_SIZE);
#ifdef USE_CIRRUS
      model = model_store.get(MODEL_BASE);
#elif defined(USE_REDIS)
      int len_model;
      char* data = redis_get_numid(r, MODEL_BASE, &len_model);
      model = lmd(data, len_model);
      free(data);
#endif

      std::cout << "[ERROR_TASK] received the model with id: "
        << MODEL_BASE
        << " csum: " << model.checksum()
        << "\n";

      // then we compute the error
      loss = 0;
      uint64_t count = 0;

      // we keep reading until a get() fails
      for (int i = 0; 1; ++i) {
        std::shared_ptr<double> samples;
        std::shared_ptr<double> labels;

        try {
#ifdef USE_CIRRUS
          samples = samples_store.get(SAMPLE_BASE + i);
          labels = labels_store.get(LABEL_BASE + i);
#elif defined(USE_REDIS)
          int len_samples;
          data = redis_get_numid(r, SAMPLE_BASE + i, &len_samples);
          if (data == NULL) throw cirrus::NoSuchIDException("");
          samples = cad_samples(data, len_samples);
          free(data);

          int len_labels;
          data = redis_get_numid(r, LABEL_BASE + i, &len_labels);
          if (data == NULL) throw cirrus::NoSuchIDException("");
          labels = cad_labels(data, len_labels);
          free(data);
#endif
        } catch(const cirrus::NoSuchIDException& e) {
          if (i == 0)
            goto continue_point;  // no loss to be computed
          else
            goto compute_loss;  // we looked at all minibatches
        }

        std::cout << "[ERROR_TASK] received data and labels with id: "
          << i
          << "\n";

        Dataset dataset(samples.get(), labels.get(),
            samples_per_batch, features_per_sample);

#ifdef DEBUG
        std::cout << "[ERROR_TASK] checking the dataset"
          << "\n";
        dataset.check_values();
#endif

        std::cout << "[ERROR_TASK] computing loss"
          << "\n";
        loss += model.calc_loss(dataset);
        std::cout << "[ERROR_TASK] computed loss"
          << "\n";
        count++;
      }

compute_loss:
      loss = loss / count;  // compute average loss over all minibatches
    } catch(const cirrus::NoSuchIDException& e) {
      std::cout << "run_compute_error_task unknown id" << std::endl;
    }

    std::cout << "Learning loss: " << loss << std::endl;

continue_point:
    first_time = first_time;
  }
}

