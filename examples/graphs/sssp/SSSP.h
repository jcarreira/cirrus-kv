#ifndef SSSP_H
#define SSSP_H

#include <vector>
#include "Vertex.h"
//#include "cache_manager/CacheManager.h"

namespace graphs {

/**
 * Return the shortest path from a single source using Dijkstra's algorithm.
 * @param cm CacheManager used to access Vertex objects
 * @param start Number of the starting Vertex
 */

void SSSP(cirrus::CacheManager<Vertex>& cm, unsigned int num_vertices, unsigned int start);

} // namespace graphs

#endif
