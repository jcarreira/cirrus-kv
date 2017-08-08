#include "BackendMap.h"
#include <map>

namespace cirrus {

/**
  * This is key-value store backed by memory
  * It used a std::map key-value store to store data
  */
class MemoryBackend : public BackendMap {
 public:
     MemoryBackend() = default;

     bool put(uint64_t oid, const std::vector<int8_t>& data) override;
     bool exists(uint64_t oid) const override;
     bool get(uint64_t oid, void* data) override;
     bool delet(uint64_t oid) override;
     uint64_t size(uint64_t oid) const override;

 private:
     std::map<uint64_t, std::vector<int8_t>> store;
};

}  // namespace cirrus
