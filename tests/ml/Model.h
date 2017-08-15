#ifndef _MODEL_H_
#define _MODEL_H_

#include <utility>
#include <memory>
#include <vector>
#include <ModelGradient.h>
#include <Dataset.h>

/**
  * Base class is used to represent a model
  * Models can have different sizes and need to be serializable
  * to send over the network
  */
class Model {
 public:
     /**
       * Serializes model into memory
       * @return Pointer to serialized model and size of serialized model
       */
     virtual std::pair<std::unique_ptr<char[]>, uint64_t> serialize() const = 0;

     /**
       * Create new model instance from serialized model
       * @param data Pointer to serialized model
       * @param size Size of model when serialized
       */
     virtual std::unique_ptr<Model> deserialize(void* data,
             uint64_t size) const = 0;

     /**
       * Perform an SGD update on the model
       * @param learning_rate Learning rate
       * @param gradient Model gradient
       */
     virtual void sgd_update(double learning_rate,
             const ModelGradient* gradient) = 0;

     /**
       * Loads the weights from a serialized model
       * @param mem Pointer to memory where the model is serialized
       */
     virtual void loadSerialized(const void* mem) = 0;

     /**
       * Randomly initialize the model
       */
     virtual void randomize() = 0;

     /**
       * Make a copy of the model
       * @return Copy of model
       */
     virtual std::unique_ptr<Model> copy() const = 0;

     /**
       * Get size of model when serialized
       * @return Size of serialized model
       */
     virtual uint64_t getSerializedSize() const = 0;

     /**
       * Get size (bytes) of gradient when serialized
       * @return Size of serialized gradient
       */
     virtual uint64_t getSerializedGradientSize() const = 0;

     /**
       * Creates new gradient instance from serialized gradient
       * @param mem Pointer to memory where gradient is serialized
       * @return New instance of gradient
       */
     virtual std::unique_ptr<ModelGradient> loadGradient(void* mem) const = 0;

     /**
       * Calculate loss when applying model to a dataset
       * @param dataset Dataset to be used to calculate loss
       * @return Total loss when applying model to dataset
       */
     virtual double calc_loss(Dataset& dataset) const = 0;

     /**
       * Compute gradient for minibatch
       * @param rank MPI worker rank
       * @param dataset Dataset to use to compute the gradient
       * @param labels Correspoding sample labels
       * @param epsilon L2 regularization rate
       * @returns SGD gradient
       */
     virtual std::unique_ptr<ModelGradient> minibatch_grad(
                int rank, const Matrix& dataset,
                double* labels,
                uint64_t labels_size,
                double epsilon) const = 0;

     virtual double checksum() const = 0;
};

#endif  // _MODEL_H_
