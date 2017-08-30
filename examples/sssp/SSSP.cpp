#include "SSSP.h"
#include <stdlib.h>
#include <limits>
#include <functional>
#include <queue>

#include "Vertex.h"

#include "cache_manager/CacheManager.h"
#include "iterator/CirrusIterable.h"


namespace graphs {

void SSSP(cirrus::CacheManager<Vertex>& cm, unsigned int num_vertices,
        unsigned int start) {
    auto cmp = [](int v1, int v2)
        { return cm.get(v1).getDist() < cm.get(v2).getDist(); };
    // fringe
    std::priority_queue<int, std::vector<int>, decltype(cmp)> pq(cmp);

    pq.push(start);

    while (!pq.empty()) {
        int v_num = pq.top();
        pq.pop();
        Vertex v = cm.get(v_num);
        for (Vertex n : v.getNeighbors()) {
            if (true) { // if not in the fringe
                pq.push(n.getId());
            } else if (!n.seen) {
                double newDist = v.getDist() + v.getDistToNeighbor(n.getId());
                if (newDist < n.getDist()) {
                    n.setDist(newDist);
                    n.setPrev(v_num);
                }
                pq.push(n.getId());
            }
        }
        v.seen = true;
    }

}

} // namespace graphs
