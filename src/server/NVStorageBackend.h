#include "StorageBackend.h"
#include <string>

namespace cirrus {

/**
  * This class aims to provide an interface for
  * non-volatile storage devices such as SSDs and 3DXPoint
  */
class NVStorageBackend : public StorageBackend {
 public:
     NVStorageBackend(const std::string& path);

     void init();
     bool put(uint64_t oid, const StorageBackend::MemData& data) override;
     bool exists(uint64_t oid) const override;
     const StorageBackend::MemData& get(uint64_t oid) override;
     bool delet(uint64_t oid) override;
     uint64_t size(uint64_t oid) const override;

 private:
     std::string path; //< path to raw device
     int fd; //< file descriptor used to access raw device
};

}  // namespace cirrus
