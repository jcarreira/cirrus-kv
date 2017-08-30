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

    //std::vector<int8_t> dt = data;
    std::string st = data;
    LOG<INFO>("Putting in rocksdb. oid: ", oid,
            " int8_t size is: ", data.size(),
            " string size is: ", st.size());
    Status s = db->Put(WriteOptions(), std::to_string(oid), st);

    if (!s.ok()) {
        throw std::runtime_error("Error in put in rocksdb");
    }
    return true;
}

bool NVStorageBackend::exists(uint64_t oid) const {
    std::string value;
    Status s = db->Get(ReadOptions(), std::to_string(oid), &value);
    
    LOG<INFO>("Object oid: ", oid, " not found: ", s.IsNotFound());
    return !s.IsNotFound();
}

MemSlice NVStorageBackend::get(uint64_t oid) const {
    std::string value;
    Status s = db->Get(ReadOptions(), std::to_string(oid), &value);
    if (!s.ok() || s.IsNotFound()) {
        throw std::runtime_error("Error in get in rocksdb");
    }
    
    LOG<INFO>("Get in rocksdb. oid: ", oid);
    return MemSlice(value);
}

bool NVStorageBackend::delet(uint64_t oid) {
    db->Delete(WriteOptions(), std::to_string(oid));
    LOG<INFO>("Deleted oid: ", oid);
    return true;
}

uint64_t NVStorageBackend::size(uint64_t oid) const {
    auto obj = get(oid); // memslice
    LOG<INFO>("Size of oid: ", oid, " is: ", obj.size());
    return obj.size();
}

}  // namespace cirrus
