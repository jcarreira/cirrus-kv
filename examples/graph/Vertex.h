#ifndef VERTEX_H_
#define VERTEX_H_

#include <vector>
#include <iostream>
#include <set>
#include <memory>

namespace graphs {

class Vertex {
    public:
        Vertex(int id = 0) : id(id) {}
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

    private:
        std::set<int> neighbors;
        int id;
};

} // graphs

#endif

