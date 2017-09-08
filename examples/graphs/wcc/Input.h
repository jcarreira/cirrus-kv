#ifndef EXAMPLES_GRAPHS_WCC_INPUT_H_
#define EXAMPLES_GRAPHS_WCC_INPUT_H_

#include <vector>
#include <string>
#include "Vertex.h"

namespace graphs {

std::vector<Vertex> readGraph(const std::string& fname);

}  // namespace graphs

#endif  // EXAMPLES_GRAPHS_WCC_INPUT_H_
