#include "server/TCPServer.h"

const int port = 12345;
const int queue_len = 10;

/* Start a server that runs and prints messages received. */
auto main() -> int {
    TCPServer server(port, queue_len);
    server.init();
    server.loop();
    return 0;
}
