#ifndef VERTEX_H
#define VERTEX_H

#include <vector>
#include <iostream>
#include <set>

namespace graphs {

class Vertex {
public:
    Vertex() = default;
    Vertex(int id) : id(id) {}
    Vertex(int id, const std::vector<int>& neighbors);

    void addNeighbor(int id);
    void setNeighbors(const std::vector<int>& neighbors);

    std::set<int> getNeighbors();
    int getId() const;
    bool hasNeighbor(int id) const;

    void deleteNeighbor(int id);
private:
    int neighbors[1000];
    std::vector<int> kCoreNeighbors;
    int id;
    int numNeighbors;
};

} // graphs

#endif
