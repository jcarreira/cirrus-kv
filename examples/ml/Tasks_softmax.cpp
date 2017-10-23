#include <Tasks_softmax.h>
#include <unistd.h>
#include <iostream>
#include <memory>

#include <ModelGradient.h>
#include <Dataset.h>
#include <client/TCPClient.h>
#include "object_store/FullBladeObjectStore.h"
#include "cache_manager/CacheManager.h"
#include "cache_manager/LRAddedEvictionPolicy.h"
#include "iterator/CirrusIterable.h"
#include "Serializers.h"
#include "Checksum.h"
#include "Input.h"
#include "Utils.h"
#include "Redis.h"

#include <config.h>

#define READ_AHEAD (3)

uint64_t nclasses = 10;

#ifdef USE_CIRRUS
void MLTask::wait_for_start(int index, cirrus::TCPClient& client) {
    std::cout << "wait_for_start index: " << index
        << std::endl;

    c_array_serializer<bool> start_counter_ser(1);
    c_array_deserializer<bool> start_counter_deser(1);
    auto t = std::make_shared<bool>(true);

    std::cout << "Connecting to CIRRUS store : " << std::endl;
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<bool>>
        start_store(IP, PORT, &client,
                      start_counter_ser, start_counter_deser);

    std::cout << "Updating start index: " << index
        << std::endl;
    start_store.put(START_BASE + index, t);
    std::cout << "Updated start index: " << index << std::endl;

    int num_waiting_tasks = 4;
    while (1) {
        int i = 0;
        for (i = 0; i < num_waiting_tasks; ++i) {
            std::cout << "Getting status" << std::endl;
            
            try {
                auto is_done = start_store.get(START_BASE + i);
                std::cout << "Checking " << i
                    << " " << *is_done.get()
                    << std::endl;
                if (!*is_done.get())
                    break;
            } catch(const cirrus::NoSuchIDException& e) {
                break;
            }

        }
        if (i == num_waiting_tasks)
            break;
    }
    std::cout << "Done waiting: " << index
        << std::endl;
}
#elif defined(USE_REDIS)
void MLTask::wait_for_start(int index, auto r) {
    std::cout << "wait_for_start index: " << index
        << std::endl;

    std::cout << "Updating start index: " << index
        << std::endl;
    char data = 1;
    redis_put_binary_numid(r, START_BASE + index, &data, 1);
    std::cout << "Updated start index: " << index << std::endl;

    int num_waiting_tasks = 4;
    while (1) {
        int i = 0;
        for (i = 0; i < num_waiting_tasks; ++i) {
            std::cout << "Getting status i: " << i << std::endl;
            int len;
            char* data = redis_get_numid(r, START_BASE + i, &len);
            if (data == nullptr) {
                std::cout << "wait_for_start breaking" << std::endl;
                break;
            }
            std::cout << "redis returned data: " << data << std::endl;
            auto is_done = bool(data[0]);
            std::cout << "is_done: " << is_done << std::endl;
            free(data);
            if (!is_done)
                break;
        }
        std::cout << "wait_for_start i: " << i << std::endl;
        if (i == num_waiting_tasks)
            break;
    }
    std::cout << "Done waiting: " << index
        << std::endl;
}
#endif

void LogisticTask::run(const Configuration& config, int worker) {
    std::cout << "[WORKER] "
        << "Worker task connecting to store" << std::endl;

    sm_gradient_serializer sgs(nclasses, MODEL_GRAD_SIZE);
    sm_gradient_deserializer sgd(nclasses, MODEL_GRAD_SIZE);
    sm_model_serializer sms(nclasses, MODEL_GRAD_SIZE);
    sm_model_deserializer smd(nclasses, MODEL_GRAD_SIZE);
    c_array_serializer<double> cas_samples(batch_size);
    c_array_deserializer<double> cad_samples(batch_size,
            "worker samples_store");
    c_array_serializer<double> cas_labels(samples_per_batch);
    c_array_deserializer<double> cad_labels(samples_per_batch,
            "worker labels_store", false);

#ifdef USE_CIRRUS
    cirrus::TCPClient client;
    // used to publish the gradient
    cirrus::ostore::FullBladeObjectStoreTempl<SoftmaxGradient>
        gradient_store(IP, PORT, &client, sgs, sgd);

    // this is used to access the most up to date model
    cirrus::ostore::FullBladeObjectStoreTempl<SoftmaxModel>
        model_store(IP, PORT, &client, sms, smd);
#elif defined(USE_REDIS)
    std::cout << "[WORKER] "
        << "Worker task using REDIS" << std::endl;
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
    std::cout << "[WORKER] "
        << "Error connecting to REDIS" << std::endl;
       throw std::runtime_error("Error connecting to redis server");
    }
#elif USE_S3
#endif


    uint64_t num_batches = config.get_num_samples() /
        config.get_minibatch_size();
    std::cout << "[WORKER] "
        << "num_batches: " << num_batches
        << "batch_size: " << batch_size
        << std::endl;

#ifdef USE_CIRRUS
    // this is used to access the training data sample
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        samples_store(IP, PORT, &client, cas_samples, cad_samples);
    //cirrus::LRAddedEvictionPolicy samples_policy(100);
    //cirrus::CacheManager<std::shared_ptr<double>> samples_cm(
    //        &samples_store, &samples_policy, 100);
    //cirrus::CirrusIterable<std::shared_ptr<double>> s_iter(
    //      &samples_cm, READ_AHEAD, SAMPLE_BASE, SAMPLE_BASE + num_batches - 1);
    //auto samples_iter = s_iter.begin();

    // this is used to access the training labels
    // we configure this store to return shared_ptr that do not free memory
    // because these objects will be owned by the Dataset
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        labels_store(IP, PORT, &client, cas_labels, cad_labels);
    //cirrus::LRAddedEvictionPolicy labels_policy(100);
    //cirrus::CacheManager<std::shared_ptr<double>> labels_cm(
    //        &labels_store, &labels_policy, 100);
    //cirrus::CirrusIterable<std::shared_ptr<double>> l_iter(
    //        &labels_cm, READ_AHEAD, LABEL_BASE,
    //        LABEL_BASE + num_batches - 1);
    //auto labels_iter = l_iter.begin();
#elif defined(USE_REDIS)
#endif

#ifdef USE_CIRRUS    
    wait_for_start(0, client);
#elif defined(USE_REDIS)
    wait_for_start(0, r);
#endif

    bool first_time = true;

    int batch_id = 0;
    int gradient_id = GRADIENT_BASE + worker;
    uint64_t version = 0;
    while (1) {
        // maybe we can wait a few iterations to get the model
        std::shared_ptr<double> samples;
        std::shared_ptr<double> labels;
        SoftmaxModel model(nclasses, MODEL_GRAD_SIZE);
        try {
            //std::cout << "[WORKER] "
            //    << "Worker task getting the model at id: "
            //    << MODEL_BASE
            //    << "\n";
#ifdef USE_CIRRUS
            model = model_store.get(MODEL_BASE);
#elif defined(USE_REDIS)
            int len;
            char* data = redis_get_numid(r, MODEL_BASE, &len);
            model = smd(data, len);
            free(data);
#endif

            //std::cout << "[WORKER] "
            //    << "Worker task received model csum: " << model.checksum()
            //    << std::endl
            //    << "Worker task getting the training data with id: "
            //    << (SAMPLE_BASE + samples_id)
            //    << "\n";

#ifdef USE_CIRRUS
            samples = *samples_iter;
            //samples = samples_store.get(SAMPLE_BASE + batch_id);
#elif defined(USE_REDIS)
            int len_samples;
            data = redis_get_numid(r, SAMPLE_BASE + batch_id, &len_samples);
            samples = cad_samples(data, len_samples);
            free(data);
#endif
            //auto now = get_time_ns();
            //samples = *samples_iter;
            //std::cout << "Samples Elapsed (ns): " << get_time_ns() - now << "\n";
            //std::cout << "[WORKER] "
            //    << "Worker task received training data with id: "
            //    << (SAMPLE_BASE + samples_id)
            //    << " and checksum: " << checksum(samples, batch_size)
            //    << "\n";

#ifdef USE_CIRRUS
            labels = *labels_iter;
            //labels = labels_store.get(LABEL_BASE + batch_id);
#elif defined(USE_REDIS)
            int len_labels;
            data = redis_get_numid(r, LABEL_BASE + batch_id, &len_labels);
            labels = cad_labels(data, len_labels);
            free(data);
#endif
            //labels = *labels_iter;

            //std::cout << "[WORKER] "
            //    << "Worker task received label data with id: "
            //    << samples_id
            //    << " and checksum: " << checksum(labels, samples_per_batch)
            //    << "\n";
        } catch(const cirrus::NoSuchIDException& e) {
            if (!first_time) {
                exit(0);

                // XXX fix this. We should be able to distribute
                // how many samples are there

                // wrap around
                batch_id = 0;
                continue;
            }
            // this happens because the ps task
            // has not uploaded the model yet
            std::cout << "[WORKER] "
                << "Model could not be found at id: "
                << MODEL_BASE
                << "\n";
            continue;
        }

        first_time = false;

        // Big hack. Shame on me
        //auto now = get_time_us();
        std::shared_ptr<double> l(new double[samples_per_batch],
                ml_array_nodelete<double>);
        std::copy(labels.get(), labels.get() + samples_per_batch, l.get());
        //std::cout << "Elapsed: " << get_time_us() - now << "\n";

        Dataset dataset(samples.get(), l.get(),
                samples_per_batch, features_per_sample);
#ifdef DEBUG
        dataset.print();
#endif

        dataset.check_values();

        // compute mini batch gradient
        std::unique_ptr<ModelGradient> gradient;
        try {
            gradient = model.minibatch_grad(0, dataset.samples_,
                    labels.get(), samples_per_batch, config.get_epsilon());
        } catch(...) {
            std::cout << "There was an error here" << std::endl;
            exit(-1);
        }
        gradient->setVersion(version++);

#ifdef DEBUG
        gradient->check_values();
        gradient->print();
#endif

        try {
            auto smg = *dynamic_cast<SoftmaxGradient*>(gradient.get());
#ifdef USE_CIRRUS
            gradient_store.put(
                    gradient_id, *smg);
#elif defined(USE_REDIS)
            char* data = new char[sgs.size(smg)];
            sgs.serialize(smg, data);
            redis_put_binary_numid(r, gradient_id, data, sgs.size(smg));
            delete[] data;
#endif
        } catch(...) {
            std::cout << "[WORKER] "
                << "Worker task error doing put of gradient"
                << "\n";
            exit(-1);
        }

        // move to next batch of samples
        batch_id++;

        // Wrap around
        if (batch_id == static_cast<int>(num_batches)) {
            batch_id = 0;
        }
    }
}

/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void PSTask::run(const Configuration& config) {
    std::cout << "[PS] "
        << "PS connecting to store" << std::endl;
    cirrus::TCPClient client;

    sm_model_serializer sms(nclasses, MODEL_GRAD_SIZE);
    sm_model_deserializer smd(nclasses, MODEL_GRAD_SIZE);
    sm_gradient_serializer sgs(nclasses, MODEL_GRAD_SIZE);
    sm_gradient_deserializer sgd(nclasses, MODEL_GRAD_SIZE);

#ifdef USE_CIRRUS
    cirrus::ostore::FullBladeObjectStoreTempl<SoftmaxModel>
        model_store(IP, PORT, &client,
            sms, smd);
    cirrus::ostore::FullBladeObjectStoreTempl<SoftmaxGradient>
        gradient_store(IP, PORT, &client,
                sgs, sgd);
#elif defined(USE_REDIS)
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
       throw std::runtime_error("Error connecting to redis server");
    }
#endif

    std::cout << "[PS] "
        << "PS task initializing model" << std::endl;
    // initialize model

    SoftmaxModel model(nclasses, MODEL_GRAD_SIZE);
    model.randomize();
    //model.print();
    std::cout << "[PS] "
        << "PS publishing model at id: "
        << MODEL_BASE
        << " csum: " << model.checksum()
        << std::endl;
    // publish the model back to the store so workers can use it
#ifdef USE_CIRRUS
    model_store.put(MODEL_BASE, model);
#elif defined(USE_REDIS)
    char* data = new char[sms.size(model)];
    sms.serialize(model, data);
    redis_put_binary_numid(r, MODEL_BASE, data, sms.size(model));
    delete[] data;
#endif

#if 0
    std::cout << "[PS] "
        << "PS getting model"
        << " with id: " << MODEL_BASE
        << std::endl;
#ifdef USE_CIRRUS
    auto m = model_store.get(MODEL_BASE);
#elif defined(USE_REDIS)
    int len_model;
    data = redis_get_numid(r, MODEL_BASE, &len_model);
    auto m = lmd(data, len_model);
    free(data);
#endif
    std::cout << "[PS] "
        << "PS model is here"
        << " with csum: " << m.checksum()
        << std::endl;
#endif

    // we keep a version number for the gradient produced by each worker
    std::vector<unsigned int> gradientCounts;
    gradientCounts.resize(10);

    bool first_time = true;

#ifdef USE_CIRRUS    
    wait_for_start(1, client);
#elif defined(USE_REDIS)
    wait_for_start(1, r);
#endif

    while (1) {
        // for every worker, check for a new gradient computed
        // if there is a new gradient, get it and update the model
        // once model is updated publish it
        for (int worker = 0; worker < static_cast<int>(nworkers); ++worker) {
            int gradient_id = GRADIENT_BASE + worker;

            // get gradient from store
            SoftmaxGradient gradient(nclasses, MODEL_GRAD_SIZE);
            try {
#ifdef USE_CIRRUS
                gradient = std::move(
                        gradient_store.get(gradient_id));
#elif defined(USE_REDIS)
		int len_grad;
		data = redis_get_numid(r, gradient_id, &len_grad);
                if (data == nullptr) {
                  throw cirrus::NoSuchIDException("");
                }
		gradient = sgd(data, len_grad);
		free(data);
#endif
            } catch(const cirrus::NoSuchIDException& e) {
                if (!first_time) {
                    std::cout
                        << "PS task not able to get gradient: "
                        << std::to_string(GRADIENT_BASE + gradient_id)
                        << std::endl;
                    //throw std::runtime_error(
                    //        "PS task not able to get gradient: "
                    //        + std::to_string(GRADIENT_BASE + gradient_id));
                }
                // this happens because the worker task
                // has not uploaded the gradient yet
                worker--;
                continue;
            }

            first_time = false;

            // check if this is a gradient we haven't used before
            if (gradient.getVersion() > gradientCounts[worker]) {
                gradientCounts[worker] = gradient.getVersion();

                model.sgd_update(config.get_learning_rate(), &gradient);

#ifdef USE_CIRRUS
                model_store.put(MODEL_BASE, model);
#elif defined(USE_REDIS)
		char* data = new char[sms.size(model)];
		sms.serialize(model, data);
		redis_put_binary_numid(r, MODEL_BASE, data, sms.size(model));
		delete[] data;
#endif
            }
        }
    }
}

void ErrorTask::run(const Configuration& /* config */) {
    std::cout << "Compute error task connecting to store" << std::endl;

    cirrus::TCPClient client;

    sm_model_serializer sms(nclasses, MODEL_GRAD_SIZE);
    sm_model_deserializer smd(nclasses, MODEL_GRAD_SIZE);
    c_array_serializer<double> cas_samples(batch_size);
    c_array_deserializer<double> cad_samples(batch_size,
            "error samples_store");
    c_array_serializer<double> cas_labels(samples_per_batch);
    c_array_deserializer<double> cad_labels(samples_per_batch,
            "error labels_store", false);

#ifdef USE_CIRRUS
    // this is used to access the most up to date model
    cirrus::ostore::FullBladeObjectStoreTempl<SoftmaxModel>
        model_store(IP, PORT, &client, sms, smd);

    // this is used to access the training data sample
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        samples_store(IP, PORT, &client, cas_samples, cad_samples);

    // this is used to access the training labels
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        labels_store(IP, PORT, &client, cas_labels, cad_labels);
    wait_for_start(2, client);
#elif defined(USE_REDIS)
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
       throw std::runtime_error("Error connecting to redis server");
    }
    
    wait_for_start(2, r);
#endif

    bool first_time = true;
    while (1) {
        sleep(2);

        double loss = 0;
        try {
            // first we get the model
            //std::cout << "[ERROR_TASK] getting the model at id: "
            //    << MODEL_BASE
            //    << "\n";
            SoftmaxModel model(nclasses, MODEL_GRAD_SIZE);
#ifdef USE_CIRRUS
            model = model_store.get(MODEL_BASE);
#elif defined(USE_REDIS)
	    int len_model;
	    char* data = redis_get_numid(r, MODEL_BASE, &len_model);
	    model = smd(data, len_model);
	    free(data);
#endif

            //std::cout << "[ERROR_TASK] received the model with id: "
            //    << MODEL_BASE
            //    << " csum: " << model.checksum()
            //    << "\n";

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

                //std::cout << "[ERROR_TASK] received data and labels with id: "
                //    << i << " with csmus: "
                //    << checksum(samples, batch_size) << " "
                //    << checksum(labels, samples_per_batch)
                //    << "\n";

                Dataset dataset(samples.get(), labels.get(),
                        samples_per_batch, features_per_sample);

                //std::cout << "[ERROR_TASK] checking the dataset"
                //        << "\n";

                dataset.check_values();

                //std::cout << "[ERROR_TASK] computing loss"
                //        << "\n";

                //std::cout << "First 10 samples labels: " << std::endl;
                //for (int k = 0; k < 10; ++k) {
                //    std::cout << "label " << k << ": " << labels.get()[k] << std::endl;
                //}
                loss += model.calc_loss(dataset);
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

/**
  * Load the object store with the training dataset
  * It reads from the criteo dataset files and writes to the object store
  * It signals when work is done by changing a bit in the object store
  */
void LoadingTask::run(const Configuration& config) {
    std::cout << "[LOADER] "
        << "Read input..."
        << std::endl;

    Input input;

    auto dataset = input.read_input_csv(
            config.get_input_path(),
            "\t", 30,
            config.get_limit_cols(), true);

    std::cout << "[LOADER] "
        << "First 10 labels"
        << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << "label: " << *dataset.label(i) << std::endl;
    }

#ifdef DEBUG
    dataset.check_values();
    dataset.print();
#endif
    
    c_array_serializer<double> cas_samples(batch_size);
    c_array_deserializer<double> cad_samples(batch_size,
            "loader samples_store");
    c_array_serializer<double> cas_labels(samples_per_batch);
    c_array_deserializer<double> cad_labels(samples_per_batch,
            "loader labels_store", false);

#ifdef USE_CIRRUS
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        samples_store(IP, PORT, &client, cas_samples, cad_samples);
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        labels_store(IP, PORT, &client, cas_labels, cad_labels);
#elif defined(USE_REDIS)
    std::cout << "[LOADER] "
        << "Connectin to redis"
        << std::endl;
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
       throw std::runtime_error("Error connecting to redis server");
    }
#endif

    std::cout << "[LOADER] "
        << "Adding "
        << dataset.num_samples()
        << " samples in batches of size (samples*features): "
        << batch_size
        << std::endl;

    // We put in batches of N samples
    for (unsigned int i = 0; i < dataset.num_samples() / samples_per_batch; ++i) {
        std::cout << "[LOADER] "
            << "Building samples batch" << std::endl;
        /** Build sample object
          */
        auto sample = std::shared_ptr<double>(
                new double[batch_size],
                std::default_delete<double[]>());
        // this memcpy can be avoided with some trickery
        std::cout << "[LOADER] "
            << "Memcpying: " << sizeof(double) * batch_size
            << std::endl;
        std::memcpy(sample.get(), dataset.sample(i * samples_per_batch),
                sizeof(double) * batch_size);

        std::cout << "[LOADER] "
            << "Building labels batch" << std::endl;
        /**
          * Build label object
          */
        auto label = std::shared_ptr<double>(
                new double[samples_per_batch],
                std::default_delete<double[]>());
        // this memcpy can be avoided with some trickery
        std::memcpy(label.get(), dataset.label(i * samples_per_batch),
                sizeof(double) * samples_per_batch);

        try {
            std::cout << "[LOADER] "
                << "Adding sample batch id: "
                << (SAMPLE_BASE + i)
                << " samples with size (bytes): " << sizeof(double) * batch_size
                << std::endl;

            if (i == 0) {
                std::cout << "[LOADER] "
                    << "Storing sample with id: " << 0
                    << " checksum: " << checksum(sample, batch_size) << std::endl
                    << std::endl;
            }

#ifdef USE_CIRRUS
            samples_store.put(SAMPLE_BASE + i, sample);
#elif defined(USE_REDIS)
            uint64_t len = cas_samples.size(sample);
	    char* data = new char[len];
            memset(data, 0, len);
	    cas_samples.serialize(sample, data);
	    redis_put_binary_numid(r, SAMPLE_BASE + i, data, cas_samples.size(sample));
	    delete[] data;
#endif
            // std::cout << "[LOADER] "
            //    << "Putting label" << std::endl;
#ifdef USE_CIRRUS
            labels_store.put(LABEL_BASE + i, label);
#elif defined(USE_REDIS)
            std::cout << "[LOADER] "
               << "Storing labels on REDIS with size: "
               << cas_labels.size(label)
               << std::endl;
	    data = new char[cas_labels.size(label)];
	    cas_labels.serialize(label, data);
	    redis_put_binary_numid(r, LABEL_BASE + i, data, cas_labels.size(label));
	    delete[] data;
#endif
        } catch(...) {
            std::cout << "[LOADER] "
                << "Caught exception" << std::endl;
            exit(-1);
        }
    }

    std::cout << "[LOADER] "
        << "Trying to get sample with id: " << 0
        << std::endl;

#ifdef USE_CIRRUS
    auto sample = samples_store.get(0);
#elif defined(USE_REDIS)
    int len_samples;
    char* data = redis_get_numid(r, 0, &len_samples);
    auto sample = cad_samples(data, len_samples);
    free(data);
    int len_label;

    data = nullptr;
    while (data == nullptr) {
        data = redis_get_numid(r, LABEL_BASE, &len_label);
    }
    free(data);
    std::cout << "[LOADER] "
        << "Got label with size: " << len_label << std::endl;
#endif

    std::cout << "[LOADER] "
        << "Got sample with id: " << 0
        << " checksum: " << checksum(sample, batch_size) << std::endl
        << "Added all samples"
        << std::endl;

    std::cout << "[LOADER] "
        << "Print sample 0"
        << std::endl;

#ifdef DEBUG
    for (int i = 0; i < batch_size; ++i) {
        double val = sample.get()[i];
        std::cout << val << " ";
    }
#endif
    std::cout << std::endl;

#ifdef USE_CIRRUS    
    wait_for_start(3, client);
#elif defined(USE_REDIS)
    wait_for_start(3, r);
#endif
}

