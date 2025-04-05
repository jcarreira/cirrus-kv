#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string>
#include <vector>
#include <thread>
#include <utility>
#include <algorithm>
#include <memory>
#include <atomic>
#include "utils/logging.h"
#include "utils/Stats.h"
#include "common/Exception.h"



const char* address_ = "127.0.0.1";
const char* port_string_ = "12345";
const int MILLION = 1000000;
const int num_messages = 100000;

namespace cirrus {


ssize_t read_all(int sock, void* data, size_t len) {
    uint64_t bytes_read = 0;

    while (bytes_read < len) {
        int64_t retval = read(sock, reinterpret_cast<char*>(data) + bytes_read,
            len - bytes_read);

        if (retval < 0) {
            throw cirrus::Exception("Error reading from server");
        }

        bytes_read += retval;
    }

    return bytes_read;
}


/**
 * Guarantees that an entire message is sent.
 * @param sock the fd of the socket to send on.
 * @param data a pointer to the data to send.
 * @param len the number of bytes to send.
 * @param flags for the send call.
 * @return the number of bytes sent.
 */
ssize_t send_all(int sock, const void* data, size_t len,
    int /* flags */) {
    uint64_t to_send = len;
    uint64_t total_sent = 0;
    int64_t sent = 0;

    while (total_sent != to_send) {
        sent = send(sock, data, len - total_sent, 0);

        if (sent == -1) {
            throw cirrus::Exception("Server error sending data to client");
        }

        total_sent += sent;

        // Increment the pointer to data we're sending by the amount just sent
        data = static_cast<const char*>(data) + sent;
    }

    return total_sent;
}

void loop() {
    std::string address(address_);
    std::string port_string(port_string_);
    int sock;
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw cirrus::ConnectionException("Error when creating socket.");
    }
    // int opt = 1;
    // if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
    //     throw cirrus::ConnectionException("Error setting socket options.");
    // }

    struct sockaddr_in serv_addr;

    // Set the type of address being used, assuming ip v4
    serv_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr) != 1) {
        throw cirrus::ConnectionException("Address family invalid or invalid "
            "IP address passed in");
    }
    // Convert port from string to int
    int port = stoi(port_string, nullptr);

    // Save the port in the info
    serv_addr.sin_port = htons(port);

    // Connect to the server
    if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        throw cirrus::ConnectionException("Client could "
                                          "not connect to server.");
    }
    std::vector<char> buf(4100);
    cirrus::TimerFunction start;
    for (int i = 0; i < num_messages; i++) {
        send_all(sock, buf.data(), buf.size(), 0);
        read_all(sock, buf.data(), 4);
        read_all(sock, buf.data(), 128);
    }
    uint64_t elapsed_us = start.getUsElapsed();

    std::cout << "msg/s: " << (num_messages * 1.0 / elapsed_us * MILLION)
        << std::endl;
    std::cout << "MB/s: " << (num_messages * 1.0 * 4096/
        (1024 * 1024 * elapsed_us / MILLION)) << std::endl;
}

}



auto main() -> int {
    cirrus::loop();
    return 0;
}
