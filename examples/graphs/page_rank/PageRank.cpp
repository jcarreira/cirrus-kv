#include <examples/graphs/page_rank/PageRank.h>
#include <stdlib.h>
#include <cmath>

#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"

#define EPS (1e-05)
#define EQUAL(a, b) (fabs(a - b) < EPS)

namespace graphs {

/**
  * Compute PageRank on graph stored in a Cirrus store
  * Preconditions:
  * a) vertices are stored in the range 0..num_vertices
  * b) Current probability of vertices is initialized to 1.0 and next to 0.0
  * @param cm CacheManager to be used to access the graph
  * @param num_vertices Number of vertices in the graph
  * @param gamma PageRanks's algorithm gamma
  * @param num_iterations Number of iterations
  */
void pageRank(cirrus::CacheManager<Vertex>& cm,
        unsigned int num_vertices,
        double gamma, uint64_t num_iterations) {
    double error = 0;

    for (uint64_t it = 0; it < num_iterations; ++it) {
        double prev_error = error;

        cirrus::CirrusIterable<Vertex> iter(&cm, 40, 0, num_vertices - 1);
        for (const auto& curr : iter) {
            auto neighbors = curr.getNeighbors();
            for (auto it = neighbors.begin();
                    it != neighbors.end();
                    ++it) {
                Vertex n = cm.get(*it);

                n.setNextProb(n.getNextProb() +
                        curr.getCurrProb() / neighbors.size());
                cm.put(*it, n);
            }
        }

        // use iterator here
        iter = cirrus::CirrusIterable<Vertex>(&cm, 40, 0, num_vertices - 1);
        uint64_t i = 0;
        for (auto curr : iter) {
            prev_error = error;

            // update p_curr for next round
            curr.setCurrProb(curr.getNextProb() * gamma + (1.0 - gamma) /
                    static_cast<double>(num_vertices));

            error += std::abs(curr.getCurrProb() - curr.getNextProb());

            // set p_next to 0.0
            curr.setNextProb(0.0);
            cm.put(i++, curr);
        }

        if (EQUAL(error, prev_error)) {
            break;
        }
    }
}

}  // namespace graphs
