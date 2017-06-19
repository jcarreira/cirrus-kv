#ifndef SRC_CLIENT_TCPCLIENT_CLIENT_H_
#define SRC_CLIENT_TCPCLIENT_CLIENT_H_

#include <string>

namespace cirrus {

using ObjectID = uint64_t;

class TCPClient {
 public:
    void connect(std::string address, std::string port);
    bool write_sync(ObjectID id, void* data, uint64_t size);
    bool read_sync(ObjectID id, void* data, uint64_t size);
    bool remove(ObjectID id);
    void test();
 private:
    int sock;
};

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
