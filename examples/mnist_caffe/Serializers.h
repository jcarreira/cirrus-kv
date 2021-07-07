#ifndef _SERIALIZERS_H_
#define _SERIALIZERS_H_

/** Format:
  * rows (uint64_t)
  * cols (uint64_t)
  * type (uint64_t)
  * data size (uint64_t)
  * raw data
  */
class image_serializer : public cirrus::Serializer<cv::Mat> {
 public:
    uint64_t size(const cv::Mat& img) const override {
        uint64_t data_size = img.total() * img.elemSize();
        uint64_t total_size = sizeof(uint64_t) * 4 + data_size;
        return total_size;
    }   

    void serialize(const cv::Mat& img, void* mem) const override {
        uint64_t* ptr = reinterpret_cast<uint64_t*>(mem);
        *ptr++ = img.rows;
        *ptr++ = img.cols;
        *ptr++ = img.type();

        uint64_t data_size = img.total() * img.elemSize();
        *ptr++ = data_size;

        std::cout << "Serializing."
            << " rows: " << img.rows
            << " cols: " << img.cols
            << " type: " << img.type()
            << " data_size: " << data_size
            << std::endl;

        memcpy(ptr, img.data, data_size);
    }   
 private:
};

class image_deserializer {
 public:
    cv::Mat operator()(const void* data, unsigned int des_size) {
        const uint64_t* ptr = reinterpret_cast<const uint64_t*>(data);
        uint64_t rows = *ptr++;
        uint64_t cols = *ptr++;
        uint64_t type = *ptr++;
        uint64_t data_size = *ptr++;

        if (des_size != data_size + sizeof(double) * 4) {
            throw std::runtime_error("Wrong size");
        }

        void* m = new char[data_size];
        std::memcpy(m, ptr, data_size);
        
        std::cout << "Deserializing."
            << " rows: " << rows
            << " cols: " << cols
            << " type: " << type
            << " data_size: " << data_size
            << std::endl;

        return cv::Mat(rows, cols, type, m);
    }

 private:
};

template<typename T>
class array_deleter {
 public:
    explicit array_deleter(const std::string& name = "") :
        name(name) {}

    void operator()(T* p) {
        delete[] p;
    }   
 private:
    std::string name;  //< name associated with this deleter
};

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

template<typename T>
class c_array_deserializer {
 public:
    c_array_deserializer(
            int nslots, const std::string& name = "") :
        numslots(nslots), name(name) {}

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
        ret_ptr = std::shared_ptr<T>(new T[numslots],
                array_deleter<T>(name));

        std::memcpy(ret_ptr.get(), ptr, size);
        return ret_ptr;
    }

 private:
    int numslots;      //< number of slots in input arrays
    std::string name;  //< name associated with this deserializer
};

typedef std::shared_ptr<std::vector<cv::Mat>> image_vector_ptr;

/** # of images - n (uint64_t)
  * Image 0:
  * rows (uint64_t)
  * cols (uint64_t)
  * type (unsigned)
  * data size (uint64_t)
  * image data (of data size)
  * Image 1:
  * ....
  * ....
  * Image n-1:
  * ....
  * ....
  */
class image_vector_serializer :
    public cirrus::Serializer<image_vector_ptr> {
 public:
    uint64_t size(const image_vector_ptr& vec) const override {
        uint64_t total_size = 0;

        for (int i = 0; i < vec->size(); ++i) {
            cv::Mat& img = vec->operator[](i);
            uint64_t img_data_size = img.total() * img.elemSize();
            total_size += img_data_size + sizeof(uint64_t) * 4;
        }

        return total_size + sizeof(uint64_t);
    }

    void serialize(const image_vector_ptr& vec, void* mem) const override {
        uint64_t* ptr = reinterpret_cast<uint64_t*>(mem);
        *ptr++ = vec->size();

        for (uint64_t i = 0; i < vec->size(); ++i) {
            cv::Mat& img = vec->operator[](i);
            *ptr++ = img.rows;
            *ptr++ = img.cols;
            *ptr++ = img.type();
            
            uint64_t data_size = img.total() * img.elemSize();
            *ptr++ = data_size;
            memcpy(ptr, img.data, data_size);

            ptr = reinterpret_cast<uint64_t*>(
                    reinterpret_cast<char*>(ptr) + data_size);
        }

        if (std::distance((char*)mem, (char*)ptr) != size(vec)) {
            throw std::runtime_error("Wrong size in image_vector_serializer");
        }

        std::cout << "Serializing image vector."
            << " size: " << vec->size()
            << " raw size: " << size(vec)
            << std::endl;
    }
 private:
};

class image_vector_deserializer {
 public:
    image_vector_deserializer(const std::string& name = "") :
        name(name) {}

    image_vector_ptr
    operator()(const void* data, unsigned int des_size) {

        const uint64_t* ptr = reinterpret_cast<const uint64_t*>(data);
        unsigned int n_images = *ptr++;

        std::cout << "Deserializing image vector with size: "
            << n_images
            << " des_size: " << des_size
            << std::endl;

        auto res = std::make_shared<std::vector<cv::Mat>>();

        for (int i = 0; i < n_images; ++i) {
            uint64_t rows = *ptr++;
            uint64_t cols = *ptr++;
            uint64_t type = *ptr++;
            uint64_t data_size = *ptr++;
            
            // XXX make sure this memory is freed later
            void* m = new char[data_size];
            std::memcpy(m, ptr, data_size);

            res->push_back(cv::Mat(rows, cols, type, m));
            
            ptr = reinterpret_cast<const uint64_t*>(
                    reinterpret_cast<const char*>(ptr) + data_size);
        }

        if (std::distance((char*)data, (char*)ptr) != des_size) {
            throw std::runtime_error("Wrong size in image_vector_deserializer");
        }

        return res;
    }

 private:
    std::string name;  //< name associated with this deserializer
};

#endif  // _SERIALIZERS_H_
