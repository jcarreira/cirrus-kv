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
    Vertex(int id, const std::vector<int>& neighbors);


    /**
     * Add and set neighbors of this vertex
     */

    void addNeighbor(int id);
    void setNeighbors(const std::vector<int>& neighbors);

    /**
     * Get set of neighbors and neighbors size
     */
    std::set<int> getNeighbors() const;
    int getNeighborsSize() const;

    /**
     * Check if vertex is a neighbor
     */
    bool hasNeighbor(int id) const;

    /**
     * Get and set the id of this vertex
     */
    int getId() const;
    void setId(int i);

    /**
     * Deserializer routines
     */

    static Vertex deserializer(const void* data, unsigned int size);

    /**
     * Utility methods for temp neighbors used in the kcores algorithm
     * temp neighbors keep track of the degree of the vertex in the algorithm
     */
    void deleteTempNeighbor(int id);
    std::set<int> getTempNeighbors() const;
    void setTempNeighbors(std::set<int> n);
    int getTempNeighborsSize() const;
    int getK() const;
    void setK(int val);
    bool getSeen() const;
    void setSeen(bool val);
private:
    std::set<int> neighbors;
    std::set<int> tempNeighbors; //< to keep track of degree in the alg
    int id;
    int k; //< maximum core number
    bool seen; //< true if the vertex has been processed by the alg
};

/** Format:
  * p_curr (double)
  * p_next (double)
  * id (uint32_t)
  * n neighbors (uint32_t)
  * neighbors list (n * uint32_t)
  */
class VertexSerializer : public cirrus::Serializer<Vertex> {
 public:
    uint64_t size(const Vertex& v) const override {
        uint64_t size = sizeof(uint32_t) * 2 +
            sizeof(double) * 2 +
            sizeof(uint32_t) * v.getNeighbors().size();  // neighbors
        return size;
    }
    void serialize(const Vertex& v, void* mem) const override {
        double* double_ptr = reinterpret_cast<double*>(mem);
        //*double_ptr++ = v.getCurrProb();
        //*double_ptr++ = v.getNextProb();

        uint32_t* ptr = reinterpret_cast<uint32_t*>(double_ptr);
        *ptr++ = htonl(v.getId());
        *ptr++ = htonl(v.getNeighbors().size());

        for (const auto& n : v.getNeighbors()) {
            *ptr++ = htonl(n);
        }
    }
 private:
};

} // graphs

#endif
