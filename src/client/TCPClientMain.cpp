#include "client/TCPClient.h" // TODO FIX THIS
#include <string>

auto main() -> int {
    std::string port = "12345";
    std::string ip = "10.10.49.83";
    cirrus::TCPClient client;
    client.connect(ip, port);
    int message = 42;
    client.write_sync(1, &message, sizeof(int));
    int returned;
    client.read_sync(1, &returned, sizeof(int));
    printf("%d returned from server\n", returned);
    return 0;
}
