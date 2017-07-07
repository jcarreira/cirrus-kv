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
#include "utils/Time.h"
#include "client/RDMAClient.h"
// TODO(Tyler): Remove hardcoded IP and PORT

const char PORT[] = "12345";
static const uint64_t MB = (1024*1024);
static const uint64_t GB = (1024*MB);
static const char IP[] = "10.10.49.83";


/**
 * Tests that simple get and put work with a string.
 */
void test_1_client() {
    char data[1000];
    const char* to_send = "CIRRUS_DDC";

    snprintf(data, sizeof(data), "%s", "WRONG");

    cirrus::LOG<cirrus::INFO>("Connecting to server in port: ", PORT);

    cirrus::RDMAClient client1;
    client1.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    client1.write_sync(0, to_send, std::strlen(to_send));

    client1.read_sync(0, data, std::strlen(to_send));

    if (strncmp(data, to_send, std::strlen(to_send)))
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

    client2.write_sync(0, "data2", 5);

    cirrus::LOG<cirrus::INFO>("Old data: ", data);
    client1.read_sync(0, data, oss.str().size());
    cirrus::LOG<cirrus::INFO>("Received data 1: ", data);

    // Check that client 2 receives the desired string
    if (strncmp(data, oss.str().c_str(), oss.str().size()))
        throw std::runtime_error("Error in test");

    client2.read_sync(0, data, 5);
    cirrus::LOG<cirrus::INFO>("Received data 2: ", data);

    // Check that client2 receives "data2"
    if (strncmp(data, "data2", 5))
        throw std::runtime_error("Error in test");
}

/**
 * Test proper performance when writing large objects
 */
void test_performance() {
    cirrus::RDMAClient client;
    client.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    uint64_t mem_size = 1 * GB;

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

auto main() -> int {
    test_1_client();
    test_2_clients();
    test_performance();
    test_async();
    return 0;
}
