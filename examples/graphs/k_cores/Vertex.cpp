#include <examples/graphs/k_cores/Vertex.h>
#include <arpa/inet.h>

namespace graphs {

Vertex::Vertex(int id) :
    id(id), k(-1), seen(false) {}
	
Vertex::Vertex(int id, const std::vector<int>& neighbors) :
    id(id), k(-1), seen(false)
{
    setNeighbors(neighbors);
}

void Vertex::setNeighbors(const std::vector<int>& v) {
    for (const auto& i : v) {
        addNeighbor(i);
    }
}

void Vertex::addNeighbor(int id) {
    neighbors.insert(id);
    tempNeighbors.insert(id);
}

std::set<int> Vertex::getNeighbors() const {
    return neighbors;
}

int Vertex::getNeighborsSize() const {
    return neighbors.size();
}

bool Vertex::hasNeighbor(int id) const {
    return neighbors.find(id) != neighbors.end();
}

Vertex Vertex::deserializer(const void* data, unsigned int size) {
    const double* double_ptr = reinterpret_cast<const double*>(data);

    Vertex v;
    //v.setCurrProb(*double_ptr++);
    //v.setNextProb(*double_ptr++);

    //std::cout << "deserialized with currProb: " << v.getCurrProb()
    //    << " nextProb: " << v.getNextProb()
    //    << std::endl;

    const uint32_t* ptr = reinterpret_cast<const uint32_t*>(double_ptr);
    v.setId(ntohl(*ptr++));
    uint32_t n = ntohl(*ptr++);

    uint32_t expected_size = 2 * sizeof(double) + sizeof(uint32_t) * (2 + n);

    if (size != expected_size) {
        throw std::runtime_error("Incorrect size: "
                + std::to_string(size) +
                + " expected: " + std::to_string(expected_size));
    }

    for (uint32_t i = 0; i < n; ++i) {
        v.addNeighbor(ntohl(*ptr++));
    }
    return v;
}

int Vertex::getId() const {
    return id;
}

void Vertex::setId(int i) {
    id = i;
}

void Vertex::deleteTempNeighbor(int id) {
    std::set<int>::iterator it = tempNeighbors.find(id);
    tempNeighbors.erase(it, tempNeighbors.end());
}

std::set<int> Vertex::getTempNeighbors() const {
    return tempNeighbors;
}

int Vertex::getTempNeighborsSize() const {
    return tempNeighbors.size();
}

int Vertex::getK() const {
    return k;
}

void Vertex::setK(int val) {
    k = val;
}

bool Vertex::getSeen() const {
    return seen;
}

void Vertex::setSeen(bool val) {
    seen = val;
}

}
