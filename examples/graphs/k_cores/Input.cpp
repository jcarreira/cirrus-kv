#include <examples/graphs/k_cores/Input.h>
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
    int num_edges_per_vertex = -1;
    infile >> num_edges_per_vertex;

    for (int j = 0; j < num_vertices; ++j) {
        Vertex v(j);  // new vertex
        
	// read neighbors for new vertex
        for (int i = 0; i < num_edges_per_vertex; ++i) {
            int neigh_id;
            infile >> neigh_id;
            v.addNeighbor(neigh_id);
        }
        vertices.push_back(v);
    }

    for (Vertex v : vertices) {
        std::cout << "Vertex ID: " << v.getId() << std::endl;
	std::cout << "Neighbors:" << std::endl;
	for (int i : v.getNeighbors()) {
	   std::cout << i << std::endl;
	}
    }

    return vertices;
}

}  // namespace graphs
