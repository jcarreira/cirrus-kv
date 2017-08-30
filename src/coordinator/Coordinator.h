#ifndef _COORDINATOR_H_
#define _COORDINATOR_H_

#include <cstdint>
#include <string>
#include <vector>
#include <poll.h>

namespace cirrus {

/**
  * A Coordinator is used to coordinate a set
  * of Cirrus tasks in a cluster
  * For now we use it to create and kill containers
  */
class Coordinator {
 public:

     Coordinator(int port, uint64_t max_fds_);
     void init();

     void loop();

 private:
    /**
      * Add a container to the list of containers tracked
      * ip Ip of the physical machine where the container lives
      */
    void addContainer(const std::string& ip);


    /**
      * This method is used in response from a client
      * when it requires more backing store
      * In practice it launches more containers and
      * respective Cirrus memory/storage tasks
      * memory How much memory the application requires
      */
    void requestMemory(uint64_t memory);
    bool testRemove(struct pollfd x);
    ssize_t send_all(int sock, const void* data, size_t len, int /* flags */);
    ssize_t read_all(int sock, void* data, size_t len);
    bool read_from_client(
            std::vector<char>& buffer, int sock, uint64_t& bytes_read);
    bool process(int sock);

    /** The port that the server is listening on. */
    int port_;
    /** The fd for the socket the server listens for incoming requests on. */
    int server_sock_ = 0;
    /** Maximum number of bytes that can be stored in the pool. */
    uint64_t pool_size;

    /** Number of bytes currently in the pool. */
    uint64_t curr_size = 0;

    /** Max number of sockets open at once. */
    const uint64_t max_fds;
    std::vector<struct pollfd> fds = std::vector<struct pollfd>(max_fds);
    
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
};

}  // namespace cirrus

#endif  // _COORDINATOR_H_
