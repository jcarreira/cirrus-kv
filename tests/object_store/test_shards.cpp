#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/BladeClient.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t MILLION = 1000000;
static const uint64_t BILLION = MILLION * 1000;
static const uint64_t GB = 1024 * 1024 * 1024;
bool use_rdma_client;

/**
  * Test the correctness of multiple shards
  * We assume all the servers are running in 127.0.01
  * with IPs in the range 12345 ... 12345 + num_shards
  */
void test_store_shards(uint64_t num_shards) {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<cirrus::ObjectID> serializer;

    std::vector<std::string> ips;
    std::vector<std::string> ports;

    for (uint64_t i = 0; i < num_shards; ++i) {
        ips.push_back("127.0.0.1");
        ports.push_back(std::to_string(12345 + i));
    }

    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::ObjectID> store(
       ips, ports,
       client.get(),
       serializer,
       cirrus::deserializer_simple<cirrus::ObjectID, sizeof(cirrus::ObjectID)>);

    srand(time(NULL));

    std::vector<cirrus::ObjectID> oids;
    for (uint64_t i = 0; i < 10000; i++) {
        cirrus::ObjectID oid = rand() % BILLION;
        oids.push_back(oid);
        store.put(oid, oid);
    }

    for (const auto& oid : oids) {
        cirrus::ObjectID retval = store.get(oid);
        if (retval != oid) {
            throw std::runtime_error("Wrong value returned.");
        }
    }
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    char* num_shards = cirrus::test_internal::ParseNumShards(argc, argv);
    std::cout << "Test starting" << std::endl;

    if (num_shards == nullptr) {
        throw std::runtime_error("Wrong num shards");
    }
    std::cout << "num_shards: " << num_shards << std::endl;
    test_store_shards(std::stoi(num_shards));
    std::cout << "Test successful" << std::endl;
    return 0;
}
