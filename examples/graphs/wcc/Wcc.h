#ifndef EXAMPLES_GRAPHS_WCC_WCC_H_
#define EXAMPLES_GRAPHS_WCC_WCC_H_

#include <algorithm>
#include <vector>
#include <list>
#include <set>
#include <iostream>

#include "Vertex.h"
#include "cache_manager/CacheManager.h"

namespace graphs {

/**
  * Compute the weakly connected components of the graph stored in this object store
  * A weakly connected component is a subset of a graph for which all vertices
  * are weakly connected, i.e., there is a path ignoring the edges direction
  * @param cm Object store where input graph is stored
  * @param size Number of vertices of the graph (vertices in ids 0..size-1)
  * @return Vector of ints containing assignment of each vertex to a CC
  */
std::unique_ptr<int[]> weakly_cc(
        cirrus::CacheManager<Vertex>& cm, unsigned int size);

/**
  * Transform a directed graph into an undirected one
  * @param Set of vertices
  */
void make_undirected(std::vector<Vertex>& vertices);

}  // namespace graphs

#endif  // EXAMPLES_GRAPHS_WCC_WCC_H_
