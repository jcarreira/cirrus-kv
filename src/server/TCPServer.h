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

    /**
      * Initializes the server
      */
    virtual void init();

    /**
      * Makes the server enter loop mode.
      * Within loop() the server starts serving requests
      */
    virtual void loop();

 private:
    /**
      * Process a request from a given socket
      */
    void process(int sock);

    int port_;
    int queue_len_;
    int server_sock_;
    std::map<uint64_t, std::vector<uint8_t>> store;
};

}  // namespace cirrus

#endif  // SRC_SERVER_TCPSERVER_H_
