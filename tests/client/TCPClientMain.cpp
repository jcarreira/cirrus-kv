#include <string>
#include <iostream>
#include "client/TCPClient.h"



/**
 * Simple test verifying that basic put/get works as intended.
 */
// TODO: Remove hardcoded port and ip
auto main() -> int {
    std::string port = "12345";
    std::string ip = "127.0.0.1";
    cirrus::TCPClient client;

    client.connect(ip, port);

    int message = 42;
    client.write_sync(1, &message, sizeof(int));
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
