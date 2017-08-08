#include "StorageBackend.h"
#include <map>

namespace cirrus {

/**
  * This is key-value store backed by memory
  * It used a std::map key-value store to store data
  */
class MemoryBackend : public StorageBackend {
 public:
     MemoryBackend() = default;

     void init();
     bool put(uint64_t oid, const MemData& data) override;
     bool exists(uint64_t oid) const override;
     const StorageBackend::MemData& get(uint64_t oid) override;
     bool delet(uint64_t oid) override;
     uint64_t size(uint64_t oid) const override;

 private:
     std::map<uint64_t, std::vector<int8_t>> store;
};

}  // namespace cirrus
