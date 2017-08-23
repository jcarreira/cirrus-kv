#include "PageRank.h"

#include <time.h>
#include <vector>
#include <iostream>

#include "Input.h"
#include "Vertex.h"
#include "utils/Log.h"
#include "object_store/FullBladeObjectStore.h"
#include "client/TCPClient.h"
#include "cache_manager/CacheManager.h"
#include "cache_manager/LRAddedEvictionPolicy.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint32_t SIZE = 128;
static const uint64_t MILLION = 1000000;
static const uint64_t N_ITER = 100;
const int cache_size = 200;

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("./pr <input_file>");
    }

    // read vertices from file
    std::vector<graphs::Vertex> vertices = graphs::readGraph(argv[1]);
    cirrus::LOG<cirrus::INFO>("Read ", vertices.size(), " vertices");

    graphs::VertexSerializer vs;
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<graphs::Vertex>
        vertex_store(IP, PORT, &client,
                vs, graphs::Vertex::deserializer);

    cirrus::LRAddedEvictionPolicy policy(cache_size);
    cirrus::CacheManager<graphs::Vertex> cm(&vertex_store, &policy, cache_size);

    std::cout << "Adding to cm" << std::endl;
    for (uint64_t i = 0; i < vertices.size(); i++) {
        std::cout << "Adding vertex: " << i << std::endl;
        cm.put(vertices[i].getId(), vertices[i]);
    }

    std::cout << "Running PageRank" << std::endl;
    graphs::pageRank(cm, vertices.size(), 0.85, 100);

    std::cout << "PageRank completed" << std::endl;

    std::cout << "PageRank probabilities size: "
        << vertices.size() << std::endl;
    for (unsigned int i = 0; i < vertices.size(); i++) {
        graphs::Vertex curr = cm.get(i);
        curr.print();
    }
}

