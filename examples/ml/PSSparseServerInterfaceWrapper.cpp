#include <cassert>
#include "PSSparseServerInterface.h"
#include "PSSparseServerInterfaceWrapper.h"

#undef DEBUG

#define MAX_MSG_SIZE (1024*1024)

PSSparseServerInterfaceWrapper::PSSparseServerInterfaceWrapper(const std::string& ip, int port, int num_servers) {
  this->num_servers = num_servers;
  for (int i = 0; i < this->num_servers; i++) { // replace 2 with num_servers
    psint[i] = new PSSparseServerInterface(ip, port + i);
  }
}

void PSSparseServerInterfaceWrapper::send_gradient(const LRSparseGradient* gradient) {
  // need to generalize to arbitrary num of servers
  std::vector<LRSparseGradient*> split_model = gradient->shard(2);
  psint[0]->send_gradient(*split_model[0]);
  psint[1]->send_gradient(*split_model[1]);
}


SparseLRModel PSSparseServerInterfaceWrapper::get_sparse_model(const SparseDataset& ds) {
  // Initialize variables
  SparseLRModel model(0);

  // we don't know the number of weights to start with
  char** msg_lst = new char*[this->num_servers];
  char** msg_begin_lst = new char*[this->num_servers];
  uint32_t* num_weights_lst = new uint32_t[this->num_servers];
  for (int i = 0; i < this->num_servers; i++) {
    msg_lst[i] = new char[MAX_MSG_SIZE];
    msg_begin_lst[i] = msg_lst[i];
    num_weights_lst[i] = 0;
    store_value<uint32_t>(msg_lst[i], num_weights_lst[i]); // just make space for the number of weights
  }


  // Split the dataset based on which server data belongs to.
  // XXX consider optimizing this
  for (const auto& sample : ds.data_) {
    for (const auto& w : sample) {
      int server_index = w.first % this->num_servers;
      int data_index = (w.first - server_index) / num_servers;
      store_value<uint32_t>(msg_lst[server_index], data_index);
      num_weights_lst[server_index]++;
    }
  }

  // we get the model subset with just the right amount of weights
  for (int i = 0; i < 2; i++) {
    psint[i]->get_sparse_model(msg_begin_lst[i], num_weights_lst[i], model, i, num_servers);
  }

  for (int i = 0; i < this->num_servers; i++) {
    delete[] msg_begin_lst[i];
  }


  delete[] msg_begin_lst;
  delete[] msg_lst;
  delete[] num_weights_lst;
  return model;
}


SparseLRModel PSSparseServerInterfaceWrapper::get_full_model() {
  SparseLRModel model(0);

  // placeholder for now NOT CORRECT
  psint[0]->get_full_model(model, 0, 2);
  psint[1]->get_full_model(model, 1, 2);

  return model;
}
