#include <stdlib.h>
#include <limits>
#include <functional>
#include <queue>

#include "Vertex.h"

#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"


namespace graphs {

void deleteKCoreNeighbor(int id, std::set<int> neighbors);

void k_cores(cirrus::CacheManager<Vertex>& cm, unsigned int num_vertices);


} // namespace graphs
