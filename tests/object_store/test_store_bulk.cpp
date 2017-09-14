#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/BladeClient.h"

/**
  * This code tests the correctness of bulk reads and writes
  * into the Cirrus object store
  */

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";
const char *IP;
bool use_rdma_client;

void test_read_bulk() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<cirrus::ObjectID> serializer;

    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::ObjectID> store(
            IP, PORT,
            client.get(),
            serializer,
            cirrus::deserializer_simple<cirrus::ObjectID,
                                        sizeof(cirrus::ObjectID)>);

    std::vector<cirrus::ObjectID> ids;
    for (cirrus::ObjectID oid = 0; oid < 100; oid++) {
        ids.push_back(oid);
    }

    store.put_bulk(0, 100, ids.data());

    cirrus::ObjectID* mem = new cirrus::ObjectID[ids.size()];
    memset(mem, 0, ids.size() * sizeof(cirrus::ObjectID));

    cirrus::TimerFunction tf;
    store.get_bulk(0, 100, mem);
    std::cout << "Bulk-get slow elapsed (us): "
        << tf.getUsElapsed() << std::endl;

    for (cirrus::ObjectID i = 0; i < ids.size(); ++i) {
        if (mem[i] != i) {
            throw std::runtime_error("Wrong data received with get_bulk");
        }
    }
    delete[] mem;

    tf.reset();
    auto data = store.get_bulk_fast(ids);
    std::cout << "Bulk-get fast elapsed (us): "
        << tf.getUsElapsed() << std::endl;

    for (cirrus::ObjectID i = 0; i < ids.size(); ++i) {
        if (data[i] != i) {
            throw std::runtime_error("Wrong data received with get_bulk2");
        }
    }
}

void test_write_bulk() {
    std::unique_ptr<cirrus::BladeClient> client =
        cirrus::test_internal::GetClient(use_rdma_client);
    cirrus::serializer_simple<cirrus::ObjectID> serializer;

    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::ObjectID> store(
            IP, PORT,
            client.get(),
            serializer,
            cirrus::deserializer_simple<cirrus::ObjectID,
                                        sizeof(cirrus::ObjectID)>);

    uint64_t num_objs = 100;

    // build data to store with Cirrus
    std::vector<cirrus::ObjectID> ids;
    std::vector<cirrus::ObjectID> data;
    for (cirrus::ObjectID oid = 0; oid < num_objs; oid++) {
        ids.push_back(oid);
        data.push_back(oid);
    }

    cirrus::TimerFunction tf;
    store.put_bulk(0, num_objs, data.data());
    std::cout << "Bulk-put slow elapsed (us): "
        << tf.getUsElapsed() << std::endl;

    // Check that put went well
    cirrus::ObjectID* mem = new cirrus::ObjectID[ids.size()];
    memset(mem, 0, ids.size() * sizeof(cirrus::ObjectID));
    store.get_bulk(0, num_objs, mem);
    for (cirrus::ObjectID i = 0; i < ids.size(); ++i) {
        if (mem[i] != i) {
            throw std::runtime_error("Wrong data received with put_bulk");
        }
    }

    tf.reset();
    // change data
    std::transform(data.begin(), data.end(), data.begin(),
            [](const cirrus::ObjectID &o) { return 2 * o; });

    // do the actual bulk put
    store.put_bulk_fast(ids, data);
    std::cout << "Bulk-put fast elapsed (us): "
        << tf.getUsElapsed() << std::endl;

    // check the bulk put went well
    store.get_bulk(0, num_objs, mem);
    for (cirrus::ObjectID i = 0; i < ids.size(); ++i) {
        if (mem[i] != 2 * i) {
            std::cout << mem[i] << " " << 2 * i << std::endl;
            throw std::runtime_error("Wrong data written with put_bulk_fast");
        }
    }

    delete[] mem;
}

auto main(int argc, char *argv[]) -> int {
    use_rdma_client = cirrus::test_internal::ParseMode(argc, argv);
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Test starting" << std::endl;
    test_read_bulk();
    test_write_bulk();
    std::cout << "Test successful" << std::endl;
    return 0;
}
