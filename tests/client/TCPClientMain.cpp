#include <string>
#include <iostream>
#include "client/TCPClient.h"
#include "tests/object_store/object_store_internal.h"

// TODO(Tyler): Remove hardcoded port
const char port[] = "12345";
const char *IP;

/**
 * Simple test verifying that basic put/get works as intended.
 */
void test_simple() {
    cirrus::TCPClient client;
    client.connect(IP, port);
    std::cout << "Connected to server." << std::endl;
    int message = 42;
    std::cout << "message declared." << std::endl;
    client.write_sync(1, &message, sizeof(int));
    std::cout << "write sync complete" << std::endl;

    int returned;
    auto ptr = client.read_sync(1);
    int *int_ptr = reinterpret_cast<int*>(ptr.get());
    std::cout << *int_ptr << " returned from server" << std::endl;

    if (*int_ptr != message) {
        throw std::runtime_error("Wrong value returned.");
    }
}

auto main(int argc, char *argv[]) -> int {
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Test Starting." << std::endl;
    test_simple();
    std::cout << "Test successful." << std::endl;
}
