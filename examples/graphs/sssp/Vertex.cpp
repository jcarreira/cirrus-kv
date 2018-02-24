#include "Vertex.h"
#include <arpa/inet.h>
#include <limits>
namespace graphs {

Vertex::Vertex(int id) :
    id(id), prev(0),
    dist(std::numeric_limits<double>::infinity()),
    processed(false), onFringe(false) {
}

Vertex::Vertex(int id, const std::vector<int>& neighbors,
        const std::vector<double>& distToNeighbors) :
    id(id), prev(0),
    dist(std::numeric_limits<double>::infinity()),
    processed(false), onFringe(false) {
    setNeighbors(neighbors, distToNeighbors);
}

void Vertex::setNeighbors(const std::vector<int>& v,
        const std::vector<double>& dist) {
    if (v.size() != dist.size()) {
        // throw an exception
    }

    for (uint32_t i = 0; i < v.size(); i++) {
        addNeighbor(v[i], dist[i]);
    }
}

void Vertex::addNeighbor(int id, double distance) {
    std::pair<int, double> p(id, distance);
    neighbors.insert(p);
}

std::set<int> Vertex::getNeighbors() const {
    std::set<int> n;
    for (const auto& i : neighbors) {
        n.insert(i.first);
    }
    return n;
}

std::set<std::pair<int, double>> Vertex::getNeighborsAndEdges() const {
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
    return getNeighbors().find(id) != getNeighbors().end();
}

double Vertex::getDist() const {
    return dist;
}

void Vertex::setDist(double d) {
    dist = d;
}

double Vertex::getDistToNeighbor(int id) {
    for (const auto& n : neighbors) {
        if (n.first == id) {
            return n.second;
        }
    }
    return -1.0;
}

int Vertex::getPrev() const {
    return prev;
}

void Vertex::setPrev(int p) {
    prev = p;
}

int Vertex::getOnFringe() const {
    return onFringe;
}

void Vertex::setOnFringe(int val) {
    onFringe = val;
}

int Vertex::getProcessed() const {
    return processed;
}

void Vertex::setProcessed(int val) {
    processed = val;
}

Vertex Vertex::deserializer(const void* data, unsigned int size) {
    const double* double_ptr = reinterpret_cast<const double*>(data);
    Vertex v;
    v.setDist(*double_ptr++);

    std::cout << "deserialized with dist: " << v.getDist() << std::endl;

    std::cout << "size: " << size << std::endl;

    const uint32_t* ptr = reinterpret_cast<const uint32_t*>(double_ptr);
    v.setId(ntohl(*ptr++));
    v.setPrev(ntohl(*ptr++));
    v.setProcessed(ntohl(*ptr++));
    v.setOnFringe(ntohl(*ptr++));

    uint32_t n = ntohl (*ptr++);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t neigh_id = ntohl(*ptr++);
	double_ptr = reinterpret_cast<const double*>(ptr);
	double neigh_dist = *double_ptr++;
	v.addNeighbor(neigh_id, neigh_dist);
	ptr = reinterpret_cast<const uint32_t*>(double_ptr);
    }
    
    return v;
}

void Vertex::print() const {
    std::cout << "Vertex ID: " << id << std::endl;
    std::cout << "Prev: " << prev << ", Dist: " << dist << std::endl;

}

} // namespace graphs
