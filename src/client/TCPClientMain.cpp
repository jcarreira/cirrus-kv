#include "src/client/TCPClient.h" // TODO FIX THIS
#include <string>

auto main() -> int {
    std::string port = "12345";
    std::string ip = "10.10.49.83";
    cirrus::TCPClient client;
    client.connect(ip, port);
    client.test();
    return 0;
}
