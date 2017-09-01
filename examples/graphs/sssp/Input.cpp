#include <examples/graphs/sssp/Input.h>
#include <fstream>

namespace graphs {

/** Graphs must be in the format:
  number of vertices
  number of edges
  number of edges per vertex
  edge for vertex 0
  ...
  edge for vertex n - 1
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

    vertices.reserve(num_vertices);

    for (int j = 0; j < num_vertices; ++j) {
        int num_edges_per_vertex = -1;
        infile >> num_edges_per_vertex;

        Vertex v(j);  // new vertex

        // read neighbors for new vertex
        for (int i = 0; i < num_edges_per_vertex; ++i) {
            int neigh_id;
            infile >> neigh_id;
	    double neigh_dist;
	    infile >> neigh_dist;
            v.addNeighbor(neigh_id, neigh_dist);
        }
        vertices.push_back(v);
    }
    return vertices;
}

}  // namespace graphs
