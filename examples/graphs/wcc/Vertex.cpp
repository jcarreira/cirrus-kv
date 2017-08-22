#include "Vertex.h"
#include <arpa/inet.h>

namespace graphs {

Vertex::Vertex(int id) :
    seen(false), cc(-1), id(id) {
}

Vertex::Vertex(int id, const std::vector<int>& neighbors) :
    seen(false), cc(-1), id(id) {
    setNeighbors(neighbors);
}

void Vertex::setNeighbors(const std::vector<int>& v) {
    for (const auto& i : v) {
        addNeighbor(i);
    }
}

void Vertex::addNeighbor(int id) {
    neighbors.insert(id);
}

std::set<int> Vertex::getNeighbors() const {
    return neighbors;
}

uint64_t Vertex::getNeighborsSize() const {
    return neighbors.size();
}

int Vertex::getId() const {
    return id;
}

void Vertex::setId(int i) {
    id = i;
}

bool Vertex::hasNeighbor(int id) const {
    return neighbors.find(id) != neighbors.end();
}

/** Format:
  * id (uint32_t)
  * n neighbors (uint32_t)
  * neighbors list (n * uint32_t)
  */
std::pair<std::unique_ptr<char[]>, unsigned int>
Vertex::serializer(const Vertex& v) {
    uint64_t size = sizeof(uint32_t) * 2 +
        sizeof(uint32_t) * v.getNeighborsSize();  // neighbors

    std::cout << "Serializing object with size: "
        << size
        << std::endl;

    std::unique_ptr<char[]> mem(new char[size]);

    uint32_t* ptr = reinterpret_cast<uint32_t*>(mem.get());
    *ptr++ = htonl(v.getId());
    *ptr++ = htonl(v.getNeighborsSize());

    for (const auto& n : v.getNeighbors()) {
        *ptr++ = htonl(n);
    }
    return std::make_pair(std::move(mem), size);
}

Vertex Vertex::deserializer(const void* data, unsigned int size) {
    Vertex v;

    const uint32_t* ptr = reinterpret_cast<const uint32_t*>(data);
    v.setId(ntohl(*ptr++));
    uint32_t n = ntohl(*ptr++);

    uint32_t expected_size = sizeof(uint32_t) * (2 + n);

    if (size != expected_size) {
        throw std::runtime_error("Incorrect size: "
                + std::to_string(size));
    }

    for (uint32_t i = 0; i < n; ++i) {
        v.addNeighbor(ntohl(*ptr++));
    }
    return v;
}

void Vertex::print() const {
    std::cout
        << "Print vertex id: " << id
        << std::endl;
}

}  // namespace graphs
