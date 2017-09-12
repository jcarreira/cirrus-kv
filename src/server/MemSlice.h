#ifndef SRC_SERVER_MEMSLICE_H_
#define SRC_SERVER_MEMSLICE_H_

#include <iterator>
#include <string>
#include <vector>
#include "utils/logging.h"

namespace cirrus {

/** This class is used to manage the data
  * that moves between the TCP server all the way down to the
  * memory and storage backends
  * We explicitly track types here to avoid expensive memory allocations
  * that are required to achieve polymorphism
  */
class MemSlice {
 public:
    /**
      * Construct a mem slice from a std::string
      * we copy the string into a new vector
      */
    explicit MemSlice(const std::string& data) :
        dataFbVector_(nullptr), begin(nullptr), end(nullptr), fromString(true) {
        dataStdVector_ = new std::vector<int8_t>(data.size());

        // copy contents over
        std::memcpy(const_cast<void*>(
                    reinterpret_cast<const void*>(dataStdVector_->data())),
                data.data(), data.size());
    }

    MemSlice(const std::vector<int8_t>* data) :
        dataStdVector_(data), dataFbVector_(nullptr),
        begin(nullptr), end(nullptr), fromString(false)
    {}

    explicit MemSlice(const flatbuffers::Vector<int8_t>* data) :
        dataStdVector_(nullptr), dataFbVector_(data),
        begin(nullptr), end(nullptr), fromString(false)
    {}

    MemSlice(const char* begin, const char* end) :
        dataStdVector_(nullptr), dataFbVector_(nullptr),
        begin(begin), end(end), fromString(false)
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
        uint64_t s_size = size();
        LOG<INFO>("Resizing string with size: ", s_size);

        std::string s;
        s.resize(s_size);

        if (dataStdVector_) {
            s.assign(reinterpret_cast<const char*>(dataStdVector_->data()),
                    s_size);
        } else if (dataFbVector_) {
            s.assign(reinterpret_cast<const char*>(dataFbVector_->data()),
                    s_size);
        } else if (begin && end) {
            s.assign(begin, s_size);
        } else {
            throw std::runtime_error("Error in toString()");
        }

        return s;
    }

    uint64_t size() const {
        if (dataStdVector_) {
            return dataStdVector_->size() * sizeof(int8_t);
        } else if (dataFbVector_) {
            return dataFbVector_->size() * sizeof(int8_t);
        } else if (begin && end) {
            return std::distance(begin, end);
        } else {
            throw std::runtime_error("Error in size()");
        }
    }

    /** Return contents of slice in vector form
      */
    std::vector<int8_t> get() const {
        if (dataStdVector_) {
            return *dataStdVector_;
        } else if (dataFbVector_) {
            return std::vector<int8_t>(dataFbVector_->begin(),
                    dataFbVector_->end());
        } else if (begin && end) {
            LOG<INFO>("Begin:", begin, end);
            return std::vector<int8_t>(begin, end);
        } else {
            throw std::runtime_error("Error in get()");
        }
    }

 private:
    const std::vector<int8_t>* dataStdVector_;
    const flatbuffers::Vector<int8_t>* dataFbVector_;
    const char* begin, *end;  //< being and end (exclusive) pointers to data

    bool fromString;  //< indicates whether slice contents came from string
};

}  // namespace cirrus

#endif  // SRC_SERVER_MEMSLICE_H_
