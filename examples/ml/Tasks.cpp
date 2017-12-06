#include <examples/ml/Tasks.h>

#include <ModelGradient.h>
#include <Dataset.h>

#include <unistd.h>
#include <iostream>
#include <memory>
#include <algorithm>
#include <vector>

#include "object_store/FullBladeObjectStore.h"
#include "cache_manager/CacheManager.h"
#include "cache_manager/LRAddedEvictionPolicy.h"
#include "iterator/CirrusIterable.h"
#include "Serializers.h"
#include "Checksum.h"
#include "Input.h"
#include "Utils.h"
#include "Redis.h"
#include "RedisIterator.h"

//#define DEBUG
#define READ_AHEAD (6)

#include "config.h"

#ifdef USE_CIRRUS
void MLTask::wait_for_start(int index, cirrus::TCPClient& client, int nworkers) {
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

    int num_waiting_tasks = 3 + nworkers;
    while (1) {
        int i = 0;
        for (i = 1; i < num_waiting_tasks; ++i) {
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
void MLTask::wait_for_start(int index, auto r, int nworkers) {
    std::cout
        << "Waiting for all workers to start. index: " << index
        << std::endl;

    char data = 1; // bit used to indicate start
    redis_put_binary_numid(r, START_BASE + index, &data, 1);

    int num_waiting_tasks = 3 + nworkers;
    while (1) {
        int i = 1;
        for (; i < num_waiting_tasks; ++i) {
#ifdef DEBUG
            std::cout << "Getting status i: " << i << std::endl;
#endif
            int len; // length of object received
            char* data = redis_get_numid(r, START_BASE + i, &len);
            if (data == nullptr) {
                std::cout << "wait_for_start breaking" << std::endl;
                break;
            }
            auto is_done = bool(data[0]);
            free(data);
            if (!is_done)
                break;
        }
        if (i == num_waiting_tasks) {
            break;
        } else {
            std::cout << "Worker " << i << " not done" << std::endl;
        }
    }
    std::cout << "Worker " << index << " done waiting: " << std::endl;
}
#endif

void LogisticTask::run(const Configuration& config, int worker) {
    std::cout << "[WORKER] "
        << "Worker task connecting to store" << std::endl;

    lr_gradient_serializer lgs(MODEL_GRAD_SIZE);
    lr_gradient_deserializer lgd(MODEL_GRAD_SIZE);
    lr_model_serializer lms(MODEL_GRAD_SIZE);
    lr_model_deserializer lmd(MODEL_GRAD_SIZE);
    c_array_serializer<double> cas_samples(batch_size);
    c_array_deserializer<double> cad_samples(batch_size,
            "worker samples_store");
    c_array_serializer<double> cas_labels(samples_per_batch);
    c_array_deserializer<double> cad_labels(samples_per_batch,
            "worker labels_store", false);

#ifdef USE_CIRRUS
    std::cout << "[WORKER] "
        << "Worker task using CIRRUS" << std::endl;
    cirrus::TCPClient client;
    // used to publish the gradient
    cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
        gradient_store(IP, PORT, &client, lgs, lgd);

    // this is used to access the most up to date model
    cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
        model_store(IP, PORT, &client,
                lms, lmd);
#elif defined(USE_REDIS)
    std::cout << "[WORKER] "
        << "Worker task connecting to REDIS. "
        << "IP: " << REDIS_IP << std::endl;
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);

    if (r == NULL || r -> err) { 
       std::cout << "[WORKER] "
           << "Error connecting to REDIS"
           << " IP: " << REDIS_IP
           << std::endl;
       throw std::runtime_error(
            "Error connecting to redis server. IP: " + std::string(REDIS_IP));
    }
#elif USE_S3
#endif


#ifdef PRELOAD_DATA
    std::cout << "PRELOADING is set. Should use other task code" << std::endl;
    exit(-1);
#endif

    uint64_t num_batches = config.get_num_samples() /
        config.get_minibatch_size();
    std::cout << "[WORKER] "
        << "num_batches: " << num_batches << std::endl;

#ifdef USE_PREFETCH
    RedisIterator rit_samples(SAMPLE_BASE, SAMPLE_BASE + 121, REDIS_IP, REDIS_PORT);
    RedisIterator rit_labels(LABEL_BASE, LABEL_BASE + 121, REDIS_IP, REDIS_PORT);
#endif

#ifdef USE_CIRRUS

    // this is used to access the training data sample
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        samples_store(IP, PORT, &client, cas_samples, cad_samples);
    cirrus::LRAddedEvictionPolicy samples_policy(6);
    cirrus::CacheManager<std::shared_ptr<double>> samples_cm(
            &samples_store, &samples_policy, 6);
    cirrus::CirrusIterable<std::shared_ptr<double>> s_iter(
          &samples_cm, READ_AHEAD, SAMPLE_BASE,
          SAMPLE_BASE + num_batches - 1);

    auto samples_iter = s_iter.begin();

    // this is used to access the training labels
    // we configure this store to return shared_ptr that do not free memory
    // because these objects will be owned by the Dataset
    cirrus::ostore::FullBladeObjectStoreTempl<std::shared_ptr<double>>
        labels_store(IP, PORT, &client,
                cas_labels, cad_labels);
    cirrus::LRAddedEvictionPolicy labels_policy(6);
    cirrus::CacheManager<std::shared_ptr<double>> labels_cm(
            &labels_store, &labels_policy, 6);
    cirrus::CirrusIterable<std::shared_ptr<double>> l_iter(
            &labels_cm, READ_AHEAD, LABEL_BASE,
            LABEL_BASE + num_batches - 1);
    auto labels_iter = l_iter.begin();
#elif defined(USE_REDIS)
#endif

#ifdef USE_CIRRUS    
    wait_for_start(3 + worker, client, nworkers);
#elif defined(USE_REDIS)
    wait_for_start(3 + worker, r, nworkers);
#endif

    bool first_time = true;

    uint64_t batch_id = 0;
    int gradient_id = GRADIENT_BASE + worker;
    uint64_t version = 0;
    while (1) {
        // maybe we can wait a few iterations to get the model
        std::shared_ptr<double> samples;
        std::shared_ptr<double> labels;
        LRModel model(MODEL_GRAD_SIZE);
        try {
#ifdef DEBUG
            std::cout << "[WORKER] "
                << "Worker task getting the model at id: "
                << MODEL_BASE
                << "\n";
#endif

            auto before_model = get_time_ns();
#ifdef USE_CIRRUS
            model = model_store.get(MODEL_BASE);
#elif defined(USE_REDIS)
            int len;
#ifdef DEBUG
            std::cout << "[WORKER] "
                << "Worker task getting the model at id: "
                << MODEL_BASE
                << "\n";
#endif
            char* data = redis_get_numid(r, MODEL_BASE, &len);
            if (data == nullptr) {
                throw cirrus::NoSuchIDException("");
            }
            auto after_model = get_time_ns();

#ifdef DEBUG
            std::cout << "[WORKER] "
                << "Worker task deserializing the model at id: "
                << MODEL_BASE
                << "\n";
#endif
            model = lmd(data, len);
#ifdef DEBUG
            std::cout << "[WORKER] "
                << "Worker task freeing the model at id: "
                << MODEL_BASE
                << "\n";
#endif
            free(data);
#endif
            std::cout << "[WORKER] "
                << "model get (ns): " << (after_model - before_model)
                << std::endl;

#ifdef DEBUG
            std::cout << "[WORKER] "
                << "Worker task received model csum: " << model.checksum()
                << std::endl
                << "Worker task getting the training data with id: "
                << (SAMPLE_BASE + batch_id)
                << "\n";
#endif

            //auto before_samples = get_time_ns();
            //auto start = auto start = std::chrono::high_resolution_clock::now();
            std::chrono::steady_clock::time_point start =
                                             std::chrono::steady_clock::now();

#ifdef USE_CIRRUS
            samples = *samples_iter;
#elif defined USE_PREFETCH
            int len_samples = batch_size * sizeof(double);
            data = rit_samples.get_next();
            samples = cad_samples(data, len_samples);
            free(data);
#elif defined(USE_REDIS)
            int len_samples;
            data = redis_get_numid(r, SAMPLE_BASE + batch_id, &len_samples);
            if (data == nullptr) {
                throw cirrus::NoSuchIDException("");
            }
            samples = cad_samples(data, len_samples);
            free(data);
#endif
            std::chrono::steady_clock::time_point finish =
                                       std::chrono::steady_clock::now();
            uint64_t elapsed_ns =
                 std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                      finish-start).count();
            //uint64_t elapsed_ns = get_time_ns() - before_samples;
            double bw = 1.0 * batch_size * sizeof(double) /
                elapsed_ns * 1000.0 * 1000 * 1000 / 1024 / 1024;
#ifdef USE_CIRRUS
            std::cout << "Get Sample " << batch_id << " Elapsed (CIRRUS) "
#elif defined(USE_REDIS)
            std::cout << "Get Sample " << batch_id << " Elapsed (REDIS) "
#endif
                << " batch size: " << batch_size
                << " len samples: " << len_samples
                << " ns: " << elapsed_ns
                << " BW (MB/s): " << bw
                << "at time: " << get_time_us()
                << "\n";
#ifdef DEBUG
            std::cout << "[WORKER] "
                << "Worker task received training data with id: "
                << (SAMPLE_BASE + batch_id)
                << " and checksum: " << checksum(samples, batch_size)
                << "\n";
#endif

            auto before_labels = get_time_ns();
#ifdef USE_CIRRUS
            labels = *labels_iter;
#elif defined USE_PREFETCH
            data = rit_labels.get_next();
            int len_labels = batch_size * sizeof(double) / 10;
            labels = cad_labels(data, len_labels);
            free(data);
#elif defined(USE_REDIS)
            int len_labels;
            data = redis_get_numid(r, LABEL_BASE + batch_id, &len_labels);
            if (data == nullptr) {
                throw cirrus::NoSuchIDException("");
            }
            labels = cad_labels(data, len_labels);
            free(data);
#endif
            auto after_labels = get_time_ns();
            std::cout << "[WORKER] "
                << "labels get (ns): " << (after_labels - before_labels)
                << "at time: " << get_time_us()
                << "\n";

#ifdef DEBUG
            std::cout << "[WORKER] "
                << "Worker task received label data with id: "
                << batch_id
                << " and checksum: " << checksum(labels, samples_per_batch)
                << "\n";
#endif
        } catch(const cirrus::NoSuchIDException& e) {
            if (!first_time) {
                std::cout << "[WORKER] "
                    << "Exiting"
                    << std::endl;
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

#ifdef DEBUG
        std::cout << "[WORKER] "
            << "Building and printing dataset"
            << " with checksums: "
            << checksum(samples, batch_size) << " "
            << checksum(labels, samples_per_batch)
            << "\n";
#endif

        // Big hack. Shame on me
        // auto now = get_time_us();
        std::shared_ptr<double> l(new double[samples_per_batch],
                ml_array_nodelete<double>);
        std::copy(labels.get(), labels.get() + samples_per_batch, l.get());
        // std::cout << "Elapsed: " << get_time_us() - now << "\n";

        Dataset dataset(samples.get(), l.get(),
                samples_per_batch, features_per_sample);
#ifdef DEBUG
        dataset.print();

        std::cout << "[WORKER] "
            << "Worker task checking dataset with csum: " << dataset.checksum()
            << "\n";

        dataset.check_values();

        std::cout << "[WORKER] "
            << "Worker task computing gradient"
            << "\n";
#endif

	auto now = get_time_us();
        // compute mini batch gradient
        std::unique_ptr<ModelGradient> gradient;
        try {
            gradient = model.minibatch_grad(0, dataset.samples_,
                    labels.get(), samples_per_batch, config.get_epsilon());
        } catch(...) {
            std::cout << "There was an error here" << std::endl;
            exit(-1);
        }
        auto elapsed_us = get_time_us() - now;
        std::cout << "[WORKER] "
            << "Gradient compute time (us): " << elapsed_us
            << "at time: " << get_time_us()
            << " version " << version
            << "\n";
        gradient->setVersion(version++);

#ifdef DEBUG
        std::cout << "[WORKER] "
            << "Checking and Printing gradient: "
            << "\n";
#endif

#ifdef DEBUG
        gradient->check_values();
        gradient->print();

        std::cout << "[WORKER] "
            << "Worker task storing gradient at id: "
            << gradient_id
            << "\n";
#endif
        try {
            auto lrg = dynamic_cast<LRGradient*>(gradient.get());
            if (lrg == nullptr) {
                throw std::runtime_error("Wrong dynamic cast");
            }
#ifdef USE_CIRRUS
            gradient_store.put(
                    gradient_id, *lrg);
#elif defined(USE_REDIS)
	    auto data = std::unique_ptr<char[]>(
	        	    new char[lgs.size(*lrg)]);
            lgs.serialize(*lrg, data.get());
            redis_put_binary_numid(r, gradient_id, data.get(), lgs.size(*lrg));
#endif
        } catch(...) {
            std::cout << "[WORKER] "
                << "Worker task error doing put of gradient"
                << "\n";
            exit(-1);
        }
        std::cout << "[WORKER] "
            << "Worker task stored gradient at id: " << gradient_id
            << " at time (us): " << get_time_us()
            << "\n";

        // move to next batch of samples
        batch_id++;
#ifdef USE_CIRRUS
        samples_iter++;
        labels_iter++;
#endif

        // Wrap around
        if (batch_id == SAMPLE_BASE + num_batches) {
            batch_id = 0;
#ifdef USE_CIRRUS
            samples_iter = s_iter.begin();
            labels_iter = l_iter.begin();
#endif
        }
    }
}

void LogisticTaskPreloaded::get_data_samples(auto r, uint64_t left_id, uint64_t right_id, auto& samples, auto& labels) {
        
    std::cout << "get_data_samples" << std::endl;

    c_array_serializer<double> cas_samples(batch_size);
    c_array_deserializer<double> cad_samples(batch_size,
            "worker samples_store");
    c_array_serializer<double> cas_labels(samples_per_batch);
    c_array_deserializer<double> cad_labels(samples_per_batch,
            "error labels_store", false);

    uint64_t size = right_id - left_id;
    samples.resize(size);
    labels.resize(size);
    for (uint64_t i = left_id; i < right_id; ++i) {
        // get samples
        int len_samples;
        std::cout << "Getting batch i: " << i << std::endl;
        char* data = redis_get_numid(r, SAMPLE_BASE + i, &len_samples);
        if (data == nullptr) {
            std::cout << "Sample batch " << i << " not found" << std::endl;
            i--;
            continue;
        }
        std::cout << "Got batch i: " << i << std::endl;
        auto sample = cad_samples(data, len_samples);
        std::cout << "Deserialized sample i: " << i << std::endl;
        samples[i - left_id] = sample;
        std::cout << "Assigned sample i: " << i << std::endl;
        free(data);
        std::cout << "Freed dat i: " << i << std::endl;
        //--------------
        
        int len_labels;
        std::cout << "Getting label i: " << i << std::endl;
        data = redis_get_numid(r, LABEL_BASE + i, &len_labels);
        if (data == nullptr) {
            std::cout << "Label batch " << i << " not found" << std::endl;
            i--;
            continue;
        }
        auto label = cad_labels(data, len_labels);
        labels[i - left_id] = label;
        free(data);
    }
    std::cout << "get_data_samples done" << std::endl;
}

void LogisticTaskPreloaded::run(const Configuration& config, int worker) {
    std::cout << "[WORKER-PRELOADED] "
        << "Worker task id: " << worker
        << "connecting to store" << std::endl;
    
    lr_gradient_serializer lgs(MODEL_GRAD_SIZE);
    lr_model_deserializer lmd(MODEL_GRAD_SIZE);

#if defined(USE_REDIS)
    std::cout << "[WORKER-PRELOADED] "
        << "Worker task using REDIS" << std::endl;
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
    std::cout << "[WORKER-PRELOADED] "
        << "Error connecting to REDIS" << std::endl;
       throw std::runtime_error(
            "Error connecting to redis server. IP: " + std::string(REDIS_IP));
    }
#else
    std::cout << "USE_S3 not supported" << std::endl;
    exit(-1);
#endif

    uint64_t num_batches = config.get_num_samples() /
        config.get_minibatch_size();
    std::cout << "[WORKER-PRELOADED] "
        << "num_batches: " << num_batches << std::endl;
        
    std::vector<std::shared_ptr<double>> samples_preloaded;
    std::vector<std::shared_ptr<double>> labels_preloaded;

    wait_for_start(3 + worker, r, nworkers);
    
    uint64_t left_id = (worker_id - 4) * (num_batches / nworkers);
    uint64_t right_id = (worker_id - 4 + 1) * (num_batches / nworkers);
    std::cout << "[WORKER-PRELOADED] "
        << "preloading data.."
        << " left_id: " << left_id
        << " right_id: " << right_id
        << " num_batches: " << num_batches
        << " nworkers: " << nworkers
        << std::endl;
    std::cout << "[WORKER-PRELOADED] "
        << "preloading2 data.."
        <<std::endl;
    get_data_samples(r, left_id, right_id, samples_preloaded, labels_preloaded);
    std::cout << "[WORKER-PRELOADED] "
        << "preloaded data"
        << std::endl;

    bool first_time = true;

    uint64_t batch_id = SAMPLE_BASE + left_id;
    int gradient_id = GRADIENT_BASE + worker;
    uint64_t version = 0;
    while (1) {
        // maybe we can wait a few iterations to get the model
        LRModel model(MODEL_GRAD_SIZE);
        try {

            std::cout << "[WORKER] "
                << "Worker task getting the model at id: "
                << MODEL_BASE
                << "\n";

            auto before_model = get_time_ns();
            // get model
            int len;
            char* data = redis_get_numid(r, MODEL_BASE, &len);
            std::cout << "[WORKER] "
                << "Worker task got the model at id: "
                << MODEL_BASE
                << " data: " << reinterpret_cast<uint64_t>(data)
                << std::endl;
            if (data == nullptr) {
                throw cirrus::NoSuchIDException("");
            }
            std::cout << "[WORKER] "
                << "Worker task deserializing the model at id: "
                << MODEL_BASE
                << std::endl;
            model = lmd(data, len);
            std::cout << "[WORKER] "
                << "Worker task freeing the model at id: "
                << MODEL_BASE
                << std::endl;
            free(data);
            //-----------
            auto after_model = get_time_ns();
            std::cout << "[WORKER] "
                << "model get (ns): " << (after_model - before_model)
                << std::endl;


        } catch(const cirrus::NoSuchIDException& e) {
            if (!first_time) {
                std::cout << "[WORKER] "
                    << "Exiting"
                    << std::endl;
                exit(0);

                // wrap around
                batch_id = SAMPLE_BASE + left_id;
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

	std::cout << "[WORKER] "
		<< "Getting sample and label"
		<< std::endl;
        auto samples = samples_preloaded[batch_id - SAMPLE_BASE - left_id];
        auto labels = labels_preloaded[batch_id - SAMPLE_BASE - left_id];

        first_time = false;

        // Big hack. Shame on me
        // auto now = get_time_us();
        std::shared_ptr<double> l(new double[samples_per_batch],
                ml_array_nodelete<double>);
        std::copy(labels.get(), labels.get() + samples_per_batch, l.get());
        // std::cout << "Elapsed: " << get_time_us() - now << "\n";

	std::cout << "[WORKER] "
		<< "Building dataset"
		<< std::endl;
        Dataset dataset(samples.get(), l.get(),
                samples_per_batch, features_per_sample);

	auto now = get_time_us();
        // compute mini batch gradient
        std::unique_ptr<ModelGradient> gradient;
        try {
            gradient = model.minibatch_grad(0, dataset.samples_,
                    labels.get(), samples_per_batch, config.get_epsilon());
        } catch(...) {
            std::cout << "There was an error here" << std::endl;
            exit(-1);
        }
        auto elapsed_us = get_time_us() - now;
        std::cout << "[WORKER] "
            << "Gradient compute time (us): " << elapsed_us
            << " version " << version
            << "\n";
        gradient->setVersion(version++);

        try {
            auto lrg = dynamic_cast<LRGradient*>(gradient.get());
            if (lrg == nullptr) {
                throw std::runtime_error("Wrong dynamic cast");
            }
#ifdef USE_REDIS
	    auto data = std::unique_ptr<char[]>(
	        	    new char[lgs.size(*lrg)]);
            lgs.serialize(*lrg, data.get());
            redis_put_binary_numid(r, gradient_id, data.get(), lgs.size(*lrg));
#endif
        } catch(...) {
            std::cout << "[WORKER] "
                << "Worker task error doing put of gradient"
                << "\n";
            exit(-1);
        }
        std::cout << "[WORKER] "
            << "Worker task stored gradient at id: " << gradient_id
            << " at time (us): " << get_time_us()
            << "\n";

        // move to next batch of samples
        batch_id++;

        // Wrap around
        //if (samples_iter == s_iter.end()) {
        if (batch_id == SAMPLE_BASE + right_id) {
	    batch_id = SAMPLE_BASE + left_id;
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
        << "PS connecting to store"
        << std::endl;
    config.print();

    lr_model_serializer lms(MODEL_GRAD_SIZE);
    lr_model_deserializer lmd(MODEL_GRAD_SIZE);
    lr_gradient_serializer lgs(MODEL_GRAD_SIZE);
    lr_gradient_deserializer lgd(MODEL_GRAD_SIZE);

#ifdef USE_CIRRUS
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<LRModel>
        model_store(IP, PORT, &client,
            lms, lmd);
    cirrus::ostore::FullBladeObjectStoreTempl<LRGradient>
        gradient_store(IP, PORT, &client,
                lgs, lgd);
#elif defined(USE_REDIS)
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
       throw std::runtime_error(
            "Error connecting to redis server. IP: " + std::string(REDIS_IP));
    }
#endif

    std::cout << "[PS] "
        << "PS task initializing model" << std::endl;
    // initialize model

    LRModel model(MODEL_GRAD_SIZE);
    model.randomize();
    std::cout << "[PS] "
        << "PS publishing model at id: "
        << MODEL_BASE
        << " csum: " << model.checksum()
        << std::endl;
    // publish the model back to the store so workers can use it
#ifdef USE_CIRRUS
    model_store.put(MODEL_BASE, model);
#elif defined(USE_REDIS)
    {
       auto data = std::unique_ptr<char[]>(
       	    new char[lms.size(model)]);
       lms.serialize(model, data.get());
       redis_put_binary_numid(r, MODEL_BASE, data.get(), lms.size(model));
    }
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
        << std::endl;
#endif

    // we keep a version number for the gradient produced by each worker
    std::vector<unsigned int> gradientVersions;
    gradientVersions.resize(nworkers, 0);

    bool first_time = true;

#ifdef USE_CIRRUS    
    wait_for_start(2, client, nworkers);
#elif defined(USE_REDIS)
    wait_for_start(2, r, nworkers);
#endif

    while (1) {
        // for every worker, check for a new gradient computed
        // if there is a new gradient, get it and update the model
        // once model is updated publish it
        for (int worker = 0; worker < static_cast<int>(nworkers); ++worker) {
            int gradient_id = GRADIENT_BASE + worker;
#ifdef DEBUG
            std::cout << "[PS] "
                << "PS task checking gradient id: "
                << gradient_id
                << std::endl;
#endif

            // get gradient from store
            LRGradient gradient(MODEL_GRAD_SIZE);
            try {
#ifdef USE_CIRRUS
                gradient = std::move(
                        gradient_store.get(gradient_id));
#elif defined(USE_REDIS)
		int len_grad;
		char* data = redis_get_numid(r, gradient_id, &len_grad);
                if (data == nullptr) {
                  throw cirrus::NoSuchIDException("");
                }
		gradient = lgd(data, len_grad);
		free(data);
#endif
            } catch(const cirrus::NoSuchIDException& e) {
                if (!first_time) {
                    std::cout
                        << "PS task not able to get gradient: "
                        << std::to_string(gradient_id)
                        << " gradient_id: " << gradient_id
                        << std::endl;
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
#ifdef DEBUG
                std::cout << "[PS] "
                    << "PS task received new gradient version: "
                    << gradient.getVersion()
                    << std::endl;
#endif

                // if it's new
                gradientVersions[worker] = gradient.getVersion();

                // do a gradient step and update model
#ifdef DEBUG
                std::cout << "[PS] "
                    << "Updating model"
                    << " with csum: " << model.checksum()
                    << std::endl;
#endif

                model.sgd_update(config.get_learning_rate(), &gradient);

                std::cout << "[PS] "
                    << "Publishing model at: " << get_time_us()
                    << "\n";
                // publish the model back to the store so workers can use it
#ifdef USE_CIRRUS
                model_store.put(MODEL_BASE, model);
#elif defined(USE_REDIS)
                uint64_t data_size = lms.size(model);
                auto data = std::unique_ptr<char[]>(
	            	    new char[data_size]);
		lms.serialize(model, data.get());
		redis_put_binary_numid(r, MODEL_BASE, data.get(), data_size);
#endif
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
    
    wait_for_start(1, client, nworkers);
#elif defined(USE_REDIS)
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
       throw std::runtime_error(
            "Error connecting to redis server. IP: " + std::string(REDIS_IP));
    }
    
    wait_for_start(1, r, nworkers);
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
                    << i << " with csmus: "
                    << checksum(samples, batch_size) << " "
                    << checksum(labels, samples_per_batch)
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

/**
  * Load the object store with the training dataset
  * It reads from the criteo dataset files and writes to the object store
  * It signals when work is done by changing a bit in the object store
  */
void LoadingTask::run(const Configuration& config) {
    std::cout << "[LOADER] "
        << "Read criteo input..."
        << std::endl;

    Input input;

    //auto dataset = input.read_input_csv(
    //        config.get_input_path(),
    //        " ", 3,
    //        config.get_limit_cols(), false);  // data is already normalized
    bool normalize = config.get_normalize();
    auto dataset = input.read_input_csv(
            config.get_input_path(),
            " ", 10,
            config.get_limit_cols(), normalize);

    dataset.check_values();
#ifdef DEBUG
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
    auto r  = redis_connect(REDIS_IP, REDIS_PORT);
    if (r == NULL || r -> err) { 
       throw std::runtime_error(
            "Error connecting to redis server. IP: " + std::string(REDIS_IP));
    }
#endif

    std::cout << "[LOADER] "
        << "Adding "
        << dataset.num_samples()
        << " samples in batches of size (samples*features): "
        << batch_size
        << std::endl;

    // We put in batches of N samples
    for (unsigned int i = 0;
            i < dataset.num_samples() / samples_per_batch; ++i) {
        std::cout << "[LOADER] "
            << "Building samples batch. Batch size: " << batch_size << std::endl;
        /** Build sample object
          */
        auto sample = std::shared_ptr<double>(
                new double[batch_size],
                std::default_delete<double[]>());
        // this memcpy can be avoided with some trickery
        std::cout << "[LOADER] "
            << "Copying samples batch" << std::endl;
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
#ifdef USE_CIRRUS
            samples_store.put(SAMPLE_BASE + i, sample);
#elif defined(USE_REDIS)
            uint64_t len = cas_samples.size(sample);
	    {
               auto data = std::unique_ptr<char[]>(
	           	    new char[len]);
	       cas_samples.serialize(sample, data.get());
	       redis_put_binary_numid(r, SAMPLE_BASE + i, data.get(), cas_samples.size(sample));
            }
#endif
#ifdef USE_CIRRUS
            std::cout << "[LOADER] "
               << "Putting label"
               << std::endl;
            labels_store.put(LABEL_BASE + i, label);
#elif defined(USE_REDIS)
            std::cout << "[LOADER] "
               << "Storing labels on REDIS with size: "
               << cas_labels.size(label)
               << std::endl;
            {
	       auto data = std::unique_ptr<char>(
	           	    new char[cas_labels.size(label)]);
	       cas_labels.serialize(label, data.get());
	       redis_put_binary_numid(r, LABEL_BASE + i, data.get(), cas_labels.size(label));
            }
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
        << "Added all samples"
        << std::endl;

#ifdef DEBUG
    std::cout << "[LOADER] "
        << "Print sample 0"
        << std::endl;

    for (unsigned int i = 0; i < batch_size; ++i) {
        double val = sample.get()[i];
        std::cout << val << " ";
    }
#endif
    std::cout << std::endl;
   
#ifdef USE_CIRRUS    
    wait_for_start(0, client, nworkers);
#elif defined(USE_REDIS)
    wait_for_start(0, r, nworkers);
#endif
}

