#include "server/TCPServer.h"

const int port = 12344;
const int queue_len = 10;

/* Start a server that runs and prints messages received. */
auto main() -> int {
    cirrus::TCPServer server(port, queue_len);
    server.init();
    server.loop();
    return 0;
}
