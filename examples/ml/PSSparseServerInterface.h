#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include "ModelGradient.h"
#include "Utils.h"

class PSSparseServerInterface {
 public:
  PSSparseServerInterface(const std::string& ip, int port);

  void send_gradient(const LRSparseGradient&);

 private:
  std::string ip;
  int port;
  int sock;
};

PSSparseServerInterface::PSSparseServerInterface(const std::string& ip, int port) :
  ip(ip), port(port) {

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    throw std::runtime_error("Error when creating socket.");
  }   
  int opt = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
    throw std::runtime_error("Error setting socket options.");
  }   

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) != 1) {
    throw std::runtime_error("Address family invalid or invalid "
        "IP address passed in");
  }   
  // Save the port in the info
  serv_addr.sin_port = htons(port);
  std::memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));

  // Connect to the server
  if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    throw std::runtime_error(
        "Client could not connect to server."
        " Address: " + ip + " port: " + std::to_string(port));
  }
}

void PSSparseServerInterface::send_gradient(const LRSparseGradient& gradient) {
  uint32_t operation = 1;
  int ret = send(sock, &operation, sizeof(uint32_t), 0);
  //std::cout << "Sent ret: " << ret << std::endl;

  uint32_t size = gradient.getSerializedSize();
  //std::cout << "Sending grad size: " << size << std::endl;
  ret = send(sock, &size, sizeof(uint32_t), 0);
  //std::cout << "Sent ret: " << ret << std::endl;
  char data[size];
  gradient.serialize(data);
  ret = send(sock, data, size, 0);
  (void)ret;
  //std::cout << "ret: " << ret << std::endl;
}

