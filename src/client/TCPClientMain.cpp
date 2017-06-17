#include "src/client/TCPClient.h" // TODO FIX THIS

const char por[] = "12345";

static const char ip[]= "10.10.49.83";

auto main() -> int {
    cirrus::TCPClient client;
    client.connect(ip, port);
    client.test();
    return 0;
}
