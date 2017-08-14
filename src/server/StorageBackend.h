#ifndef SRC_SERVER_STORAGEBACKEND_H_
#define SRC_SERVER_STORAGEBACKEND_H_

#include <vector>
#include <cstdint>

namespace cirrus {

/**
  * Base class for datastructures that store key-value data
  */
class StorageBackend {
 public:
    using MemData = std::vector<int8_t>;

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
    virtual bool put(uint64_t oid, const std::vector<int8_t>& data) = 0;

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
    virtual const MemData& get(uint64_t oid) = 0;

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
