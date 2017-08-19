#include "Wcc.h"

namespace graphs {

/** Traverse all the vertices that are weakly connected
  * XXX We should use DFS
  */
void bfs(int cc, cirrus::CacheManager<Vertex>& cm,
        std::list<int> &fringe,
        const std::unique_ptr<int[]>& ccs) {
    while (!fringe.empty()) {
        int ID = fringe.front();
        fringe.pop_front();

        Vertex curr = cm.get(ID);
        curr.seen = true;

        curr.cc = cc;
        ccs[ID] = cc;

        if (!curr.getNeighbors().empty()) {
            for (const auto& v : curr.getNeighbors()) {
                auto nei = cm.get(v);
                if (nei.seen == false) {
                    fringe.push_back(v);
                }
            }
        }
    }
}

void make_undirected(std::vector<Vertex>& vertices) {
    // make directed edges undirected
    for (auto& curr : vertices) {
        for (auto& v : curr.getNeighbors()) {
            Vertex n = vertices[v];

            // check if node v's neighbour doesn't have v as neighbour
            if (!n.hasNeighbor(curr.getId())) {
                n.addNeighbor(curr.getId());
            }
        }
    }
}

/** Weakly connected components
  * A weakly connected component is a subset of a graph for which all vertices
  * are weakly connected, i.e., there is a path ignoring the edges direction
  * @param vertices Graph vertices
  * @param num-vertices Number of vertices
  * @return Returns a vector containing component id for each vertex
  */
std::unique_ptr<int[]> weakly_cc(cirrus::CacheManager<Vertex>& cm,
        unsigned int num_vertices) {
    // vertices have to be 0-indexed, maybe can use a map instead

    std::list<int> fringe;

    std::unique_ptr<int[]> ccs(new int[num_vertices]);
    std::fill(ccs.get(), ccs.get() + sizeof(int) * num_vertices, -1);

    int cc = 1;
    for (unsigned int i = 0; i < num_vertices; i ++) {
        Vertex curr = cm.get(i);
        if (!curr.seen) {
            fringe.push_back(i);
            bfs(cc, cm, fringe, ccs);
            cc++;
        }
    }

    return ccs;
}


}  // namespace graphs
