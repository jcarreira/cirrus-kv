#ifndef SRC_CLIENT_TCPCLIENT_CLIENT_H_
#define SRC_CLIENT_TCPCLIENT_CLIENT_H_

#include <string>

namespace cirrus {

class TCPClient {
 public:
    void connect(std::string address, std::string port);
    void test();
 private:
    int sock;
};

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
