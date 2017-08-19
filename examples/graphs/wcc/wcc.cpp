#include "Wcc.h"

#include <time.h>
#include <vector>
#include <iostream>

#include "Input.h"
#include "Vertex.h"
#include "object_store/FullBladeObjectStore.h"
#include "client/TCPClient.h"
#include "cache_manager/CacheManager.h"
#include "cache_manager/LRAddedEvictionPolicy.h"

using namespace graphs;

const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
const int cache_size = 200;

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("./test_wcc <input_file>");
    }

    std::vector<Vertex> vertices = readGraph(argv[1]);

    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<graphs::Vertex>
        vertex_store(IP, PORT, &client,
                Vertex::serializer,
                Vertex::deserializer);

    cirrus::LRAddedEvictionPolicy policy(cache_size);
    cirrus::CacheManager<Vertex> cm(&vertex_store, &policy, cache_size);

    std::cout << "Adding to cm" << std::endl;
    for (uint64_t i = 0; i < vertices.size(); i++) {
        cm.put(vertices[i].getId(), vertices[i]);
    }

    auto output = weakly_cc(cm, vertices.size());

    for (unsigned int i = 0; i < vertices.size(); i++) {
        std::cout << "Vertex ID ";
        std::cout << i;
        std::cout << ": ";
        std::cout << output[i] << std::endl;
    }
}
