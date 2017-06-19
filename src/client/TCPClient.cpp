#include "src/client/TCPClient.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
namespace cirrus {

void TCPClient::connect(std::string address, std::string port_string) {
    // Create socket

    // TODO: Replace with logged errors and exit
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("socket creation error \n \n");
    }
    struct sockaddr_in serv_addr;


    // Set the type of address being used, assuming ip v4
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr);
    // Convert port from string to int
    int port = stoi(port_string, nullptr);

    // Save the port in the info
    serv_addr.sin_port = htons(port);

    // Connect to the server
    // TODO: Replace with logged errors and exit
    if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      printf("could not connect to server\n\n");
    }
}

bool TCPClient::write_sync(ObjectID /*id*/, void*  /*data*/, uint64_t /*size*/) {
    return true;
}

bool TCPClient::read_sync(ObjectID /*id*/, void* /*data*/, uint64_t /*size*/) {
    return true;
}

bool TCPClient::remove(ObjectID /*id*/){
    return true;
}
void TCPClient::test() {
    std::string hello = "Hello from client";
    send(sock, hello.c_str(), hello.length(), 0);
    char buffer[1024] = {0};
    int retval = read(sock, buffer, 1024);
    if (retval < 0) {
        printf("issue in receiving\n");
    }
    printf("%s\n", buffer);
}
}  // namespace cirrus
