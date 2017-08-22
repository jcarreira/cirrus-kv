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

        /**
          * Add and set neighbrs of this vertex
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
          * get and set the id of this vertex
          */
        int getId() const;
        void setId(int id);

        /**
          * Serializer and deserializer routines
          */
        static std::pair<std::unique_ptr<char[]>, unsigned int>
        serializer(const Vertex&);
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

}  // namespace graphs

#endif  // _VERTEX_H_

