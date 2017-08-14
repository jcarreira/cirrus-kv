#include <unistd.h>
#include <iostream>
#include <csignal>
#include <memory>
#include <sstream>
#include <cstring>
#include <string>

#include "client/BladeClient.h"
#include "common/AllocationRecord.h"
#include "common/Serializer.h"
#include "utils/logging.h"
#include "authentication/AuthenticationToken.h"
#include "utils/CirrusTime.h"
#include "client/RDMAClient.h"
#include "tests/object_store/object_store_internal.h"

// TODO(Tyler): Remove hardcoded IP and PORT
const char PORT[] = "12345";
const char *IP;
static const uint64_t MB = (1024*1024);
static const uint64_t GB = (1024*MB);


/**
 * Tests that simple get and put work with integers.
 */
void test_1_client() {
    int to_send = 42;

    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient client1;
    cirrus::serializer_simple<int> serializer;
    client1.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");
    cirrus::WriteUnitTemplate<int> w(serializer, to_send);
    client1.write_sync(0, w);

    auto ret_pair = client1.read_sync(0);
    int ret_val = *(reinterpret_cast<int*>(ret_pair.first.get()));

    if (ret_val != to_send)
        throw std::runtime_error("Incorrect value returned");
}

/**
 * Tests that two clients can function at once without overwriting one
 * another.
 */
void test_2_clients() {
    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient client1, client2;
    cirrus::serializer_simple<int> serializer;

    client1.connect(IP, PORT);
    client2.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    unsigned int seed = 42;
    int random;
    {
        random = rand_r(&seed);
        cirrus::LOG<cirrus::INFO>("Writing ", random);
        cirrus::TimerFunction tf("client1.write");
        cirrus::WriteUnitTemplate<int> w(serializer, random);
        client1.write_sync(0, w);
    }
    int data2 = 1442;
    cirrus::WriteUnitTemplate<int> w(serializer, data2);
    client2.write_sync(0, w);

    auto ret_pair = client1.read_sync(0);
    int ret_val = *(reinterpret_cast<int*>(ret_pair.first.get()));
    
    cirrus::LOG<cirrus::INFO>("Received data 1: ", ret_val);

    // Check that client 1 receives the desired random value
    if (ret_val != random)
        throw std::runtime_error("Error in test");

    ret_pair = client2.read_sync(0);
    ret_val = *(reinterpret_cast<int*>(ret_pair.first.get()));
    cirrus::LOG<cirrus::INFO>("Received data 2: ", ret_val);

    // Check that client2 receives "data2"
    if (ret_val != data2)
        throw std::runtime_error("Error in test");
}

/**
 * Test proper performance when writing large objects, in this case
 * a std::array
 */
void test_performance() {
    const int size = 199 * MB;
    cirrus::RDMAClient client;
    cirrus::serializer_simple<cirrus::Dummy<size>> serializer;
    client.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    std::unique_ptr<cirrus::Dummy<size>> d =
        std::make_unique<cirrus::Dummy<size>>(42);

    {
        cirrus::TimerFunction tf("Timing write", true);
        cirrus::WriteUnitTemplate<cirrus::Dummy<size>> w(serializer, *d);
        client.write_sync(0, w);
    }

    {
        cirrus::TimerFunction tf("Timing read", true);
        auto ret_pair = client.read_sync(0);

        std::cout << "Length: " << ret_pair.second << std::endl;

        auto d2 =
            *(reinterpret_cast<cirrus::Dummy<size>*>(ret_pair.first.get()));

        std::cout << "Returned id: " << d2.id << std::endl;
        if (d2.id != 42) {
            throw std::runtime_error("Returned value does not match");
        }
    }
}

/**
 * Simple test verifying that basic asynchronous put/get works as intended.
 */
void test_async() {
    cirrus::RDMAClient client;
    cirrus::serializer_simple<int> serializer;
    client.connect(IP, PORT);

    int message = 42;
    cirrus::WriteUnitTemplate<int> w(serializer, message);
    auto future = client.write_async(1, w);
    std::cout << "write async complete" << std::endl;

    if (!future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    auto read_future = client.read_async(1);

    if (!read_future.get()) {
        throw std::runtime_error("Error during async write.");
    }
    auto ret_pair = read_future.getDataPair();

    int ret_val = *(reinterpret_cast<int*>(ret_pair.first.get()));
    std::cout << ret_val << " returned from server" << std::endl;

    if (ret_val != message) {
        throw std::runtime_error("Wrong value returned.");
    }
}

/**
 * Tests multiple concurrent puts and gets. It first puts N items, then ensures
 * all N were successful. It then gets N items, and ensures each value matches.
 * @param N the number of puts and gets to perform.
 */
template <int N>
void test_async_N() {
    cirrus::RDMAClient client;
    cirrus::serializer_simple<int> serializer;
    client.connect(IP, PORT);
    std::vector<cirrus::BladeClient::ClientFuture> put_futures;
    std::vector<cirrus::BladeClient::ClientFuture> get_futures;

    int i;
    for (i = 0; i < N; i++) {
        int val = i;
        cirrus::WriteUnitTemplate<int> w(serializer, val);
        put_futures.push_back(client.write_async(i, w));
    }
    // Check the success of each put operation
    for (i = 0; i < N; i++) {
        if (!put_futures[i].get()) {
            throw std::runtime_error("Error during an async put.");
        }
    }
    std::cout << "BEGINNING READS" << std::endl;
    for (i = 0; i < N; i++) {
        auto pair = client.read_sync(i);
        int val = *(reinterpret_cast<int*>(pair.first.get()));

        if (val != i) {
            std::cout << "Expected " << i << "but got " << val << std::endl;
            throw std::runtime_error("Wrong value returned test_async_N");
        }
    }

    for (i = 0; i < N; i++) {
        get_futures.push_back(client.read_async(i));
    }
    // check the value of each get
    for (i = 0; i < N; i++) {
        bool success = get_futures[i].get();
        if (!success) {
            throw std::runtime_error("Error during an async read");
        }
        int ret_value = *(reinterpret_cast<int*>(
            get_futures[i].getDataPair().first.get()));

        if (ret_value != i) {
            std::cout << "Expected " << i << " but got " << ret_value
                << std::endl;
            throw std::runtime_error("Wrong value returned in test_async_N");
        }
    }
}

auto main(int argc, char *argv[]) -> int {
    IP = cirrus::test_internal::ParseIP(argc, argv);
    test_1_client();
    test_2_clients();
    test_performance();
    test_async();
    test_async_N<10>();
    return 0;
}
