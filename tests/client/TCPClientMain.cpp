#include <string>
#include <iostream>
#include "client/TCPClient.h"
#include "tests/object_store/object_store_internal.h"


/**
 * Simple test verifying that basic put/get works as intended.
 */
// TODO(Tyler): Remove hardcoded port and ip
auto main() -> int {
    std::string port = "12345";
    std::string ip = "127.0.0.1";
    cirrus::TCPClient<int> client;
    cirrus::serializer_simple<int> serializer;
    std::cout << "Test Starting." << std::endl;
    client.connect(ip, port);
    std::cout << "Connected to server." << std::endl;
    int message = 42;
    std::cout << "message declared." << std::endl;
    client.write_sync(1, message, serializer);
    std::cout << "write sync complete" << std::endl;

    int returned;
    client.read_sync(1, &returned, sizeof(int));
    std::cout << returned << " returned from server" << std::endl;

    if (returned == message) {
        return 0;
    } else {
        return -1;
    }
}
