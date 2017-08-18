#include "PageRank.h"
#include "cache_manager/CacheManager.h"

#include <stdlib.h>
#include <cmath>

namespace graphs {

double calculate_error(const std::vector<double>& p_curr,
        const std::vector<double>& p_next) {
    double error = 0.0;
    for (unsigned int i = 0; i < p_curr.size(); i++) {
        error += std::abs(p_curr[i] - p_next[i]);
    }
    return error;
}

std::vector<double> pageRank(cirrus::CacheManager<Vertex> cm,
        const std::vector<Vertex>& vertices,
        double gamma, double epsilon) {

    unsigned int num_vertices = vertices.size();
    std::vector<double> p_curr(num_vertices);
    std::vector<double> p_next(num_vertices);

    for (unsigned int i = 0; i < num_vertices; i++) {
        p_curr[i] = 1.0 / num_vertices;
        p_next[i] = 0.0;
    }

    double error = 1.0;

    int count = 1;
    while (error > epsilon) {
        std::cout << "PageRank iteration: " << count++ << std::endl;

        for (unsigned int i = 0; i < num_vertices; i++) {
            Vertex curr = cm.get(i);

            auto neighbors = curr.getNeighbors();
            for (auto& neighbor : neighbors) {
                p_next[neighbor] += p_curr[i] / neighbors.size();
            }
        }

        for (unsigned int i = 0; i < num_vertices; i++) {
            p_next[i] = p_next[i] * gamma + (1.0 - gamma) /
                static_cast<double>(num_vertices);
        }

        error = calculate_error(p_curr, p_next);

        for (unsigned int i = 0; i < p_curr.size(); i++) {
            p_curr[i] = p_next[i];
            p_next[i] = 0.0;

            std::cout << "p[" << i << "] = " << p_curr[i] << std::endl;
        }
    }
    return p_curr;

}

} // namespace graphs
