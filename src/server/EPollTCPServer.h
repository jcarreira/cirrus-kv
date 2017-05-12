/* Copyright 2017 Joao Carreira */

#ifndef _EPOLLTCP_SERVER_H_
#define _EPOLLTCP_SERVER_H_

#include <sys/epoll.h>

namespace sirius {

/**
  * This class serves as a remote store that allows connection from clients through TCP
  * clients can make requests for reading and writing data
  * This class uses the epoll() mechanism to handle large numbers of simultaneous connections
  */
class EPollTCPServer {
public:
    EPollTCPServer(int port, int queue_len_ = 100);
    ~EPollTCPServer();

    /**
      * initializes the server
      */
    virtual void init();

    /**
      * Makes the server enter loop mode.
      * Within loop() the server starts serving requests
      */
    virtual void loop();
private:
    /**
      * Process request from socket
      */
    void process(int socket);
    /**
      * Configure socket to be non blocking
      */
    void make_non_blocking(int sock);
    
    epoll_event* events;
    epoll_event ev;
    int max_conn_;
    int listener;
    int queue_len_;
    int fdmax_;
    int port_;
    int epfd_;
};

} // namespace sirius

#endif // _EPOLLTCP_SERVER_H_
