#ifndef SRC_CLIENT_TCPCLIENT_CLIENT_H_
#define SRC_CLIENT_TCPCLIENT_CLIENT_H_

#include <string>
#include <thread>

namespace cirrus {

using ObjectID = uint64_t;

class TCPClient {
 public:
    void connect(std::string address, std::string port);
    bool write_sync(ObjectID id, void* data, uint64_t size);
    bool read_sync(ObjectID id, void* data, uint64_t size);
    std::future<bool> write_sync(ObjectID id, void* data, uint64_t size);
    std::future<bool> read_sync(ObjectID id, void* data, uint64_t size);
    bool remove(ObjectID id);
    void test();

 private:
    int sock; /**< fd of the socket used to communicate w/ remote store. */

    std::thread receiver_thread;
    std::thread sender_thread;


    void process_received();
    void process_send();
};

}  // namespace cirrus

#endif  // SRC_CLIENT_TCPCLIENT_H_
