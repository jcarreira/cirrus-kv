#ifndef PAGE_RANK_H
#define PAGE_RANK_H

#include <vector>
#include "Vertex.h"
#include "cache_manager/CacheManager.h"

namespace graphs {

void pageRank2(cirrus::CacheManager<Vertex> cm,
        unsigned int num_vertices,
        double gamma, double epsilon);

} // namespace graphs

#endif
