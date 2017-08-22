#include "Input.h"
#include <fstream>

namespace graphs {

/** Graphs must be in the format:
  number of vertices
  number of edges
  number of edges for vertex 0 (n)
  edge 0 for vertex 0
  edge 1 for vertex 0
  ...
  edge (n-1) for vertex 0
  number of edges for vertex 1 (m)
  ...
 **/
std::vector<Vertex> readGraph(const std::string& fname) {
    std::ifstream infile(fname.c_str());
    std::vector<Vertex> vertices;

    if (!infile.is_open()) {
        throw std::runtime_error("Error opening file: " + fname);
    }

    int num_vertices = -1;
    int num_edges = -1;
    infile >> num_vertices;
    infile >> num_edges;

    vertices.resize(num_vertices);

    for (int j = 0; j < num_vertices; ++j) {
        int num_edges_per_vertex = -1;
        infile >> num_edges_per_vertex;

        Vertex v(j);  // new vertex

        // read neighbors for new vertex
        for (int i = 0; i < num_edges_per_vertex; ++i) {
            int neigh_id;
            infile >> neigh_id;
            v.addNeighbor(neigh_id);
        }
        vertices.push_back(v);
    }
    return vertices;
}

}  // namespace graphs
