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
    std::cout << "Num vertices: " << num_vertices << std::endl;
    auto cmp = [&cm](int v1, int v2)
        { return cm.get(v1).getDist() < cm.get(v2).getDist(); };

    // fringe
    std::priority_queue<int, std::vector<int>, decltype(cmp)> pq(cmp);
    Vertex v = cm.get(start);
    v.setDist(0);
    cm.put(start, v);
    pq.push(start);

    while (!pq.empty()) {
        int v_num = pq.top();
        pq.pop();
        v = cm.get(v_num);
        for (int neigh_id : v.getNeighbors()) {
	    Vertex n = cm.get(neigh_id);
            if (n.getOnFringe() == 0) { // if not in the fringe
                pq.push(n.getId());
		n.setOnFringe(1);
		n.setDist(v.getDist() + v.getDistToNeighbor(n.getId()));
		n.setPrev(v.getId());
		cm.put(neigh_id, n);
            } else if (n.getProcessed() == 0) { // if vertex hasn't been seen
		double newDist = v.getDist() + v.getDistToNeighbor(n.getId());
                if (newDist < n.getDist()) {
		    n.setDist(newDist);
                    n.setPrev(v_num);
		    cm.put(neigh_id, n);
                }
                pq.push(n.getId());
            }
        }
        v.setProcessed(1);
	cm.put(v_num, v);
    }

}

} // namespace graphs
