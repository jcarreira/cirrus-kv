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
const char *IP;


/**
 * Tests that simple get and put work with a string.
 */
void test_1_client() {
    char data[1000];
    std::string to_send("CIRRUS_DDC");

    snprintf(data, sizeof(data), "%s", "WRONG");

    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient client1;
    client1.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    client1.write_sync(0, to_send.c_str(), to_send.size());

    client1.read_sync(0, data, to_send.size());

    if (strncmp(data, to_send.c_str(), to_send.size()))
        throw std::runtime_error("Error in test");
}

/**
 * Tests that two clients can function at once without overwriting one
 * another.
 */
void test_2_clients() {
    char data[1000];
    snprintf(data, sizeof(data), "%s", "WRONG");

    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient client1, client2;

    client1.connect(IP, PORT);
    client2.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    unsigned int seed = 42;
    std::ostringstream oss;
    {
        oss << "data" << rand_r(&seed);
        cirrus::LOG<cirrus::INFO>("Writing ", oss.str().c_str());
        cirrus::TimerFunction tf("client1.write");
        client1.write_sync(0, oss.str().c_str(), oss.str().size());
    }
    std::string message("data2");
    client2.write_sync(0, message.c_str(), message.size());

    cirrus::LOG<cirrus::INFO>("Old data: ", data);
    client1.read_sync(0, data, oss.str().size());
    cirrus::LOG<cirrus::INFO>("Received data 1: ", data);

    // Check that client 2 receives the desired string
    if (strncmp(data, oss.str().c_str(), oss.str().size()))
        throw std::runtime_error("Error in test");

    client2.read_sync(0, data, message.size());
    cirrus::LOG<cirrus::INFO>("Received data 2: ", data);

    // Check that client2 receives "data2"
    if (strncmp(data, message.c_str(), message.size()))
        throw std::runtime_error("Error in test");
}

/**
 * Test proper performance when writing large objects
 */
void test_performance() {
    cirrus::RDMAClient client;
    client.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    uint64_t mem_size = .25 * GB;

    char* data = reinterpret_cast<char*>(malloc(mem_size));
    if (!data)
        exit(-1);

    memset(data, 0, mem_size);
    data[0] = 'Y';

    {
        cirrus::TimerFunction tf("Timing write", true);
        client.write_sync(0, data, mem_size);
    }

    {
        cirrus::TimerFunction tf("Timing read", true);
        client.read_sync(0, data, mem_size);
        std::cout << "data[0]: " << data[0] << std::endl;
        if (data[0] != 'Y') {
            throw std::runtime_error("Returned value does not match");
        }
    }

    free(data);
}

/**
 * Simple test verifying that basic asynchronous put/get works as intended.
 */
void test_async() {
    cirrus::RDMAClient client;
    client.connect(IP, PORT);

    int message = 42;
    auto future = client.write_async(1, &message, sizeof(int));
    std::cout << "write sync complete" << std::endl;

    if (!future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    int returned;
    auto read_future = client.read_async(1, &returned, sizeof(int));

    if (!read_future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    std::cout << returned << " returned from server" << std::endl;

    if (returned != message) {
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
    client.connect(IP, PORT);
    std::vector<cirrus::BladeClient::ClientFuture> put_futures;
    std::vector<cirrus::BladeClient::ClientFuture> get_futures;

    int i;
    for (i = 0; i < N; i++) {
        int val = i;
        put_futures.push_back(client.write_async(i, &val, sizeof(int)));
    }
    // Check the success of each put operation
    for (i = 0; i < N; i++) {
        if (!put_futures[i].get()) {
            throw std::runtime_error("Error during an async put.");
        }
    }
    std::cout << "BEGINNING READS" << std::endl;
    int ret_values[10];
    for (i = 0; i < N; i++) {
        int val;
        client.read_sync(i, &val, sizeof(int));
        if (val != i) {
            std::cout << "Expected " << i << "but got " << val << std::endl;
            throw std::runtime_error("Wrong value returned test_async_N");
        }
    }

    // for (i = 0; i < N; i++) {
    //     get_futures.push_back(client.read_async(i, &ret_values[i],
    //         sizeof(int)));
    // }
    // // check the value of each get
    // for (i = 0; i < N; i++) {
    //     bool success = get_futures[i].get();
    //     if (!success) {
    //         throw std::runtime_error("Error during an async read");
    //     }
    //     if (ret_values[i] != i) {
    //         std::cout << "Expected " << i << " but got " << ret_values[i]
    //             << std::endl;
    //         throw std::runtime_error("Wrong value returned in test_async_N");
    //     }
    // }
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
