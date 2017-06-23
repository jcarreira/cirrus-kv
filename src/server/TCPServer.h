#ifndef SRC_SERVER_TCPSERVER_H_
#define SRC_SERVER_TCPSERVER_H_

#include <vector>
#include <map>
#include "server/Server.h"

namespace cirrus {

using ObjectID = uint64_t;
/**
  * This class serves as a remote store that allows connection from
  * clients over TCP.
  * Clients can make requests for reading and writing data.
  * WARNING: This class only handles one client at a time.
  */
class TCPServer : public Server {
 public:
    explicit TCPServer(int port, int queue_len = 100);
    ~TCPServer();


    virtual void init();

    virtual void loop();

 private:
    void process(int sock);

    /** The port that the server is listening on. */
    int port_;
    /** The number of pending connection requests allowed at once. */
    int queue_len_;
    /** The fd for the socket the server listens for incoming requests on. */
    int server_sock_;
    /** The map the server uses to map ObjectIDs to byte vectors. */
    std::map<uint64_t, std::vector<int8_t>> store;
};

}  // namespace cirrus

#endif  // SRC_SERVER_TCPSERVER_H_
