#include "Vertex.h"

namespace graphs {

Vertex::Vertex(int id, const std::vector<int>& neighbors,
        const std::vector<double>& distToNeighbors) :
    id(id), prev(-1), processed(false),
    onFringe(false), dist(std::numeric_limits<double>::infinity())
{
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
    neighbors[numNeighbors] = id;
    distToNeighbors[numNeighbors] = distance;
    numNeighbors += 1;
}

std::set<int> Vertex::getNeighbors() {
    std::set<int> n;
    for (int i = 0; i < numNeighbors; i++) {
        n.insert(neighbors[i]);
    }
    return n;
}

int Vertex::getId() const {
    return id;
}

bool Vertex::hasNeighbor(int id) const {
    for (int i = 0; i < numNeighbors; i++) {
        if (neighbors[i] == id) {
            return true;
        }
    }
    return false;
}

void Vertex::setDist(double d) {
    dist = d;
}

double Vertex::getDist() {
    return dist;
}

double Vertex::getDistToNeighbor(int id) {
    int count = 0;
    for (const auto& i : neighbors) {
        if (i == id) {
            return distToNeighbors[i];
        }
        count++;
    }
    return -1.0;
}

void Vertex::setPrev(int p) {
    prev = p;
}

int Vertex::getPrev() {
    return prev;
}

void Vertex::setOnFringe(bool val) {
    onFringe = val;
}

bool Vertex::getOnFringe() {
    return onFringe;
}

void Vertex::setProcessed(bool val) {
    processed = val;
}

int Vertex::getProcessed() {
    return processed;
}

}
