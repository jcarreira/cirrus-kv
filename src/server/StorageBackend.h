#ifndef SRC_SERVER_STORAGEBACKEND_H_
#define SRC_SERVER_STORAGEBACKEND_H_

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

namespace cirrus {

class MemSlice {
public:
    MemSlice(const std::string& data) {
        // make sure data_ has enough size
        data_.resize(data.size() / sizeof(int8_t));
        
        // copy contents over
        std::memcpy(data_.data(), data.data(), data.size());
    }

    MemSlice(const std::vector<int8_t>& data) : data_(data) {
    }

    operator std::string() const {
        std::string s;
        // make sure s has the right size
        uint64_t size = data_.size() * sizeof(int8_t);
        s.resize(size);

        s.assign(s.data(), data_.size() * sizeof(int8_t));

        return s;
    }
    
    operator std::vector<int8_t>() const {
        return data_;
    }

private:
    std::vector<int8_t> data_;
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
      * @param data Raw data to be written
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
      * @param data Memory to copy the object to
      * @return bool Indicates success (true) or failure (false)
      */
    virtual MemSlice get(uint64_t oid) = 0;

    /**
      * Delete object
      * @param oid Object ID
      * @return bool Indicates success (true) or failure (false)
      */
    virtual bool delet(uint64_t oid) = 0;

    /**
      * Get size of object
      * @param oid Object ID
      * @return bool Indicates success (true) or failure (false)
      */
    virtual uint64_t size(uint64_t oid) const = 0;
};

}  // namespace cirrus

#endif  // SRC_SERVER_STORAGEBACKEND_H_
