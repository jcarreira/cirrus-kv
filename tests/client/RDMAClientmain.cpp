#include <unistd.h>
#include <iostream>
#include <csignal>
#include <memory>
#include <sstream>
#include <cstring>
#include <string>
#include "client/BladeClient.h"
#include "common/AllocationRecord.h"
#include "utils/logging.h"
#include "authentication/AuthenticationToken.h"
#include "utils/CirrusTime.h"
#include "client/RDMAClient.h"
#include "tests/object_store/object_store_internal.h"

// TODO(Tyler): Remove hardcoded IP and PORT

const char PORT[] = "12345";
static const uint64_t MB = (1024*1024);
static const uint64_t GB = (1024*MB);
static const char IP[] = "10.10.49.84";


/**
 * Tests that simple get and put work with integers.
 */
void test_1_client() {
    int to_send = 42;

    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient<int> client1;
    cirrus::serializer_simple<int> serializer;
    client1.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    client1.write_sync(0, to_send, serializer);

    int ret_val;
    client1.read_sync(0, &ret_val, sizeof(to_send));

    if (ret_val != to_send)
        throw std::runtime_error("Incorrect value returned");
}

/**
 * Tests that two clients can function at once without overwriting one
 * another.
 */
void test_2_clients() {
    int data = 0;

    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient<int> client1, client2;
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
        client1.write_sync(0, random, serializer);
    }
    int data2 = 1442;
    client2.write_sync(0, data2, serializer);

    cirrus::LOG<cirrus::INFO>("Old data: ", data);
    client1.read_sync(0, &data, sizeof(random));
    cirrus::LOG<cirrus::INFO>("Received data 1: ", data);

    // Check that client 1 receives the desired random value
    if (data != random)
        throw std::runtime_error("Error in test");

    client2.read_sync(0, &data, sizeof(data2));
    cirrus::LOG<cirrus::INFO>("Received data 2: ", data);

    // Check that client2 receives "data2"
    if (data != data2)
        throw std::runtime_error("Error in test");
}

/**
 * Test proper performance when writing large objects, in this case
 * a std::array
 */
void test_performance() {
    const int size = 199 * MB;
    cirrus::RDMAClient<cirrus::Dummy<size>> client;
    cirrus::serializer_simple<cirrus::Dummy<size>> serializer;
    client.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    std::unique_ptr<cirrus::Dummy<size>> d =
        std::make_unique<cirrus::Dummy<size>>(42);
    std::unique_ptr<cirrus::Dummy<size>> d2 =
        std::make_unique<cirrus::Dummy<size>>(0);
    {
        cirrus::TimerFunction tf("Timing write", true);
        client.write_sync(0, *d, serializer);
    }

    {
        cirrus::TimerFunction tf("Timing read", true);
        client.read_sync(0, d2.get(), sizeof(*d));
        std::cout << "Returned id: " << d2->id << std::endl;
        if (d2->id != 42) {
            throw std::runtime_error("Returned value does not match");
        }
    }
}

auto main() -> int {
    test_1_client();
    test_2_clients();
    test_performance();
    return 0;
}
