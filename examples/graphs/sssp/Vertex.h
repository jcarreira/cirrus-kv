#ifndef VERTEX_H
#define VERTEX_H

#include <arpa/inet.h>
#include <vector>
#include <iostream>
#include <set>
#include <memory>
#include <utility>

#include "common/Serializer.h"

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

    int getOnFringe() const;
    void setOnFringe(int val);

    /*
     * Get and set if the vertex has been processed
     */

    int getProcessed() const;
    void setProcessed(int val);

    /*
     * Deserializer
     */

    static Vertex deserializer(const void* data, unsigned int size);
    void print() const;
private:
    std::set<std::pair<int, double>> neighbors; //<set of (neighbor id, edge dist)
    int id;
    int prev; //< the previous vertex
    double dist; //< the distance from the source to this vertex
    int processed; //< if the algorithm has explored this vertex or not
    int onFringe; //< if the vertex is on the fringe
};

/** Format:
 * dist (double)
 * id (uint32_t)
 * prev (uint32_t)
 * processed (uint32_t)
 * onFringe (uint32_t)
 * n (uint32_t), number of neighbors
 * n (neighbor id, edge dist) (uint32_t, double)
 */

class VertexSerializer : public cirrus::Serializer<Vertex> {
 public:
    uint64_t size(const Vertex& v) const override {
        uint64_t size = sizeof(uint32_t) * 5 +
            sizeof(double) +
            sizeof(uint32_t) * v.getNeighborsSize() +
	    sizeof(double) * v.getNeighborsSize();  // neighbors
        return size;
    }
    void serialize(const Vertex& v, void* mem) const override {
        double* double_ptr = reinterpret_cast<double*>(mem);
        *double_ptr++ = v.getDist();
        
        uint32_t* ptr = reinterpret_cast<uint32_t*>(double_ptr);
        *ptr++ = htonl(v.getId());
	*ptr++ = htonl(v.getPrev());
	*ptr++ = htonl(v.getProcessed());
	*ptr++ = htonl(v.getOnFringe());

	*ptr++ = htonl(v.getNeighborsSize());

	for (const std::pair<int, double>& n : v.getNeighborsAndEdges()) {
	    *ptr++ = htonl(n.first);
	    double_ptr = reinterpret_cast<double*>(ptr);
	    *double_ptr++ = n.second;
	    ptr = reinterpret_cast<uint32_t*>(double_ptr);
	}
    }
 private:
};


} // graphs

#endif
