#ifndef SRC_SERVER_STORAGEBACKEND_H_
#define SRC_SERVER_STORAGEBACKEND_H_

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <cassert>

#include "utils/logging.h"

#include "flatbuffers/flatbuffers.h"
#include "MemSlice.h"

namespace cirrus {

/**
  * Base class for datastructures that store key-value data
  */
class StorageBackend {
 public:
     virtual ~StorageBackend() = default;

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
