#ifndef VERTEX_H
#define VERTEX_H

#include <arpa/inet.h>
#include <vector>
#include <iostream>
#include <set>
#include <memory>
#include <utility>

//#include "common/Serializer.h"

namespace graphs {

class Vertex {
public:
    explicit Vertex(int id = 0);
    Vertex(int id, const std::vector<int>& neighbors,
            const std::vector<double>& distToNeighbors);

    /**
     * Add and set neighbors of this vertex
     */

    void addNeighbor(int id, double distToNeighbor);
    void setNeighbors(const std::vector<int>& neighbors,
            const std::vector<double>& distToNeighbors);

    /**
     * Get set of neighbors and neighbors size
     */

    std::set<int> getNeighbors() const;
    std::set<std::pair<int, double>> getNeighborsAndEdges() const;
    uint64_t getNeighborsSize() const;
    
    /**
     * Check if vertex is a neighbor
     */

    bool hasNeighbor(int id) const;

    /**
     * Get and set the id of this vertex
     */

    int getId() const;
    void setId(int id);

    /**
     * Get and set distance to this vertex from source
     */

    double getDist() const;
    void setDist(double d);
    
    /**
     * Get distance to neighbor id from this vertex
     */
    
    double getDistToNeighbor(int id);

    /*
     * Get and set the previous vertex
     */

    int getPrev() const;
    void setPrev(int p);

    /*
     * Get and set if the vertex is on the fringe
     */

    bool getOnFringe() const;
    void setOnFringe(bool val);

    /*
     * Get and set if the vertex has been processed
     */

    bool getProcessed() const;
    void setProcessed(bool val);

    /*
     * Deserializer
     */

    Vertex Vertex::deserializer(const void* data, unsigned int size); 
private:
    std::set<std::pair<int, double>> neighbors; //<set of (neighbor id, edge dist)
    int id;
    int prev; //< the previous vertex
    double dist; //< the distance from the source to this vertex
    bool processed; //< if the algorithm has explored this vertex or not
    bool onFringe; //< if the vertex is on the fringe
};

/** Format:
 * dist (double)
 * processed (bool)
 * onFringe (bool)
 * id (uint32_t)
 * prev(uint32_t)
 * n (neighbor id, edge dist) (uint32_t, double)
 */

class VertexSerializer : public cirrus::Serializer<Vertex> {
 public:
    uint64_t size(const Vertex& v) const override {
        uint64_t size = sizeof(uint32_t) * 2 +
            sizeof(double) +
	    sizeof(bool) * 2 +
            sizeof(uint32_t) * v.getNeighborsSize() +
	    sizeof(double) * v.getNeighborsSize();  // neighbors
        return size;
    }
    void serialize(const Vertex& v, void* mem) const override {
        double* double_ptr = reinterpret_cast<double*>(mem);
        *double_ptr++ = v.getDist();
	for (const auto& n : v.getNeighbors()) {
            *double_ptr++ = n.second;
        }
        uint32_t* ptr = reinterpret_cast<uint32_t*>(double_ptr);
        *ptr++ = htonl(v.getId());

        for (const auto& n : v.getNeighbors()) {
            *ptr++ = htonl(n.first);
        }
        if (v.getProcessed()) {
	    *ptr++ = 1;
	}
	if (v.getOnFringe()) {
	    *ptr++ = 1;
	}
    }
 private:
};


} // graphs

#endif
