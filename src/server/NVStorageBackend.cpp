#include "src/server/NVStorageBackend.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>
#include "utils/logging.h"

/**
  * Interface over RocksDB
  */


namespace cirrus {

NVStorageBackend::NVStorageBackend(const std::string& path) :
    path(path) {
}

void NVStorageBackend::init() {
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();

    options.create_if_missing = true;

    // open DB
    Status s = DB::Open(options, path, &db);
    if (!s.ok()) {
        throw std::runtime_error("Error opening rocksdb");
    }
    LOG<INFO>("Opened rocksdb in path: ", path);
}

bool NVStorageBackend::put(uint64_t oid, const MemSlice& data) {

    std::string st = data;
    Status s = db->Put(WriteOptions(), std::to_string(oid), st);

    if (!s.ok()) {
        throw std::runtime_error("Error in put in rocksdb");
    }
    LOG<INFO>("Put in rocksdb");
    return true;
}

bool NVStorageBackend::exists(uint64_t oid) const {
    std::string value;
    Status s = db->Get(ReadOptions(), std::to_string(oid), &value);
    return s.ok();
}

MemSlice NVStorageBackend::get(uint64_t oid) {
    std::string value;
    Status s = db->Get(ReadOptions(), std::to_string(oid), &value);
    if (!s.ok()) {
        throw std::runtime_error("Error in get in rocksdb");
    }
    return MemSlice(value);
}

bool NVStorageBackend::delet(uint64_t /* oid */) {
    return true;
}

uint64_t NVStorageBackend::size(uint64_t /* oid */) const {
    return 0;
}

}  // namespace cirrus
