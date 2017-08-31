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

    void deleteTempNeighbor(int id);
    std::vector<int> getTempNeighbors();
    int numTempNeighbors();
    void setK(int val);
    void setSeen(bool val);
    bool getSeen();
private:
    int neighbors[1000];
    std::vector<int> tempNeighbors; //< to keep track of degree in the alg
    int id;
    int numNeighbors;
    int k; //< maximum core number
    bool seen; //< true if the vertex has been processed by the alg
};

} // graphs

#endif
