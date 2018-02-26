#ifndef PS_SPARSE_SERVER_INTERFACE_H_
#define PS_SPARSE_SERVER_INTERFACE_H_

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
#include "SparseLRModel.h"
#include "SparseMFModel.h"

class PSSparseServerInterface {
 public:
  PSSparseServerInterface(const std::string& ip, int port);

  void send_lr_gradient(const LRSparseGradient&);
  void send_mf_gradient(const MFSparseGradient&);
  
  //void get_model(SparseLRModel& model);
  SparseLRModel get_full_model();
  SparseLRModel get_lr_sparse_model(const SparseDataset& ds);
  SparseMFModel get_sparse_mf_model(const SparseDataset& ds, uint32_t, uint32_t);

 private:
  std::string ip;
  int port;
  int sock;
};

#endif //  PS_SPARSE_SERVER_INTERFACE_H_
