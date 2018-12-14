#include "MemoryBackend.h"

#include <cstring>
#include <malloc.h>
#include "server/MemoryBackend.h"

namespace cirrus {

MemoryBackend::MemoryBackend(uint64_t bytes) : remove_increment(bytes) {}

void MemoryBackend::init() {}

bool MemoryBackend::put(uint64_t oid, const MemSlice& data) {
    store[oid] = data.get();
    return true;
}

bool MemoryBackend::exists(uint64_t oid) const {
    return store.find(oid) != store.end();
}

MemSlice MemoryBackend::get(uint64_t oid) const {
    auto it = store.find(oid);
    if (it == store.end()) {
        throw std::runtime_error(
                "MemoryBackend get() called on nonexistent id");
    }

    return &it->second;
}

bool MemoryBackend::delet(uint64_t oid) {
    // we assume object exists
    remove_counter += store[oid].size();
    store.erase(oid);
    if (remove_counter >= remove_increment) {
        remove_counter %= remove_increment;
        malloc_trim(0);
    }
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
