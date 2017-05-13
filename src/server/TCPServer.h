/* Copyright 2016 Joao Carreira */

#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

namespace cirrus {

/**
  * This class serves as a remote store that allows connection from clients through TCP
  * clients can make requests for reading and writing data
  * WARNING: This class only handles one client at a time. For more clients see EpollServer
  */
class TCPServer {
public:
    TCPServer(int port, int queue_len = 100);
    ~TCPServer();

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
      * Server a request from a given socket
      */
    void process(int sock);

    int port_;
    int queue_len_;
    int server_sock_;
};

} // namespace cirrus

#endif // _TCP_SERVER_H_
