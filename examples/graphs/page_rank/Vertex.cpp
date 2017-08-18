#include "Vertex.h"
#include <arpa/inet.h>

namespace graphs {
        
Vertex::Vertex(int id) :
    id(id), p_curr(1.0), p_next(0.0) {
}

Vertex::Vertex(int id, const std::vector<int>& neighbors) :
    id(id), p_curr(1.0), p_next(0.0) {
    setNeighbors(neighbors);
}

void Vertex::setNeighbors(const std::vector<int>& v) {
    for (const auto& i : v) {
        addNeighbor(i);
        neighbors.insert(i);
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
  * p_curr (double)
  * p_next (double)
  * id (uint32_t)
  * n neighbors (uint32_t)
  * neighbors list (n * uint32_t)
  */
std::pair<std::unique_ptr<char[]>, unsigned int>
Vertex::serializer(const Vertex& v) {
    uint64_t size = sizeof(uint32_t) * 2 +
        sizeof(double) * 2 +
        sizeof(uint32_t) * v.getNeighborsSize(); // neighbors

    std::cout << "Serializing object with size: "
        << size
        << std::endl;

    std::unique_ptr<char[]> mem(new char[size]);
    
    double* double_ptr = reinterpret_cast<double*>(mem.get());
    *double_ptr++ = v.getCurrProb();
    *double_ptr++ = v.getNextProb();

    uint32_t* ptr = reinterpret_cast<uint32_t*>(double_ptr);
    *ptr++ = htonl(v.getId());
    *ptr++ = htonl(v.getNeighborsSize());

    for (const auto& n : v.getNeighbors()) {
        *ptr++ = htonl(n);
    }
    return std::make_pair(std::move(mem), size);
}

Vertex Vertex::deserializer(const void* data, unsigned int size) {
    const double* double_ptr = reinterpret_cast<const double*>(data);

    Vertex v;
    v.setCurrProb(*double_ptr++);
    v.setNextProb(*double_ptr++);

    std::cout << "deserialized with currProb: " << v.getCurrProb()
        << " nextProb: " << v.getNextProb()
        << std::endl;
    
    const uint32_t* ptr = reinterpret_cast<const uint32_t*>(double_ptr);
    v.setId(ntohl(*ptr++));
    uint32_t n = ntohl(*ptr++);

    uint32_t expected_size = 2 * sizeof(double) + sizeof(uint32_t) * (2 + n);

    if (size != expected_size) {
        throw std::runtime_error("Incorrect size: "
                + std::to_string(size));
    }

    for (uint32_t i = 0; i < n; ++i) {
        v.addNeighbor(ntohl(*ptr++));
    }
    return v;
}
         
double Vertex::getCurrProb() const {
    return p_curr;
}

void Vertex::setCurrProb(double prob) {
    p_curr = prob;
}

double Vertex::getNextProb() const {
    return p_next;
}

void Vertex::setNextProb(double prob) {
    p_next = prob;
}

void Vertex::print() const {
    std::cout 
        << "Print vertex id: " << id
        << " p_next: " << p_next
        << " p_curr: " << p_curr
        << std::endl;
}

}
