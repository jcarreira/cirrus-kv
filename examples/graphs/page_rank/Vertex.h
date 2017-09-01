#ifndef EXAMPLES_GRAPHS_PAGE_RANK_VERTEX_H_
#define EXAMPLES_GRAPHS_PAGE_RANK_VERTEX_H_

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
          * Deserializer routines
          */
        static Vertex deserializer(const void* data, unsigned int size);

        /**
          * Get and set current and next probabilities
          */
        double getCurrProb() const;
        void setCurrProb(double prob);
        double getNextProb() const;
        void setNextProb(double prob);

        /**
          * Print some info about this vertex
          */
        void print() const;

 private:
        std::set<int> neighbors;  //< set of neighboring vetrices
        int id;         //< id of the vertex
        double p_curr;  //< current probability of this vertex
        double p_next;  //<  probability of this vertex in the next round
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
            sizeof(uint32_t) * v.getNeighborsSize();  // neighbors
        return size;
    }
    void serialize(const Vertex& v, void* mem) const override {
        double* double_ptr = reinterpret_cast<double*>(mem);
        *double_ptr++ = v.getCurrProb();
        *double_ptr++ = v.getNextProb();

        uint32_t* ptr = reinterpret_cast<uint32_t*>(double_ptr);
        *ptr++ = htonl(v.getId());
        *ptr++ = htonl(v.getNeighborsSize());

        for (const auto& n : v.getNeighbors()) {
            *ptr++ = htonl(n);
        }
    }
 private:
};

}  // namespace graphs

#endif  // EXAMPLES_GRAPHS_PAGE_RANK_VERTEX_H_

