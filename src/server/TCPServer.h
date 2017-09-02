#ifndef SRC_SERVER_TCPSERVER_H_
#define SRC_SERVER_TCPSERVER_H_

#include <poll.h>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include "server/Server.h"
#include "server/MemoryBackend.h"

namespace cirrus {

using ObjectID = uint64_t;
/**
  * This class serves as a remote store that allows connection from
  * clients over TCP.
  * Clients can make requests for reading and writing data.
  */
class TCPServer : public Server {
 public:
    explicit TCPServer(
            int port, uint64_t pool_size_,
            const std::string& backend = "Memory",
            const std::string& storage_path = "/tmp/cirrus_storage/",
            uint64_t max_fds = 100);
    ~TCPServer() = default;

    virtual void init();

    virtual void loop();

 private:
    bool process(int sock);

    ssize_t send_all(int, const void*, size_t, int);
    ssize_t read_all(int sock, void*, size_t len);
    bool read_from_client(std::vector<char>&, int, uint64_t&);

    bool testRemove(struct pollfd x);

    /** The port that the server is listening on. */
    int port_;
    /** The fd for the socket the server listens for incoming requests on. */
    int server_sock_ = 0;
    /** The map the server uses to map ObjectIDs to byte vectors. */
    std::map<uint64_t, std::vector<int8_t>> store;

    /** Maximum number of bytes that can be stored in the pool. */
    uint64_t pool_size;

    /** Number of bytes currently in the pool. */
    uint64_t curr_size = 0;

    /** Max number of sockets open at once. */
    const uint64_t max_fds;

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
     * struct pollfd objects. Used for calls to poll(). New structs are
     * always inserted to the end of a continus range, indicated by curr_index.
     * When the vector reaches capacity, unused structs are removed and the
     * remaining items shifted.
     */
    std::vector<struct pollfd> fds = std::vector<struct pollfd>(max_fds);

    /**
      * Memory interface
      */
    std::unique_ptr<StorageBackend> mem;
};

}  // namespace cirrus

#endif  // SRC_SERVER_TCPSERVER_H_
