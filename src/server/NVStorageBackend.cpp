#include "src/server/NVStorageBackend.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>
#include "utils/logging.h"

/**
  * Design of an on-SSD key-value store
  * Components
  * 1. RAM write buffer. Writes are written first to memory.
  * Buffers are flushed out after they fill up or after a timeout
  * a. Strict durability mode only returns when write buffer has been flushed
  * b. Weak durability mode returns immediately
  *
  * 2. RAM Cache maps objects to SSD
  *
  * 3. Disk presence bloom filter
  *
  */



namespace cirrus {

NVStorageBackend::NVStorageBackend(const std::string& path) :
    path(path) {
    fd = open(path.c_str(), O_RDWR);

    if (fd == -1) {
        throw std::runtime_error(
                std::string("Error opening raw device: ") + path);
    }

    LOG<INFO>("Opened storage path: ", path,
            " fd: ", fd);
}

void NVStorageBackend::init() {
}

bool NVStorageBackend::put(uint64_t /* oid */, const MemData& /* data */) {
    return true;
}

bool NVStorageBackend::exists(uint64_t /* oid */) const {
    /** Check bloom filter
      * If bloom filter say it might exist, go to hash table
      */
    return false;
}

const StorageBackend::MemData& NVStorageBackend::get(uint64_t /* oid */ ) {
    /** Check bloom filter
      * Check hash table object id to position
      */
    return *(new MemData());  // BUG
}

bool NVStorageBackend::delet(uint64_t /* oid */) {
    return true;
}

uint64_t NVStorageBackend::size(uint64_t /* oid */) const {
    return 0;
}

}  // namespace cirrus
