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

#define APPLY_GRADIENT_REQ (1)
#define GET_MODEL_REQ (2)
#define GET_FULL_MODEL_REQ (3)
#define REGISTER_WORKER_REQ (4)
#define GET_SERVER_CLOCK_REQ (5)

#define STALE_THRESHOLD (4)

class PSSparseServerInterface {
 public:
  PSSparseServerInterface(const std::string& ip, int port);

  void send_gradient(const LRSparseGradient&);
  
  //void get_model(SparseLRModel& model);
  SparseLRModel get_full_model();
  SparseLRModel get_sparse_model(const SparseDataset& ds);

  void register_worker();

 private:
  SparseLRModel get_sparse_model_aux(const SparseDataset& ds);
  uint64_t get_ps_clock() const;

  std::string ip;
  int port;
  int sock;

  uint64_t worker_clock = 0;
};

#endif //  PS_SPARSE_SERVER_INTERFACE_H_
