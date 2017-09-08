#ifndef SRC_SERVER_MEMORYBACKEND_H_
#define SRC_SERVER_MEMORYBACKEND_H_

#include "StorageBackend.h"
#include <unordered_map>
#include <vector>

namespace cirrus {

/**
  * This is a key-value store backed by memory
  * It's just a wrapper around a std::map or something similar
  */
class MemoryBackend : public StorageBackend {
 public:
     MemoryBackend() = default;
     virtual ~MemoryBackend() = default;

     void init();
     bool put(uint64_t oid, const MemSlice& data) override;
     bool exists(uint64_t oid) const override;
     MemSlice get(uint64_t oid) const override;
     bool delet(uint64_t oid) override;
     uint64_t size(uint64_t oid) const override;

 private:
     // make this mutable because std::map
     // doesn't play well with const
     mutable std::unordered_map<uint64_t, std::vector<int8_t>> store;
};

}  // namespace cirrus

#endif  // SRC_SERVER_MEMORYBACKEND_H_
