#include <string>
#include <iostream>
#include "client/TCPClient.h"

// TODO(Tyler): Remove hardcoded port and ip
const char port[] = "12345";
const char ip[] = "127.0.0.1";

/**
 * Simple test verifying that basic put/get works as intended.
 */
void test_sync() {
    cirrus::TCPClient client;
    client.connect(ip, port);

    int message = 42;
    client.write_sync(1, &message, sizeof(int));
    std::cout << "write sync complete" << std::endl;

    int returned;
    client.read_sync(1, &returned, sizeof(int));
    std::cout << returned << " returned from server" << std::endl;

    if (returned != message) {
        throw std::runtime_error("Wrong value returned.");
    }
}

/**
 * Simple test verifying that basic asynchronous put/get works as intended.
 */
void test_async() {
    cirrus::TCPClient client;
    client.connect(ip, port);

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
    std::cout << "Test starting" << std::endl;
    test_sync();
    test_async();
    std::cout << "Test Successful." << std::endl;
    return 0;
}
