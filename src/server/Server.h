#ifndef SRC_SERVER_SERVER_H_
#define SRC_SERVER_SERVER_H_

namespace cirrus {

/**
  * Parent class for all servers.
  */
class Server {
 public:
    Server() = default;
    Server(Server&) = delete;
};

}  // namespace cirrus

#endif  // SRC_SERVER_SERVER_H_
