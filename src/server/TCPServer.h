#ifndef SRC_SERVER_TCPSERVER_H_
#define SRC_SERVER_TCPSERVER_H_

#include <poll.h>
#include <vector>
#include <map>
#include "server/Server.h"

namespace cirrus {

using ObjectID = uint64_t;
/**
  * This class serves as a remote store that allows connection from
  * clients over TCP.
  * Clients can make requests for reading and writing data.
  */
class TCPServer : public Server {
 public:
    explicit TCPServer(int port, uint64_t pool_size_);
    ~TCPServer() = default;

    virtual void init();

    virtual void loop();

 private:
    bool process(int sock);

    ssize_t send_all(int, const void*, size_t, int);

    bool read_from_client(std::vector<char>&, int, int&);

    /** The port that the server is listening on. */
    int port_;
    /** The fd for the socket the server listens for incoming requests on. */
    int server_sock_;
    /** The map the server uses to map ObjectIDs to byte vectors. */
    std::map<uint64_t, std::vector<int8_t>> store;

    /** Maximum number of bytes that can be stored in the pool. */
    uint64_t pool_size;

    /** Number of bytes currently in the pool. */
    uint64_t curr_size = 0;

    /** Max number of sockets open at once. */
    // TODO(TYLER): Enforce this limit, or remove it and allow scaling
    uint64_t num_fds = 100;

    /**
     * Index that the next socket accepted should have in the
     * array of struct pollfds.
     */
    uint64_t curr_index = 0;
    /**
     * How long the client will wait during a call to poll() before
     * timing out.
     */
    int timeout = 60 * 1000 * 3;
    /**
     * Vector that serves as a wrapper for a c style array containing
     * struct pollfd objects. Used for calls to poll().
     */
    std::vector<struct pollfd> fds = std::vector<struct pollfd>(num_fds);
};

}  // namespace cirrus

#endif  // SRC_SERVER_TCPSERVER_H_
