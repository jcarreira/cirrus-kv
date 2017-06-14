#ifndef _SERVER_H_
#define _SERVER_H_

namespace cirrus {

/**
  * Parent class for all servers.
  */
class Server {
public:
    Server() = default;
    Server(Server&) = delete;
};

} // cirrus

#endif // _SERVER_H_
