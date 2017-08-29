#ifndef SRC_SERVER_NVSTORAGEBACKEND_H_
#define SRC_SERVER_NVSTORAGEBACKEND_H_

#include "StorageBackend.h"
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace rocksdb;

namespace cirrus {

/**
  * This class aims to provide an interface for
  * non-volatile storage devices such as SSDs and 3DXPoint
  */
class NVStorageBackend : public StorageBackend {
 public:
    explicit NVStorageBackend(const std::string& path = "/data/joao/rocksdb");

    void init();
    bool put(uint64_t oid, const MemSlice& data) override;
    bool exists(uint64_t oid) const override;
    MemSlice get(uint64_t oid) override;
    bool delet(uint64_t oid) override;
    uint64_t size(uint64_t oid) const override;

 private:
    std::string path;  //< path to raw device
    
    DB* db;           //< rocksdb handler 
    Options options;  //< rocksdb options

};

}  // namespace cirrus

#endif  // SRC_SERVER_NVSTORAGEBACKEND_H_
