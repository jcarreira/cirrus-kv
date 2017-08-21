#include <arpa/inet.h>
#include <string>
#include <iostream>
#include "client/TCPClient.h"
#include "tests/object_store/object_store_internal.h"

// TODO(Tyler): Remove hardcoded port
const char port[] = "12345";
const char *IP;

using AtomicType = uint32_t;

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

    const int returned = *(reinterpret_cast<const int*>(ret_ptr.get()));

    std::cout << returned << " returned from server" << std::endl;

    if (returned != message) {
        throw std::runtime_error("Wrong value returned.");
    }
}

/**
 * Simple test verifying that basic atomics work.
 */
void test_atomics() {
    cirrus::TCPClient client;
    client.connect(IP, port);
    std::cout << "Connected to server." << std::endl;
    AtomicType message = 15;
    std::cout << "message declared." << std::endl;
    client.write_sync(1, &message, sizeof(AtomicType));
    std::cout << "write sync complete" << std::endl;

    std::cout << "Exchanging" << std::endl;
    AtomicType retval = client.exchange(1, 23);
    if (retval != message) {
        std::cout << retval << std::endl;
        throw std::runtime_error("Wrong value returned from exchange.");
    }

    auto ptr_pair = client.read_sync(1);
    AtomicType *ret_ptr = reinterpret_cast<AtomicType*>(ptr_pair.first.get());
    std::cout << *ret_ptr << " returned from server" << std::endl;

    if (*ret_ptr != 23) {
        throw std::runtime_error("Wrong value after exchange.");
    }



    AtomicType message2 = 3;
    AtomicType message2_network = htonl(message2);
    std::cout << "message 2 network" << message2_network << std::endl;
    client.write_sync(2, &message2_network, sizeof(AtomicType));

    std::cout << "FetchAdd" << std::endl;
    AtomicType val = 1;
    AtomicType val_network = htonl(val);
    std::cout << "Val network: " << val_network << std::endl;
    AtomicType retval_network = client.fetchAdd(2, val_network);
    std::cout << "retVal network: " << retval_network << std::endl;
    AtomicType retval2 = ntohl(retval_network);
    std::cout << "retval: " << retval2 << std::endl;
    if (retval2 != message2) {
        std::cout << retval2 << std::endl;
        std::cout << "Wrong value returned" << std::endl;
        throw std::runtime_error("Wrong value returned from fetchadd.");
    }

    AtomicType returned_network;
    ptr_pair = client.read_sync(2);
    ret_ptr = reinterpret_cast<AtomicType*>(ptr_pair.first.get());
    std::cout << *ret_ptr << std::endl;
    std::cout << ntohl(*ret_ptr)
        << " returned from server" << std::endl;

    if (ntohl(*ret_ptr) != val + message2) {
        std::cout << ntohl(*ret_ptr) << std::endl;
        throw std::runtime_error("Wrong value returned after fetchadd");
    }
}

/**
 * This test ensures that errors are thrown by the server in all situations
 * where they should be.
 */
void test_atomics_sanity() {
    cirrus::TCPClient client;
    client.connect(IP, port);
    std::cout << "Connected to server." << std::endl;

    // Ensure 8 byte item exists under oid 1
    uint64_t message = 15;
    std::cout << "message declared." << std::endl;
    client.write_sync(1, &message, sizeof(uint64_t));
    std::cout << "write sync complete" << std::endl;

    std::cout << "Exchanging" << std::endl;

    try {
        AtomicType retval = client.exchange(1, 23);
        std::cout << "Exception not thrown when attempting exchange "
            "on object of incorrect type." << std::endl;
        throw std::runtime_error("No exception when object of wrong type.");
    } catch (const cirrus::Exception& e) {
    }

    try {
        AtomicType retval = client.exchange(1500, 23);
        std::cout << "Exception not thrown when attempting exchange "
            "on nonexistent object." << std::endl;
        throw std::runtime_error("No exception when "
            "exchanging nonexistent id.");
    } catch (const cirrus::NoSuchIDException& e) {
    }

    try {
        AtomicType retval = client.fetchAdd(1, 23);
        std::cout << "Exception not thrown when attempting fetchadd "
            "on object of incorrect type." << std::endl;
        throw std::runtime_error("No exception when object of wrong type.");
    } catch (const cirrus::Exception& e) {
    }

    try {
        AtomicType retval = client.fetchAdd(1500, 23);
        std::cout << "Exception not thrown when attempting fetchAdd "
            "on nonexistent object." << std::endl;
        throw std::runtime_error("No exception when "
            "calling fetchAdd on nonexistent id.");
    } catch (const cirrus::NoSuchIDException& e) {
    }
}

auto main(int argc, char *argv[]) -> int {
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Test Starting." << std::endl;
    test_sync();
    test_async();
    test_atomics();
    test_atomics_sanity();
    std::cout << "Test successful." << std::endl;
    return 0;
}
