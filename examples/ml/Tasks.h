#ifndef _TASKS_H_
#define _TASKS_H_

#include <Configuration.h>
#include <string>

class LogisticTask {
 public:
     LogisticTask(const std::string& IP, const std::string& PORT,
             uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
             uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
             uint64_t SAMPLE_BASE, uint64_t batch_size,
             uint64_t samples_per_batch, uint64_t features_per_sample,
             uint64_t nworkers) :
         IP(IP), PORT(PORT), MODEL_GRAD_SIZE(MODEL_GRAD_SIZE),
         MODEL_BASE(MODEL_BASE), LABEL_BASE(LABEL_BASE),
         GRADIENT_BASE(GRADIENT_BASE), SAMPLE_BASE(SAMPLE_BASE),
         batch_size(batch_size), samples_per_batch(samples_per_batch),
         features_per_sample(features_per_sample), nworkers(nworkers)
    {}

     /**
       * Worker here is a value 0..nworkers - 1
       */
     void run(const Configuration& config, int worker);

 private:
     std::string IP;
     std::string PORT;
     uint64_t MODEL_GRAD_SIZE;
     uint64_t MODEL_BASE;
     uint64_t LABEL_BASE;
     uint64_t GRADIENT_BASE;
     uint64_t SAMPLE_BASE;
     uint64_t batch_size;
     uint64_t samples_per_batch;
     uint64_t features_per_sample;
     uint64_t nworkers;
};

class PSTask {
 public:
     PSTask(const std::string& IP, const std::string& PORT,
             uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
             uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
             uint64_t SAMPLE_BASE, uint64_t batch_size,
             uint64_t samples_per_batch, uint64_t features_per_sample,
             uint64_t nworkers) :
         IP(IP), PORT(PORT), MODEL_GRAD_SIZE(MODEL_GRAD_SIZE),
         MODEL_BASE(MODEL_BASE), LABEL_BASE(LABEL_BASE),
         GRADIENT_BASE(GRADIENT_BASE), SAMPLE_BASE(SAMPLE_BASE),
         batch_size(batch_size), samples_per_batch(samples_per_batch),
         features_per_sample(features_per_sample), nworkers(nworkers)
    {}
     void run(const Configuration& config);

 private:
     std::string IP;
     std::string PORT;
     uint64_t MODEL_GRAD_SIZE;
     uint64_t MODEL_BASE;
     uint64_t LABEL_BASE;
     uint64_t GRADIENT_BASE;
     uint64_t SAMPLE_BASE;
     uint64_t batch_size;
     uint64_t samples_per_batch;
     uint64_t features_per_sample;
     uint64_t nworkers;
};

class ErrorTask {
 public:
     ErrorTask(const std::string& IP, const std::string& PORT,
             uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
             uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
             uint64_t SAMPLE_BASE, uint64_t batch_size,
             uint64_t samples_per_batch, uint64_t features_per_sample,
             uint64_t nworkers) :
         IP(IP), PORT(PORT), MODEL_GRAD_SIZE(MODEL_GRAD_SIZE),
         MODEL_BASE(MODEL_BASE), LABEL_BASE(LABEL_BASE),
         GRADIENT_BASE(GRADIENT_BASE), SAMPLE_BASE(SAMPLE_BASE),
         batch_size(batch_size), samples_per_batch(samples_per_batch),
         features_per_sample(features_per_sample), nworkers(nworkers)
    {}
     void run(const Configuration& config);

 private:
     std::string IP;
     std::string PORT;
     uint64_t MODEL_GRAD_SIZE;
     uint64_t MODEL_BASE;
     uint64_t LABEL_BASE;
     uint64_t GRADIENT_BASE;
     uint64_t SAMPLE_BASE;
     uint64_t batch_size;
     uint64_t samples_per_batch;
     uint64_t features_per_sample;
     uint64_t nworkers;
};

class LoadingTask {
 public:
     LoadingTask(const std::string& IP, const std::string& PORT,
             uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
             uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
             uint64_t SAMPLE_BASE, uint64_t batch_size,
             uint64_t samples_per_batch, uint64_t features_per_sample,
             uint64_t nworkers) :
         IP(IP), PORT(PORT), MODEL_GRAD_SIZE(MODEL_GRAD_SIZE),
         MODEL_BASE(MODEL_BASE), LABEL_BASE(LABEL_BASE),
         GRADIENT_BASE(GRADIENT_BASE), SAMPLE_BASE(SAMPLE_BASE),
         batch_size(batch_size), samples_per_batch(samples_per_batch),
         features_per_sample(features_per_sample), nworkers(nworkers)
    {}
     void run(const Configuration& config);

 private:
     std::string IP;
     std::string PORT;
     uint64_t MODEL_GRAD_SIZE;
     uint64_t MODEL_BASE;
     uint64_t LABEL_BASE;
     uint64_t GRADIENT_BASE;
     uint64_t SAMPLE_BASE;
     uint64_t batch_size;
     uint64_t samples_per_batch;
     uint64_t features_per_sample;
     uint64_t nworkers;
};

#endif  // _TASKS_H_
