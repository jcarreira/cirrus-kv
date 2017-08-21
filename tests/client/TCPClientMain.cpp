#include <string>
#include <iostream>
#include "client/TCPClient.h"
#include "tests/object_store/object_store_internal.h"
#include "common/Serializer.h"

// TODO(Tyler): Remove hardcoded port
const char port[] = "12345";
const char *IP;

/**
 * Simple test verifying that basic put/get works as intended.
 */
void test_sync() {
    cirrus::TCPClient client;
    cirrus::serializer_simple<int> serializer;
    std::cout << "Test Starting." << std::endl;
    client.connect(IP, port);
    std::cout << "Connected to server." << std::endl;
    int message = 42;
    std::cout << "message declared." << std::endl;
    cirrus::WriteUnitTemplate<int> w(serializer, message);
    client.write_sync(1, w);
    std::cout << "write sync complete" << std::endl;

    auto ptr_pair = client.read_sync(1);
    const int *int_ptr = reinterpret_cast<const int*>(ptr_pair.first.get());
    std::cout << *int_ptr << " returned from server" << std::endl;

    if (*int_ptr != message) {
        throw std::runtime_error("Wrong value returned.");
    }
}

/**
 * Simple test verifying that basic asynchronous put/get works as intended.
 */
void test_async() {
    cirrus::TCPClient client;
    cirrus::serializer_simple<int> serializer;
    client.connect(IP, port);

    int message = 42;
    cirrus::WriteUnitTemplate<int> w(serializer, message);
    auto future = client.write_async(1, w);
    std::cout << "write sync complete" << std::endl;

    if (!future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    auto read_future = client.read_async(1);

    if (!read_future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    auto ret_ptr = read_future.getDataPair().first;

    const int returned = *(reinterpret_cast<const int*>(ret_ptr.get()));

    std::cout << returned << " returned from server" << std::endl;

    if (returned != message) {
        throw std::runtime_error("Wrong value returned.");
    }
}

auto main(int argc, char *argv[]) -> int {
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Test Starting." << std::endl;
    test_sync();
    test_async();
    std::cout << "Test successful." << std::endl;
    return 0;
}
