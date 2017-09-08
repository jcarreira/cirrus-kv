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
    rocksdb::Status s = rocksdb::DB::Open(options, path, &db);
    if (!s.ok()) {
        throw std::runtime_error("Error opening rocksdb");
    }
    LOG<INFO>("Opened rocksdb in path: ", path);
}

bool NVStorageBackend::put(uint64_t oid, const MemSlice& data) {
    LOG<INFO>("Putting in rocksdb. oid: ", oid,
            " with size: ", data.size());
    rocksdb::Status s = db->Put(rocksdb::WriteOptions(),
            std::to_string(oid), data.toString());

    if (!s.ok()) {
        throw std::runtime_error("Error in put in rocksdb");
    }
    return true;
}

bool NVStorageBackend::exists(uint64_t oid) const {
    std::string value;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(),
            std::to_string(oid), &value);

    LOG<INFO>("Object oid: ", oid, " not found: ", s.IsNotFound());
    return !s.IsNotFound();
}

MemSlice NVStorageBackend::get(uint64_t oid) const {
    std::string value;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(),
            std::to_string(oid), &value);
    if (!s.ok() || s.IsNotFound()) {
        throw std::runtime_error("Error in get in rocksdb");
    }

    LOG<INFO>("Get in rocksdb. oid: ", oid);
    return MemSlice(value);
}

bool NVStorageBackend::delet(uint64_t oid) {
    // we assume object exists
    db->Delete(rocksdb::WriteOptions(), std::to_string(oid));
    LOG<INFO>("Deleted oid: ", oid);
    return true;
}

uint64_t NVStorageBackend::size(uint64_t oid) const {
    MemSlice obj = get(oid);
    LOG<INFO>("Size of oid: ", oid, " is: ", obj.size());
    return obj.size();
}

}  // namespace cirrus
