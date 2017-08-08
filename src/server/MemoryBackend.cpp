#include "MemoryBackend.h"

#include <cstring>

namespace cirrus {

void MemoryBackend::init() {

}

bool MemoryBackend::put(uint64_t oid, const std::vector<int8_t>& data) {
    store[oid] = data;
    return true;
}

bool MemoryBackend::exists(uint64_t oid) const {
    return store.find(oid) != store.end();
}

const StorageBackend::MemData& MemoryBackend::get(uint64_t oid) {
    auto it = store.find(oid);
    if (it == store.end()) {
        throw std::runtime_error("Error");
        //return false;
    }

    //std::memcpy(data, it->second.data(), it->second.size());
    //return true;
    return store[oid];
}

bool MemoryBackend::delet(uint64_t oid) {
    store.erase(oid); // we assume object exists
    return true;
}

uint64_t MemoryBackend::size(uint64_t oid) const {
    auto it = store.find(oid);
    if (it == store.end()) {
        return 0;
    }

    return it->second.size();
}

}  // namespace cirrus
