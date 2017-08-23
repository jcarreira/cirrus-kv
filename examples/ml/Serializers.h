#ifndef _SERIALIZERS_H_
#define _SERIALIZERS_H_

#include <string>
#include <memory>
#include <utility>
#include <arpa/inet.h>
#include <LRModel.h>

#include "common/Serializer.h"

/**
  * Deleter used to free arrays
  */
template<typename T>
class ml_array_deleter {
 public:
    explicit ml_array_deleter(const std::string& name) :
        name(name) {}

    void operator()(T* p) {
        delete[] p;
    }
 private:
    std::string name;  //< name associated with this deleter
};

/**
  * Deleter used to NOT free arrays
  */
template<typename T>
void ml_array_nodelete(T * /* p */) {}

/**
  * Serializer for raw arrays
  */
template<typename T>
class c_array_serializer : public cirrus::Serializer<std::shared_ptr<T>> {
 public:
    explicit c_array_serializer(int nslots, const std::string& name = "") :
        numslots(nslots), name(name) {}

    uint64_t size(const std::shared_ptr<T>& /*obj*/) const override {
        uint64_t size = numslots * sizeof(T);
        return size;
    }

    void serialize(const std::shared_ptr<T>& obj, void* mem) const override {
        T* array = obj.get();

        // copy samples to array
        memcpy(mem, array, size(obj));
    }
 private:
    int numslots;      //< number of entries in the array
    std::string name;  //< name associated with this serializer
};

/**
  * Deserializer for raw arrays
  */
template<typename T>
class c_array_deserializer{
 public:
    c_array_deserializer(
            int nslots, const std::string& name = "", bool to_free = true) :
        numslots(nslots), name(name), to_free(to_free) {}

    std::shared_ptr<T>
    operator()(const void* data, unsigned int des_size) {
        unsigned int size = sizeof(T) * numslots;

        if (des_size != size) {
            throw std::runtime_error(
                 std::string("Wrong size received at c_array_deserializer)")
                    + " size: " + std::to_string(des_size)
                    + " expected: " + std::to_string(size)
                    + " name: " + name);
        }

        // cast the pointer
        const T* ptr = reinterpret_cast<const T*>(data);

        std::shared_ptr<T> ret_ptr;
        if (to_free) {
            ret_ptr = std::shared_ptr<T>(new T[numslots],
                    ml_array_deleter<T>(name));
        } else {
            ret_ptr = std::shared_ptr<T>(new T[numslots],
                    ml_array_nodelete<T>);
        }

        std::memcpy(ret_ptr.get(), ptr, size);
        return ret_ptr;
    }

 private:
    int numslots;      //< number of slots in input arrays
    std::string name;  //< name associated with this deserializer
    bool to_free;      //< whether memory passed be reference counted
};


/**
  * LRModel serializer
  */
class lr_model_serializer : public cirrus::Serializer<LRModel> {
 public:
    explicit lr_model_serializer(uint64_t n, const std::string& name = "") :
        n(n), name(name) {}

    uint64_t size(const LRModel& model) const override;
    void serialize(const LRModel& model, void* mem) const override;

 private:
    uint64_t n;             //< size of the model
    std::string name;  //< name associated with this serializer
};

/**
  * LRModel deserializer
  */
class lr_model_deserializer {
 public:
    explicit lr_model_deserializer(uint64_t n, const std::string& name = "") :
        n(n), name(name) {}

    LRModel
    operator()(const void* data, unsigned int des_size);

 private:
    uint64_t n;             //< size of the model
    std::string name;  //< name associated with this serializer
};

/**
  * LRModel serializer
  */
class lr_gradient_serializer : public cirrus::Serializer<LRGradient> {
 public:
    explicit lr_gradient_serializer(uint64_t n, const std::string& name = "") :
        n(n), name(name) {}

    uint64_t size(const LRGradient& g) const override;
    void serialize(const LRGradient& g, void* mem) const override;

 private:
    uint64_t n;             //< size of the model
    std::string name;  //< name associated with this serializer
};

/**
  * LRModel deserializer
  */
class lr_gradient_deserializer {
 public:
    explicit lr_gradient_deserializer(uint64_t n) : n(n) {}

    LRGradient
    operator()(const void* data, unsigned int des_size);

 private:
    uint64_t n;  //< size of the gradient
};

#endif  // _SERIALIZERS_H_
