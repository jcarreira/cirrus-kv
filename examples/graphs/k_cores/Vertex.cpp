#include "Vertex.h"

namespace graphs {

Vertex::Vertex(int id, const std::vector<int>& neighbors) :
    id(id)
{
    setNeighbors(neighbors);
}

void Vertex::setNeighbors(const std::vector<int>& v) {
    for (const auto& i : v) {
        addNeighbor(i);
    }
}

void Vertex::addNeighbor(int id) {
    neighbors[numNeighbors] = id;
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

void Vertex::setK(int val) {
    k = val;
}

}
