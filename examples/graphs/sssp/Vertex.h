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
    Vertex(int id, const std::vector<int>& neighbors,
            const std::vector<double>& distToNeighbors);

    void addNeighbor(int id, double distToNeighbor);
    void setNeighbors(const std::vector<int>& neighbors,
            const std::vector<double>& distToNeighbors);

    std::set<int> getNeighbors();
    int getId() const;
    bool hasNeighbor(int id) const;

    void setDist(double d);
    double getDist();
    double getDistToNeighbor(int id);

    /*
     * gets and sets the previous vertex
     */
    void setPrev(int p);
    int getPrev();

    /*
     * gets and sets if the vertex is on the fringe
     */
    void setOnFringe(bool val);
    bool getOnFringe();

    /*
     * gets and sets if the vertex has been processed
     */
    void setProcessed(bool val);
    int getProcessed();

private:
    int neighbors[1000];
    int id;
    int numNeighbors;
    int distToNeighbors[1000];
    int prev; //< the previous vertex
    double dist; //< the distance from the source to this vertex
    bool processed; //< if the algorithm has explored this vertex or not
    bool onFringe; //< if the vertex is on the fringe
};

} // graphs

#endif
