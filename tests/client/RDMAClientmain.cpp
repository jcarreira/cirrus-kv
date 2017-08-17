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
    std::string to_send("CIRRUS_DDC");

    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient client1;
    client1.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    client1.write_sync(0, to_send.c_str(), to_send.size());

    auto ret_pair = client1.read_sync(0);

    if (strncmp(ret_pair.first.get(), to_send.c_str(), to_send.size()))
        throw std::runtime_error("Error in test");
}

/**
 * Tests that two clients can function at once without overwriting one
 * another.
 */
void test_2_clients() {
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

    auto ret_pair = client1.read_sync(0);
    cirrus::LOG<cirrus::INFO>("Received data 1: ", ret_pair.first.get());

    // Check that client 1 receives the desired string
    if (strncmp(ret_pair.first.get(), oss.str().c_str(), oss.str().size()))
        throw std::runtime_error("Error in test");

    auto ret_pair2 = client2.read_sync(0);
    cirrus::LOG<cirrus::INFO>("Received data 2: ", ret_pair2.first.get());

    // Check that client2 receives "data2"
    if (strncmp(ret_pair2.first.get(), message.c_str(), message.size()))
        throw std::runtime_error("Error in test");
}

/**
 * Test proper performance when writing large objects
 */
void test_performance() {
    cirrus::RDMAClient client;
    client.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    uint64_t mem_size = 150 * MB;

    char* data = new char[mem_size];
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
        auto ret_pair = client.read_sync(0);
        std::cout << "Length: " << ret_pair.second << std::endl;
        std::cout << "first byte: " << *(ret_pair.first.get()) << std::endl;
        if (*(ret_pair.first.get()) != 'Y') {
            throw std::runtime_error("Returned value does not match");
        }
    }

    delete[] data;
}

/**
 * Simple test verifying that basic asynchronous put/get works as intended.
 */
void test_async() {
    cirrus::RDMAClient client;
    client.connect(IP, PORT);

    int message = 42;
    auto future = client.write_async(1, &message, sizeof(int));
    std::cout << "write async complete" << std::endl;

    if (!future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    auto read_future = client.read_async(1);

    if (!read_future.get()) {
        throw std::runtime_error("Error during async write.");
    }
    auto ret_pair = read_future.getDataPair();

    const int ret_val = *(reinterpret_cast<const int*>(ret_pair.first.get()));
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
    for (i = 0; i < N; i++) {
        auto pair = client.read_sync(i);
        const int val = *(reinterpret_cast<const int*>(pair.first.get()));

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
        const int ret_value = *(reinterpret_cast<const int*>(
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
