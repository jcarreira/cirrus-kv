#include "PageRank2.h"
#include "cache_manager/CacheManager.h"

#include <stdlib.h>
#include <cmath>

#define EPS (1e-05)
#define EQUAL(a,b) (fabs(a-b) < EPS)

namespace graphs {

/**
  * Compute PageRank on graph stored in a Cirrus store
  * Preconditions:
  * a) vertices are stored in the range 0..num_vertices
  * b) Current probability of vertices is initialized to 1.0 and next to 0.0
  * c)
  * @param cm CacheManager to be used to access the graph
  * @param num_vertices Number of vertices in the graph
  * @param gamma PageRanks's algorithm gamma
  */
void pageRank2(cirrus::CacheManager<Vertex>& cm,
        unsigned int num_vertices,
        double gamma, uint64_t num_iterations) {

    //int count = 1;
    double error = 0;
    for (uint64_t it = 0; it < num_iterations; ++it) {
        //std::cout << "PageRank iteration: " << count++ << std::endl;
        double prev_error = error;

        for (unsigned int i = 0; i < num_vertices; i++) {
            Vertex curr = cm.get(i);

            auto neighbors = curr.getNeighbors();
            for (auto it = neighbors.begin();
                    it != neighbors.end();
                    ++it) {
                Vertex n = cm.get(*it);

                //auto it2 = it;
                //++it2;
                //if (it2 != neighbors.end()) {
                //    cm.prefetch(*it2);
                //}
                //++it2;
                //if (it2 != neighbors.end()) {
                //    cm.prefetch(*it2);
                //}

                //std::cout << "Updating vertex: " << neighbor
                //    << " with new value: "
                //    << n.getNextProb() + curr.getCurrProb() / neighbors.size()
                //    << " getNextProb: " << curr.getNextProb()
                //    << " getCurrProb: " << curr.getCurrProb()
                //    << " neighbors.size(): " << neighbors.size()
                //    << std::endl;
                n.setNextProb(n.getNextProb() +
                        curr.getCurrProb() / neighbors.size());
                cm.put(*it, n);
            }
        }

        // use iterator here
        for (unsigned int i = 0; i < num_vertices; i++) {

            prev_error = error;
            Vertex curr = cm.get(i);

            // update p_curr for next round
            curr.setCurrProb(curr.getNextProb() * gamma + (1.0 - gamma) /
                    static_cast<double>(num_vertices));
            //std::cout << "New currProb: " << curr.getCurrProb()
            //    << " nextProb: " << curr.getNextProb()
            //    << std::endl;

            error += std::abs(curr.getCurrProb() - curr.getNextProb());

            // set p_next to 0.0
            curr.setNextProb(0.0);
            //curr.print();
            cm.put(i, curr);

            //Vertex curr2 = cm.get(i);
            //curr2.print();
        }
        //std::cout
        //    << "Error: " << error
        //    << " prev_error: " << prev_error
        //    << std::endl;

        if (EQUAL(error, prev_error)) {
            break;
        }
    }
}

} // namespace graphs
