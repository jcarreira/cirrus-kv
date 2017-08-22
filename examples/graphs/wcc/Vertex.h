#ifndef _VERTEX_H_
#define _VERTEX_H_

#include <vector>
#include <iostream>
#include <set>
#include <memory>
#include <utility>

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

        static std::pair<std::unique_ptr<char[]>, unsigned int>
        serializer(const Vertex&);
        static Vertex deserializer(const void* data, unsigned int size);

        void print() const;

 public:
        bool seen;  //< flag to indicate whether node has been traversed yet
        int cc;     //< weakly connected component id (-1 if not assigned yet)

 private:
        std::set<int> neighbors;  //< set of the neighbors of this node

        int id;  //< id of this vertex
};

}  // namespace graphs

#endif  // _VERTEX_H_
