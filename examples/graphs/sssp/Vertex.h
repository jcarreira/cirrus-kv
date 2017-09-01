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

    static Vertex deserializer(const void* data, unsigned int size); 
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
 * id (uint32_t)
 * prev (uint32_t)
 * processed (uint32_t), where 1 represents true
 * onFringe (uint32_t), where 1 represents true
 * n (uint32_t), number of neighbors
 * n (neighbor id, edge dist) (uint32_t, double)
 */

class VertexSerializer : public cirrus::Serializer<Vertex> {
 public:
    uint64_t size(const Vertex& v) const override {
        uint64_t size = sizeof(uint32_t) * 2 +
            sizeof(double) +
	    //sizeof(bool) * 2 +
	    sizeof(uint32_t) * 3 +
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

        if (v.getProcessed()) {
	    *ptr++ = htonl(1);
	} else {
	    *ptr++ = htonl(0);
	}
	if (v.getOnFringe()) {
	    *ptr++ = htonl(1);
	} else {
	    *ptr++ = htonl(0);
	}

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
