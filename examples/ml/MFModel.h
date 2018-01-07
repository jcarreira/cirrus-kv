#ifndef EXAMPLES_ML_MFMODEL_H_
#define EXAMPLES_ML_MFMODEL_H_

#include <vector>
#include <utility>
#include <Model.h>
#include <Matrix.h>
#include <Dataset.h>
#include <ModelGradient.h>
#include <SparseDataset.h>

/**
  * Matrix Factorization model
  * Model is represented with a vector of doubles
  */

// XXX Should derive from Model
class MFModel {
 public:
    /**
      * MFModel constructor from weight vector
      * @param w Array of model weights
      * @param d Features dimension
      */
    MFModel(const void* w, bool is_sparse, uint64_t n, uint64_t d);
    MFModel(uint64_t n, uint64_t d);
    
    explicit MFModel(const SparseDataset&);

    /**
     * Set the model weights to values between 0 and 1
     */
    void randomize();

    /**
     * Loads model weights from serialized memory
     * @param mem Memory where model is serialized
     */
    void loadSerialized(const void* mem);

    /**
      * serializes this model into memory
      * @return pair of memory pointer and size of serialized model
      */
    std::pair<std::unique_ptr<char[]>, uint64_t>
        serialize() const;

    /**
      * serializes this model into memory pointed by mem
      */
    void serializeTo(void* mem, bool is_sparse) const;

    /**
     * Create new model from serialized weights
     * @param data Memory where the serialized model lives
     * @param size Size of the serialized model
     */
    std::unique_ptr<Model> deserialize(void* data,
            uint64_t size) const;

    /**
     * Performs a deep copy of this model
     * @return New model
     */
    std::unique_ptr<Model> copy() const;

    /**
     * Performs an SGD update in the direction of the input gradient
     * @param learning_rate Learning rate to be used
     * @param gradient Gradient to be used for the update
     */
    void sgd_update(double learning_rate, const ModelGradient* gradient);

    /**
     * Returns the size of the model weights serialized
     * @returns Size of the model when serialized
     */
    uint64_t getSerializedSize() const;

    /**
     * Compute a minibatch gradient
     * @param dataset Dataset to learn on
     * @param epsilon L2 Regularization rate
     * @return Newly computed gradient
     */
    std::unique_ptr<ModelGradient> minibatch_grad(
            const SparseDataset& dataset,
            double epsilon) const;
    /**
     * Compute the logistic loss of a given dataset on the current model
     * @param dataset Dataset to calculate loss on
     * @return Total loss of whole dataset
     */
    double calc_loss(Dataset& dataset) const;

    /**
     * Return the size of the gradient when serialized
     * @return Size of gradient when serialized
     */
    uint64_t getSerializedGradientSize() const;

    /**
      * Builds a gradient that is stored serialized
      * @param mem Memory address where the gradient is serialized
      * @return Pointer to new gradient object
      */
    std::unique_ptr<ModelGradient> loadGradient(void* mem) const;

    /**
      * Compute checksum of the model
      * @return Checksum of the model
      */
    double checksum() const;

    /**
      * Print the model's weights
      */
    void print() const;

    /**
      * Return model size (should match sample size)
      * @return Size of the model
      */
    uint64_t size() const;

 private:
    double* weights_;
    uint64_t n_;
    uint64_t d_;
};

#endif  // EXAMPLES_ML_MFMODEL_H_
