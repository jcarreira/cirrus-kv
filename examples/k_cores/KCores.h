#ifndef K_CORES_H
#define K_CORES_H

#include <vector>
#include "Vertex.h"
//#include "cache_manager/CacheManager.h"

namespace graphs {

/**
 * Returns the k-core of the inputted graph.
 * @param cm CacheManager used to access Vertex objects
 * @param num_vertices Number of vertices in the graph
 * @param k The maximum degree of a vertex in the ending graph
 */

void k_cores(cirrus::CacheManager<Vertex>& cm, unsigned int num_vertices);
    int k = 1;
    std::set<int> processed;
    while (seen.size() != num_vertices) {
        
} // namespace graphs

#endif
