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
void test_sync() {
    cirrus::TCPClient client;
    client.connect(IP, port);
    std::cout << "Connected to server." << std::endl;

    int message = 42;
    std::cout << "message declared." << std::endl;
    client.write_sync(1, &message, sizeof(int));
    std::cout << "write sync complete" << std::endl;

    auto ptr_pair = client.read_sync(1);
    int *int_ptr = reinterpret_cast<int*>(ptr_pair.first.get());
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
    client.connect(IP, port);

    int message = 42;
    auto future = client.write_async(1, &message, sizeof(int));
    std::cout << "write sync complete" << std::endl;

    if (!future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    auto read_future = client.read_async(1);

    if (!read_future.get()) {
        throw std::runtime_error("Error during async write.");
    }

    auto ret_ptr = read_future.getDataPair().first;

    int returned = *(reinterpret_cast<int*>(ret_ptr.get()));

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
    cirrus::TCPClient client;
    client.connect(IP, port);
    std::vector<cirrus::BladeClient::ClientFuture> put_futures;
    std::vector<cirrus::BladeClient::ClientFuture> get_futures;
    client.open_additional_cxns(3);
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
    std::cout << "Test Starting." << std::endl;
    test_sync();
    test_async();
    test_async_N<100>();
    std::cout << "Test successful." << std::endl;
    return 0;
}
