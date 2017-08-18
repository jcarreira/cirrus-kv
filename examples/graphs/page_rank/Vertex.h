#ifndef VERTEX_H_
#define VERTEX_H_

#include <vector>
#include <iostream>
#include <set>
#include <memory>

namespace graphs {

class Vertex {
    public:
        Vertex(int id = 0);
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

        double getCurrProb() const;
        void setCurrProb(double prob);
        double getNextProb() const;
        void setNextProb(double prob);

        void print() const;

    private:
        std::set<int> neighbors;

        int id; //< id of the vertex

        // these values hold the current and next probabilities of the pagerank
        // algorithm. They alternate betwee
        double p_curr;
        double p_next;
};

} // graphs

#endif

