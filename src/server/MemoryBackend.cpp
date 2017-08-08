#include "MemoryBackend.h"

#include <cstring>

namespace cirrus {

bool MemoryBackend::put(uint64_t oid, const std::vector<int8_t>& data) {
    store[oid] = data;
    return true;
}

bool MemoryBackend::exists(uint64_t oid) const {
    return store.find(oid) != store.end();
}

bool MemoryBackend::get(uint64_t oid, void* data) {
    auto it = store.find(oid);
    if (it == store.end())
        return false;

    std::memcpy(data, it->second.data(), it->second.size());

    return true;
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
