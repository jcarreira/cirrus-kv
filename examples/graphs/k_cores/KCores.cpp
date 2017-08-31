#ifndef K_CORES_H
#define K_CORES_H

#include <vector>
#include "Vertex.h"

#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"

namespace graphs {

/**
 * For all of the neighbors n, deletes id from n's list of neighbors
 * @param cm CacheManager used to access Vertex objects
 * @param id the vertex that should be deleted from neighbors' neighbors
 * @param neighbors set of vertices that id should be deleted from 
 */

void deleteKCoreNeighbor(cirrus::CacheManager<Vertex>& cm,
        int id, std::set<int> neighbors) {
    for (int n : neighbors) {
        Vertex v = cm.get(n);
        v.deleteTempNeighbor(id);
    }
}
/**
 * Returns the k-core of the inputted graph. Graphs must be undirected.
 * @param cm CacheManager used to access Vertex objects
 * @param num_vertices Number of vertices in the graph
 * @param k The maximum degree of a vertex in the ending graph
 */

void k_cores(cirrus::CacheManager<Vertex>& cm, unsigned int num_vertices) {
    int k = 1;
    std::set<int> processed;
    while (processed.size() != num_vertices) {
        cirrus::CirrusIterable<Vertex> iter(&cm, 40, 0, num_vertices - 1);
        for (const auto& curr : iter) {
            if (!curr.getSeen() && curr.getTempNeighbors().size() < k) {
                curr.setK(k);
                processed.insert(curr.getId());
                curr.setSeen(true);
                deleteKCoreNeighbor(curr.getId(), curr.getNeighbors());
            }
        }
        k++;
    }
}
} // namespace graphs

#endif
