#ifndef _SERIALIZERS_H_
#define _SERIALIZERS_H_

#include <string>
#include <memory>
#include <utility>
#include <LRModel.h>

template<typename T>
class ml_array_deleter {
 public:
    ml_array_deleter(const std::string& name) :
        name(name) {}

    void operator()(T* p) {
        //std::cout << "Deleting: " << name << std::endl;
        delete[] p;
    }
 private:
    std::string name;
};

template<typename T>
void ml_array_nodelete(T * p) {}

template<typename T>
class c_array_serializer{
 public:
    c_array_serializer(int nslots, const std::string& name = "") :
        numslots(nslots), name(name) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const std::shared_ptr<T>& v) {
        //std::cout << "[ARRAYSER-" + name + "]"
        //    << " c_array_serializer numslots: " << numslots
        //    << std::endl;
        
        T* array = v.get();

        // allocate array
        std::unique_ptr<char[]> ptr(new char[numslots * sizeof(T)]);

        // copy samples to array
        memcpy(ptr.get(), array, numslots * sizeof(T));
        return std::make_pair(std::move(ptr), numslots * sizeof(T));
    }

 private:
    int numslots;
    std::string name;
}; 

template<typename T>
class c_array_deserializer{
 public:
    c_array_deserializer(
            int nslots, const std::string& name = "", bool to_free = true) :
        numslots(nslots), name(name), to_free(to_free) {}
    
    std::shared_ptr<T>
    operator()(void* data, unsigned int des_size) {
        unsigned int size = sizeof(T) * numslots;
        
        //std::cout << "[ARRAYDESER-" + name + "]"
        //    << " c_array_deserializer des_size: " << des_size
        //    << std::endl;

        if (des_size != size) {
            //std::cout << "Error in deserializing."
            //   << " Expecting size: " << size
            //   << " got des_size: " << des_size
            //   << std::endl;
            throw std::runtime_error(
                    "Wrong deserializer size at c_array_deserializer");
        }

        // cast the pointer
        T* ptr = reinterpret_cast<T*>(data);

        std::shared_ptr<T> ret_ptr;
        if (to_free) {
            ret_ptr = std::shared_ptr<T>(new T[numslots],
                    ml_array_deleter<T>(name));
            //std::default_delete<T[]>());
        } else {
            ret_ptr = std::shared_ptr<T>(new T[numslots],
                    ml_array_nodelete<T>);
        }

        std::memcpy(ret_ptr.get(), ptr, size);
        return ret_ptr;
    }

 private:
    int numslots;
    std::string name;
    bool to_free;
};


/**
  * LRModel serializer / deserializer
  */
class lr_model_serializer {
 public:
    lr_model_serializer(int n, const std::string& name = "") :
        n(n), name(name) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const LRModel& v);

 private:
    int n;
    std::string name;
}; 

class lr_model_deserializer {
 public:
    lr_model_deserializer(uint64_t n, const std::string& name = "") :
        n(n), name(name) {}
    
    LRModel
    operator()(void* data, unsigned int des_size);

 private:
    uint64_t n;
    std::string name;
};

/**
  * LRGradient serializer / deserializer
  */
class lr_gradient_serializer {
 public:
    lr_gradient_serializer(int n) : n(n) {}

    std::pair<std::unique_ptr<char[]>, unsigned int>
    operator()(const LRGradient& g);

 private:
    int n;
}; 

class lr_gradient_deserializer {
 public:
    lr_gradient_deserializer(uint64_t n) : n(n) {}
    
    LRGradient
    operator()(void* data, unsigned int des_size);

 private:
    uint64_t n;
};

#endif  // _SERIALIZERS_H_
