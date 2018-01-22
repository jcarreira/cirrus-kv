#ifndef EXAMPLES_ML_MODELGRADIENT_H_
#define EXAMPLES_ML_MODELGRADIENT_H_

#include <cstdint>
#include <vector>
#include <config.h>

/**
  * This class is a base class for a model's gradient
  */
class ModelGradient {
 public:
     ModelGradient() : version(0) {}
     virtual ~ModelGradient() = default;

     /**
       * Serialize gradient
       * @param mem Pointer where to serialize the gradient
       */
     virtual void serialize(void* mem) const = 0;

     /**
       * Get the size of the gradient when serialized
       * @returns Size of serialized gradient
       */
     virtual uint64_t getSerializedSize() const = 0;

     /**
       * Load gradient from serialized memory
       * @param mem Pointer to memory where the gradient lives serialized
       */
     virtual void loadSerialized(const void* mem) = 0;

     /**
       * Print gradient
       */
     virtual void print() const = 0;

     /**
       * Set gradient version
       */
     virtual void setVersion(uint64_t v) {
         version = v;
     }

     /**
       * Get version of gradient
       */
     virtual uint64_t getVersion() const {
         return version;
     }

     /**
       * Sanity check gradient values
       */
     virtual void check_values() const = 0;

 protected:
     uint64_t version = 0;  //< this gradient's version
};

class LRGradient : public ModelGradient {
 public:
    friend class LRModel;
    friend class SparseLRModel;

    virtual ~LRGradient() = default;

    LRGradient(LRGradient&& data);
    explicit LRGradient(const std::vector<FEATURE_TYPE>& data);
    explicit LRGradient(int d);

    LRGradient& operator=(LRGradient&& other);

    void loadSerialized(const void*);
    void serialize(void*) const override;
    uint64_t getSerializedSize() const override;

    void print() const override;
    void check_values() const override;
 protected:
    std::vector<FEATURE_TYPE> weights;  //< weights of the LR gradient
};

class LRSparseGradient : public ModelGradient {
 public:
    friend class LRModel;
    friend class SparseLRModel;
    friend class SparseLRModel;

    virtual ~LRSparseGradient() = default;

    LRSparseGradient(LRSparseGradient&& data);
    explicit LRSparseGradient(const std::vector<std::pair<int, FEATURE_TYPE>>& data);
    explicit LRSparseGradient(int d);

    LRSparseGradient& operator=(LRSparseGradient&& other);

    void loadSerialized(const void*);
    void serialize(void*) const override;
    uint64_t getSerializedSize() const override;

    void print() const override;
    void check_values() const override;
 protected:
    std::vector<std::pair<int, FEATURE_TYPE>> weights;  //< weights
};

class SoftmaxGradient : public ModelGradient {
 public:
    friend class SoftmaxModel;

    virtual ~SoftmaxGradient() = default;

    SoftmaxGradient(uint64_t nclasses, uint64_t d);
    explicit SoftmaxGradient(const std::vector<std::vector<FEATURE_TYPE>>&);

    void loadSerialized(const void*);
    void serialize(void*) const override;
    uint64_t getSerializedSize() const override;

    void print() const override;
    void check_values() const override;
 protected:
    // [D * K]
    std::vector<std::vector<FEATURE_TYPE>> weights;  //< weights for softmax gradient
};

class MFGradient : public ModelGradient {
 public:
    friend class MFModel;

    virtual ~MFGradient() = default;

    MFGradient(uint64_t nsamples, uint64_t nfactors);
    explicit MFGradient(const std::vector<std::vector<FEATURE_TYPE>>&);

    void loadSerialized(const void*);
    void serialize(void*) const override;
    uint64_t getSerializedSize() const override;

    void print() const override;
    void check_values() const override;
 protected:
    // [D * K]
    std::vector<std::vector<FEATURE_TYPE>> weights;
};

#endif  // EXAMPLES_ML_MODELGRADIENT_H_
