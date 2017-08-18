#ifndef PAGE_RANK2_H
#define PAGE_RANK2_H

#include <vector>
#include "Vertex.h"
#include "cache_manager/CacheManager.h"

namespace graphs {

std::vector<double> pageRank(cirrus::CacheManager<Vertex> cm,
        const std::vector<Vertex>& vertices,
        double gamma, double epsilon);

} // namespace graphs

#endif
