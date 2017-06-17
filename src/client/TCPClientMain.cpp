#include "client/TCPClient.h"

const char PORT[] = "12345";

static const char IP[] = "10.10.49.83";

/**
  * Just send a message to the server
  */
void test_simple() {
    const char* to_send = "CIRRUS_DDC";

    snprintf(data, sizeof(data), "%s", "WRONG");

    cirrus::LOG<cirrus::INFO>("Connecting to server", IP, " in port: ", PORT);

    cirrus::BladeClient client1;
    client1.connect(IP, PORT);

    cirrus::LOG<cirrus::INFO>("Connected to blade");

    cirrus::AllocationRecord alloc1 = client1.allocate(10);


}

auto main() -> int {
    cirrus::TCPClient client;
    client.connect(ip, port);
    client.test();
    return 0;
}
