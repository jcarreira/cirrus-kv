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
void test_simple() {
    cirrus::TCPClient client;
    client.connect(IP, port);
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

/**
 * Simple test verifying that basic atomics work.
 */
void test_atomics() {
    cirrus::TCPClient client;
    client.connect(IP, port);
    std::cout << "Connected to server." << std::endl;
    AtomicType message = 42;
    std::cout << "message declared." << std::endl;
    client.write_sync(1, &message, sizeof(AtomicType));
    std::cout << "write sync complete" << std::endl;

    AtomicType val_network = htonl(23);
    std::cout << "Exchanging" << std::endl;
    AtomicType retval_network = client.exchange(1, val_network);
    AtomicType retval = ntohl(retval_network);
    if (retval != 42) {
        std::cout << retval << std::endl;
        throw std::runtime_error("Wrong value returned from exchange.");
    }

    std::cout << "FetchAdd" << std::endl;
    val_network = htonl(1);
    retval_network = client.fetchAdd(1, val_network);
    retval = ntohl(retval_network);
    if (retval != 24) {
        std::cout << retval << std::endl;
        throw std::runtime_error("Wrong value returned from fetchadd.");
    }
}

auto main(int argc, char *argv[]) -> int {
    IP = cirrus::test_internal::ParseIP(argc, argv);
    std::cout << "Test Starting." << std::endl;
    test_simple();
    test_atomics();
    std::cout << "Test successful." << std::endl;
}
