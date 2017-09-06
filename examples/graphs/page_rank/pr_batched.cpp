#include "PageRank.h"

#include <time.h>
#include <vector>
#include <iostream>
#include <memory>

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
static const int batch_size = 100;

typedef std::shared_ptr<graphs::Vertex> vertex_vector;

/** Format:
  * p_curr (double)
  * p_next (double)
  * id (uint32_t)
  * n neighbors (uint32_t)
  * neighbors list (n * uint32_t)
  */
class VVSerializer : public cirrus::Serializer<vertex_vector> {
 public:
    VVSerializer(uint64_t batch_size) : batch_size(batch_size) {}

    uint64_t size(const vertex_vector& v) const override {
        uint64_t total_size = 0;
        for (uint64_t i = 0; i < batch_size; ++i)
            total_size += vs.size(v.get()[i]);
        return total_size;
    }
    void serialize(const vertex_vector& v, void* mem) const override {
        char* ptr = (char*)mem;
        for (uint64_t i = 0; i < batch_size; ++i) {
            vs.serialize(v.get()[i], (void*)ptr);
            ptr += vs.size(v.get()[i]);
        }
    }
 private:
    uint32_t batch_size;
    graphs::VertexSerializer vs;
};

class VVDeserializer {
 public:
    VVDeserializer(uint64_t batch_size) : batch_size(batch_size) {}

    uint64_t vertex_size(const void* data) {
        const double* double_ptr = reinterpret_cast<const double*>(data);

        double_ptr++;
        double_ptr++;

        const uint32_t* ptr = reinterpret_cast<const uint32_t*>(double_ptr);
        ptr++;
        uint32_t n = ntohl(*ptr);

        return 2 * sizeof(double) + sizeof(uint32_t) * (2 + n);
    }

    vertex_vector operator()(const void* data, unsigned int /* size*/ ) {
        vertex_vector ptr(new graphs::Vertex[batch_size]);

        for (uint64_t i = 0; i < batch_size; ++i) {
            ptr.get()[i] = graphs::Vertex::deserializer(data, vertex_size(data));
        }

        return ptr;
    }
 private:
    uint32_t batch_size;
};

void store_graph(cirrus::CacheManager<vertex_vector>& cm,
        const std::vector<graphs::Vertex>& vertices) {
    std::cout << "Adding to cm" << std::endl;

    uint32_t count = 0;
    for (uint64_t i = 0; i + batch_size <= vertices.size();
            i += batch_size, count++) {
        std::cout << "Adding batch: " << i << std::endl;

        vertex_vector data(new graphs::Vertex[batch_size]);
        for (uint64_t j = 0; j < batch_size; ++j) {
            data.get()[j] = vertices[i + j];
        }

        cm.put(count, data);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("./pr_batched <input_file>");
    }

    // read vertices from file
    std::vector<graphs::Vertex> vertices = graphs::readGraph(argv[1]);
    cirrus::CIRRUS_LOG<cirrus::INFO>("Read ", vertices.size(), " vertices");

    VVSerializer vvs(batch_size);
    VVDeserializer vvds(batch_size);
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<vertex_vector>
        vertex_store(IP, PORT, &client,
                vvs, vvds);

    cirrus::LRAddedEvictionPolicy policy(cache_size);
    cirrus::CacheManager<vertex_vector> cm(&vertex_store, &policy, cache_size);

    store_graph(cm, vertices);

    std::cout << "Running PageRank" << std::endl;
    graphs::pageRank(cm, vertices.size(), 0.85, 100);

    std::cout << "PageRank completed" << std::endl;

    //std::cout << "PageRank probabilities size: "
    //    << vertices.size() << std::endl;
    //for (unsigned int i = 0; i < vertices.size(); i++) {
    //    graphs::Vertex curr = cm.get(i);
    //    curr.print();
    //}
}

