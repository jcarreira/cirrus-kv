#include <string>
#include <iostream>
#include "client/TCPClient.h"
#include "tests/object_store/object_store_internal.h"

// TODO(Tyler): Remove hardcoded port
const char port[] = "12345";
char *ip;

/**
 * Simple test verifying that basic put/get works as intended.
 */
void test_simple() {
    cirrus::TCPClient client;
    client.connect(ip, port);
    std::cout << "Connected to server." << std::endl;
    int message = 42;
    std::cout << "message declared." << std::endl;
    client.write_sync(1, &message, sizeof(int));
    std::cout << "write sync complete" << std::endl;

    int returned;
    client.read_sync(1, &returned, sizeof(int));
    std::cout << returned << " returned from server" << std::endl;

    if (returned != message) {
        throw std::runtime_error("Wrong value returned.");
    }
}

auto main(int argc, char *argv[]) -> int {
    ip = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Test Starting." << std::endl;
    test_simple();
    std::cout << "Test successful." << std::endl;
}
