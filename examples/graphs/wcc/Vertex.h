#ifndef _VERTEX_H_
#define _VERTEX_H_

#include <vector>
#include <iostream>
#include <set>
#include <memory>
#include <utility>
#include <arpa/inet.h>

#include "common/Serializer.h"

namespace graphs {

class Vertex {
 public:
        explicit Vertex(int id = 0);
        Vertex(int id, const std::vector<int>& neighbors);

        void addNeighbor(int id);
        void setNeighbors(const std::vector<int>& neighbors);

        std::set<int> getNeighbors() const;
        uint64_t getNeighborsSize() const;

        int getId() const;
        void setId(int id);

        bool hasNeighbor(int id) const;

        static Vertex deserializer(const void* data, unsigned int size);

        void print() const;

 public:
        bool seen;  //< flag to indicate whether node has been traversed yet
        int cc;     //< weakly connected component id (-1 if not assigned yet)

 private:
        std::set<int> neighbors;  //< set of the neighbors of this node

        int id;  //< id of this vertex
};

/** Format:
  * id (uint32_t)
  * n neighbors (uint32_t)
  * neighbors list (n * uint32_t)
  */
class VertexSerializer : public cirrus::Serializer<Vertex> {
 public:
    uint64_t size(const Vertex& v) const override {
        uint64_t size = sizeof(uint32_t) * 2 +
            sizeof(uint32_t) * v.getNeighborsSize();  // neighbors
        return size;
    }
    void serialize(const Vertex& v, void* mem) const override {
        std::cout << "Serializing vertex: "
            << std::endl;

        uint32_t* ptr = reinterpret_cast<uint32_t*>(mem);
        *ptr++ = htonl(v.getId());
        *ptr++ = htonl(v.getNeighborsSize());

        for (const auto& n : v.getNeighbors()) {
            *ptr++ = htonl(n);
        }
    }
 private:
};

}  // namespace graphs

#endif  // _VERTEX_H_
