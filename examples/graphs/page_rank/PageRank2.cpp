#include "PageRank2.h"
#include "cache_manager/CacheManager.h"

#include <stdlib.h>
#include <cmath>

namespace graphs {

double calculate_error(cirrus::CacheManager<Vertex>& cm,
        unsigned int num_vertices) {
    double error = 0.0;
    for (unsigned int i = 0; i < num_vertices; i++) {
        Vertex curr = cm.get(i);
        error += std::abs(curr.getCurrProb() - curr.getNextProb());
    }

    std::cout << "calculate_error: " << error << std::endl;

    return error;
}

/**
  * Compute PageRank on graph stored in a Cirrus store
  * Preconditions:
  * a) vertices are stored in the range 0..num_vertices
  * b) Current probability of vertices is initialized to 1.0 and next to 0.0
  * c)
  * @param cm CacheManager to be used to access the graph
  * @param num_vertices Number of vertices in the graph
  * @param gamma PageRanks's algorithm gamma
  * @param epsilon PageRank's algorithm epsilon
  */
void pageRank2(cirrus::CacheManager<Vertex> cm,
        unsigned int num_vertices,
        double gamma, double epsilon) {

    double error = 1.0;

    int count = 1;
    while (error > epsilon) {
        std::cout << "PageRank iteration: " << count++ << std::endl;

        for (unsigned int i = 0; i < num_vertices; i++) {
            Vertex curr = cm.get(i);

            auto neighbors = curr.getNeighbors();
            for (auto& neighbor : neighbors) {
                Vertex n = cm.get(neighbor);
                std::cout << "Updating vertex: " << neighbor
                    << " with new value: "
                    << n.getNextProb() + curr.getCurrProb() / neighbors.size()
                    << " getNextProb: " << curr.getNextProb()
                    << " getCurrProb: " << curr.getCurrProb()
                    << " neighbors.size(): " << neighbors.size()
                    << std::endl;
                n.setNextProb(n.getNextProb() +
                        curr.getCurrProb() / neighbors.size());
                cm.put(neighbor, n);
            }
        }

        // use iterator here
        for (unsigned int i = 0; i < num_vertices; i++) {
            Vertex curr = cm.get(i);

            // update p_curr for next round
            curr.setCurrProb(curr.getNextProb() * gamma + (1.0 - gamma) /
                    static_cast<double>(num_vertices));
            std::cout << "New currProb: " << curr.getCurrProb()
                << " nextProb: " << curr.getNextProb()
                << std::endl;

            // set p_next to 0.0
            curr.setNextProb(0.0);
            curr.print();
            cm.put(i, curr);

            Vertex curr2 = cm.get(i);
            curr2.print();
        }

        // Check error once in a while
        if (count % 10 == 0) {
            error = calculate_error(cm, num_vertices);
        }
    }
}

} // namespace graphs
