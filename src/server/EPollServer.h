/* Copyright 2017 Joao Carreira */

#ifndef _EPOLLTCP_SERVER_H_
#define _EPOLLTCP_SERVER_H_

namespace sirius {

/**
  * This class serves as a remote store that allows connection from clients through TCP
  * clients can make requests for reading and writing data
  * This class uses the epoll() mechanism to handle large numbers of simultaneous connections
  */
class EPollTCPServer {
public:
    EPollTCPServer(int port);
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
};

} // namespace sirius

