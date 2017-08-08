#include "NVStorageBackend.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>

namespace cirrus {

NVStorageBackend::NVStorageBackend(const std::string& path) :
    path(path) {
    fd = open(path.c_str(), O_RDWR);

    if (fd == -1) {
        throw std::runtime_error(
                std::string("Error opening raw device: ") + path);
    }
}

void NVStorageBackend::init() {
}

bool NVStorageBackend::put(uint64_t /* oid */, const MemData& /* data */) {
    return true;
}

bool NVStorageBackend::exists(uint64_t /* oid */) const {
    return false;
}

const StorageBackend::MemData& NVStorageBackend::get(uint64_t /* oid */ ) {
    return *(new MemData()); // BUG
}

bool NVStorageBackend::delet(uint64_t /* oid */) {
    return true;
}

uint64_t NVStorageBackend::size(uint64_t /* oid */) const {
    return 0;
}

}  // namespace cirrus
