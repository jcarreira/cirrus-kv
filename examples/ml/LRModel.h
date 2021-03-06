#ifndef EXAMPLES_ML_LRMODEL_H_
#define EXAMPLES_ML_LRMODEL_H_

#include <vector>
#include <utility>
#include <Model.h>
#include <Matrix.h>
#include <Dataset.h>
#include <ModelGradient.h>

/**
  * Logistic regression model
  * Model is represented with a vector of doubles
  */
class LRModel : public Model {
 public:
    /**
      * LRModel constructor
      * @param d Features dimension
      */
    explicit LRModel(uint64_t d);

    /**
      * LRModel constructor from weight vector
      * @param w Array of model weights
      * @param d Features dimension
      */
    LRModel(const double* w, uint64_t d);

    /**
     * Set the model weights to values between 0 and 1
     */
    void randomize() override;

    /**
     * Loads model weights from serialized memory
     * @param mem Memory where model is serialized
     */
    void loadSerialized(const void* mem) override;

    /**
      * serializes this model into memory
      * @return pair of memory pointer and size of serialized model
      */
    std::pair<std::unique_ptr<char[]>, uint64_t>
        serialize() const override;

    /**
      * serializes this model into memory pointed by mem
      */
    void serializeTo(void* mem) const;

    /**
     * Create new model from serialized weights
     * @param data Memory where the serialized model lives
     * @param size Size of the serialized model
     */
    std::unique_ptr<Model> deserialize(void* data,
            uint64_t size) const override;

    /**
     * Performs a deep copy of this model
     * @return New model
     */
    std::unique_ptr<Model> copy() const override;

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
    uint64_t getSerializedSize() const override;

    /**
     * Compute a minibatch gradient
     * @param rank MPI worker rank
     * @param dataset Dataset to learn on
     * @param labels Labels of the samples
     * @param labels_size Size of the labels array
     * @param epsilon L2 Regularization rate
     * @return Newly computed gradient
     */
    std::unique_ptr<ModelGradient> minibatch_grad(
            int rank, const Matrix& dataset,
            double* labels,
            uint64_t labels_size,
            double epsilon) const override;
    /**
     * Compute the logistic loss of a given dataset on the current model
     * @param dataset Dataset to calculate loss on
     * @return Total loss of whole dataset
     */
    double calc_loss(Dataset& dataset) const override;

    /**
     * Return the size of the gradient when serialized
     * @return Size of gradient when serialized
     */
    uint64_t getSerializedGradientSize() const override;

    /**
      * Builds a gradient that is stored serialized
      * @param mem Memory address where the gradient is serialized
      * @return Pointer to new gradient object
      */
    std::unique_ptr<ModelGradient> loadGradient(void* mem) const override;

    /**
      * Compute checksum of the model
      * @return Checksum of the model
      */
    double checksum() const override;

    /**
      * Print the model's weights
      */
    void print() const;

 private:
    /**
      * Check whether value n is an integer
      */
    bool is_integer(double n) const;

    std::vector<double> weights;  //< vector of the model weights
    uint64_t d;                   //< size of the model
};

#endif  // EXAMPLES_ML_LRMODEL_H_
