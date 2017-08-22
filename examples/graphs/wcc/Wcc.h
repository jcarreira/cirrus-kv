#ifndef WEAKLY_CC_H
#define WEAKLY_CC_H

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

}

#endif
