#include "PageRank.h"
#include "Input.h"
#include "Vertex.h"
#include "utils/Log.h"

#include <vector>
#include <iostream>
#include <time.h>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
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

using namespace graphs;

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("./pr <input_file>");
    }

    // read vertices from file
    std::vector<Vertex> vertices = graphs::readGraph(argv[1]);
    cirrus::LOG<cirrus::INFO>("Read ", vertices.size(), " vertices");

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<graphs::Vertex>
        vertex_store(IP, PORT, &client,
                Vertex::serializer,
                Vertex::deserializer);


    cirrus::LRAddedEvictionPolicy policy(cache_size);
    std::cout << "Adding to cm" << std::endl;
    cirrus::CacheManager<Vertex> cm(&vertex_store, &policy, cache_size);

    for (uint64_t i = 0; i < vertices.size(); i++) {
        cm.put(vertices[i].getId(), vertices[i]);
    }
    std::vector<double> output = graphs::pageRank(cm, vertices, 0.85, .01);

    std::cout << "PageRank completed" << std::endl;

    //std::cout << "PageRank probabilities: " << std::endl;
    //for (unsigned int i = 0; i < vertices.size(); i++) {
    //    std::cout << output[i] << "\t\t" << output[i] * vertices.size() << std::endl;
    //}

}
