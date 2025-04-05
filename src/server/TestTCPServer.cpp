#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include "common/Exception.h"
#include "utils/logging.h"

namespace cirrus {

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



ssize_t read_all(int sock, void* data, size_t len) {
    uint64_t bytes_read = 0;

    while (bytes_read < len) {
        int64_t retval = read(sock, reinterpret_cast<char*>(data) + bytes_read,
            len - bytes_read);

        if (retval == -1) {
            char *error = strerror(errno);
            LOG<ERROR>(error);
            throw cirrus::Exception("Error reading from client");
        }

        bytes_read += retval;
    }

    return bytes_read;
}

void loop() {
    int server_sock_;
    int port_ = 12345;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    server_sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_ < 0) {
        throw cirrus::ConnectionException("Server error creating socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_);

    LOG<INFO>("Created socket in TCPServer");

    int opt = 1;
    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        switch (errno) {
            case EBADF:
                LOG<ERROR>("EBADF");
                break;
            case ENOTSOCK:
                LOG<ERROR>("ENOTSOCK");
                break;
            case ENOPROTOOPT:
                LOG<ERROR>("ENOPROTOOPT");
                break;
            case EFAULT:
                LOG<ERROR>("EFAULT");
                break;
            case EDOM:
                LOG<ERROR>("EDOM");
                break;
        }
        throw cirrus::ConnectionException("Error forcing port binding");
    }
    //
    // if (setsockopt(server_sock_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
    //     throw cirrus::ConnectionException("Error setting socket options.");
    // }

    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        switch (errno) {
            case EBADF:
                LOG<ERROR>("EBADF");
                break;
            case ENOTSOCK:
                LOG<ERROR>("ENOTSOCK");
                break;
            case ENOPROTOOPT:
                LOG<ERROR>("ENOPROTOOPT");
                break;
            case EFAULT:
                LOG<ERROR>("EFAULT");
                break;
            case EDOM:
                LOG<ERROR>("EDOM");
                break;
        }
        throw cirrus::ConnectionException("Error forcing port binding");
    }

    int ret = bind(server_sock_, reinterpret_cast<sockaddr*>(&serv_addr),
            sizeof(serv_addr));
    if (ret < 0) {
        throw cirrus::ConnectionException("Error binding in port "
               + to_string(port_));
    }

    // SOMAXCONN is the "max reasonable backlog size" defined in socket.h
    if (listen(server_sock_, SOMAXCONN) == -1) {
        throw cirrus::ConnectionException("Error listening on port "
            + to_string(port_));
    }

    int sock = accept(server_sock_,
            reinterpret_cast<struct sockaddr*>(&cli_addr),
            &clilen);
    if (sock < 0) {
        throw std::runtime_error("Error accepting socket");
    }
    std::vector<char> buf(4096);

    while (1) {
        read_all(sock, buf.data(), 4);
        read_all(sock, buf.data(), 4096);
        send_all(sock, buf.data(), 132, 0);
    }
}

}

auto main() -> int {
    cirrus::loop();
    return 0;
}
