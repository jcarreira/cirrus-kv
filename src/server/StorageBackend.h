#ifndef SRC_SERVER_STORAGEBACKEND_H_
#define SRC_SERVER_STORAGEBACKEND_H_

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <cassert>

#include "utils/logging.h"

#include "flatbuffers/flatbuffers.h"

namespace cirrus {

/** This class is used to manage the data
  * that moves between the TCP server all the way down to the
  * memory and storage backends
  * The goal here is to minimize number of copies
  */
class MemSlice {
 public:
    /**
      * Construct a mem slice from a std::string
      * we copy the string into a new vector
      */
    explicit MemSlice(const std::string& data) {
        fromString = true;
        dataStdVector_ = new std::vector<int8_t>(data.size());

        // copy contents over
        std::memcpy(const_cast<void*>(
                    reinterpret_cast<const void*>(dataStdVector_->data())),
                data.data(), data.size());
    }

    MemSlice(const std::vector<int8_t>* data) : 
        dataStdVector_(data), dataFbVector_(nullptr), fromString(false)
    {}

    MemSlice(const flatbuffers::Vector<int8_t>* data) :
        dataStdVector_(nullptr), dataFbVector_(data), fromString(false)
    {}

    ~MemSlice() {
        /**
          * We have to free this vector if we built this slice from a string
          */
        if (fromString)
            delete dataStdVector_;
    }
   
    /** Translate MemSlice to a string
      */ 
    std::string toString() const {
        // make sure s has the right size
        uint64_t size = 0;
        
        if (dataStdVector_) {
            size = dataStdVector_->size() * sizeof(int8_t);
        } else {
            size = dataFbVector_->size() * sizeof(int8_t);
        }

        LOG<INFO>("Resizing string with size: ", size);
        
        std::string s;
        s.resize(size);

        if (dataStdVector_) {
            s.assign(reinterpret_cast<const char*>(dataStdVector_->data()),
                    size);
        } else {
            s.assign(reinterpret_cast<const char*>(dataFbVector_->data()),
                    size);
        }

        return s;
    }

    uint64_t size() const {
        if (dataStdVector_) {
            return dataStdVector_->size() * sizeof(int8_t);
        } else {
            assert(dataFbVector_);
            return dataFbVector_->size() * sizeof(int8_t);
        }
    }

    /** Return contents of slice in vector form
      */
    std::vector<int8_t> get() const {
        if (dataStdVector_) {
            return *dataStdVector_;
        } else {
            assert(dataFbVector_);
            return std::vector<int8_t>(dataFbVector_->begin(),
                    dataFbVector_->end());
        }
    }

 private:
    const std::vector<int8_t>* dataStdVector_;
    const flatbuffers::Vector<int8_t>* dataFbVector_;

    bool fromString;  //< indicates whether slice contents came from string
};

/**
  * Base class for datastructures that store key-value data
  */
class StorageBackend {
 public:
    /**
      * Initialize backend
      */
    virtual void init() = 0;

    /**
      * Put object
      * @param oid Object ID
      * @param data MemSlice to be written
      * @return bool Indicates success (true) or failure (false)
      */
    virtual bool put(uint64_t oid, const MemSlice& data) = 0;

    /**
      * Check if object exists
      * @param oid Object ID
      * @return bool Indicates whether object exists (true) or not (false)
      */
    virtual bool exists(uint64_t) const = 0;

    /**
      * Get object
      * @param oid Object ID
      * @return MemSlice containing the object's data
      */
    virtual MemSlice get(uint64_t oid) const = 0;

    /**
      * Delete object
      * @param oid Object ID
      * @return bool Indicates success (true) or failure (false)
      */
    virtual bool delet(uint64_t oid) = 0;

    /**
      * Get size of object
      * @param oid Object ID
      * @return Size of the object
      */
    virtual uint64_t size(uint64_t oid) const = 0;
};

}  // namespace cirrus

#endif  // SRC_SERVER_STORAGEBACKEND_H_
