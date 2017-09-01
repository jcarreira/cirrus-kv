#ifndef EXAMPLES_GRAPHS_PAGE_RANK_PAGERANK_H_
#define EXAMPLES_GRAPHS_PAGE_RANK_PAGERANK_H_

#include <vector>
#include "Vertex.h"
#include "cache_manager/CacheManager.h"

namespace graphs {

/**
  * Compute the PageRank probs. of a set of nodes stored in a Cirrus store
  * @param cm Cache manager used to access nodes
  * @param num_vertices Number of vertices (0..num_vertices - 1)
  * @param gamma Value of PageRank's gamma (e.g., 0.85)
  * @param num_iterations Number of pageRank iterations
  */
void pageRank(cirrus::CacheManager<Vertex>& cm,
        unsigned int num_vertices,
        double gamma, uint64_t num_iterations);

}  // namespace graphs

#endif  // EXAMPLES_GRAPHS_PAGE_RANK_PAGERANK_H_
