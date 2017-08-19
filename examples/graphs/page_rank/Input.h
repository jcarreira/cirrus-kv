#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include "Vertex.h"

namespace graphs {

std::vector<Vertex> readGraph(const std::string& fname);

}  // graphs

#endif  // _UTILS_H_
