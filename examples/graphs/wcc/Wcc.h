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

void bfs(int cc, const std::vector<Vertex> &vertices, std::set<int> &seen,
                std::list<int> &fringe, int* &ccs);
void make_undirected(std::vector<Vertex>& vertices);
std::unique_ptr<int[]> weakly_cc(
        cirrus::CacheManager<Vertex>& cm, unsigned int size);

}

#endif
