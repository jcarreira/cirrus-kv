#ifndef PS_SPARSE_SERVER_INTERFACE_WRAPPER_H_
#define PS_SPARSE_SERVER_INTERFACE_WRAPPER_H_

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
#include "PSSparseServerInterface.h"

class MultiplePSInterface {
 public:
  MultiplePSInterface();

  void send_gradient(const LRSparseGradient*);

  //void get_model(SparseLRModel& model);
  std::unique_ptr<CirrusModel> get_full_model();
  SparseLRModel get_lr_sparse_model(const SparseDataset& ds, const Configuration& config);


 private:
  //std::vector<PSSparseServerInterface*> psint;
  PSSparseServerInterface* psint[NUM_PS];
  int num_servers;
};

#endif //  PS_SPARSE_SERVER_INTERFACE_WRAPPER_H_
