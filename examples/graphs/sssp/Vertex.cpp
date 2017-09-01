#include "Vertex.h"
#include <arpa/inet.h>
#include <limits>
namespace graphs {

Vertex::Vertex(int id) :
    id(id), prev(0), processed(false), onFringe(false),
    dist(std::numeric_limits<double>::infinity()) {
}

Vertex::Vertex(int id, const std::vector<int>& neighbors,
        const std::vector<double>& distToNeighbors) :
    id(id), prev(0), processed(false), onFringe(false),
    dist(std::numeric_limits<double>::infinity()) {
    setNeighbors(neighbors, distToNeighbors);
}

void Vertex::setNeighbors(const std::vector<int>& v,
        const std::vector<double>& dist) {
    if (v.size() != dist.size()) {
        // throw an exception
    }

    for (int i = 0; i < v.size(); i ++) {
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
}

int Vertex::getPrev() const {
    return prev;
}

void Vertex::setPrev(int p) {
    prev = p;
}

bool Vertex::getOnFringe() const {
    return onFringe;
}

void Vertex::setOnFringe(bool val) {
    onFringe = val;
}

bool Vertex::getProcessed() const {
    return processed;
}

void Vertex::setProcessed(bool val) {
    processed = val;
}

Vertex Vertex::deserializer(const void* data, unsigned int size) {
    Vertex v;
    return v;
}

} // namespace graphs
