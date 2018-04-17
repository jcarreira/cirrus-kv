#include <cassert>
#include "PSSparseServerInterface.h"
#include "MultiplePSInterface.h"
#include "Constants.h"
#include <memory>

#undef DEBUG

#define MAX_MSG_SIZE (1024*1024)

MultiplePSInterface::MultiplePSInterface(const Configuration& config) {
  this->num_servers = config.get_num_ps();
  this->minibatch_fraction = config.get_minibatch_size() / this->num_servers;
  for (int i = 0; i < this->num_servers; i++) {
    psint.push_back(std::make_shared<PSSparseServerInterface>(config.get_ps_ip(i), config.get_ps_port(i)));
  }
}

void MultiplePSInterface::send_gradient(const LRSparseGradient* gradient) {
  // need to generalize to arbitrary num of servers
  std::vector<std::shared_ptr<LRSparseGradient>> split_model = gradient->gradient_shards(num_servers);
  for (int i = 0; i < num_servers; i++)
    psint[i]->send_lr_gradient(*split_model[i]);
}

void MultiplePSInterface::send_mf_gradient(const MFSparseGradient* gradient) {
  // need to generalize to arbitrary num of servers
  std::vector<std::shared_ptr<MFSparseGradient>> split_model = gradient->gradient_shards(num_servers);
  for (int i = 0; i < num_servers; i++)
    psint[i]->send_mf_gradient(*split_model[i]);
}


SparseLRModel MultiplePSInterface::get_lr_sparse_model(const SparseDataset& ds, const Configuration& config) {
  // Initialize variables
  SparseLRModel model(0);
  //std::unique_ptr<CirrusModel> model = std::make_unique<SparseLRModel>(0);
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
      //std::cout << "[converted] " << w.first << " to " << data_index << std::endl;
      store_value<uint32_t>(msg_lst[server_index], data_index);
      num_weights_lst[server_index]++;
    }
  }

  // we get the model subset with just the right amount of weights
  for (int i = 0; i < config.get_num_ps(); i++) {
    psint[i]->get_lr_sparse_model_inplace_sharded(model, config, msg_begin_lst[i], num_weights_lst[i], i);
  }

  for (int i = 0; i < this->num_servers; i++) {
    delete[] msg_begin_lst[i];
  }


  delete[] msg_begin_lst;
  delete[] msg_lst;
  delete[] num_weights_lst;
  return model;
}

//XXX: Adapt this code!!
SparseMFModel MultiplePSInterface::get_mf_sparse_model(const SparseDataset& ds, const Configuration& config, uint64_t user_base) {
  // Initialize variables
  int nfactors = 10;
  SparseMFModel model((uint64_t) 0, 0, 0);
  //std::unique_ptr<CirrusModel> model = std::make_unique<SparseLRModel>(0);
  // we don't know the number of weights to start with

  // Initialize the messages
  // XXX: Rename these weights things
  char** msg_lst = new char*[this->num_servers];
  char** msg_begin_lst = new char*[this->num_servers];
  uint32_t* num_items_lst = new uint32_t[this->num_servers];
  for (int i = 0; i < this->num_servers; i++) {
    msg_lst[i] = new char[MAX_MSG_SIZE];
    msg_begin_lst[i] = msg_lst[i];
    num_items_lst[i] = 0;
    store_value<uint32_t>(msg_lst[i], num_items_lst[i]); // just make space for the number of weights
    store_value<uint32_t>(msg_lst[i], user_base);
    store_value<uint32_t>(msg_lst[i], this->minibatch_fraction);
    store_value<uint32_t>(msg_lst[i], MAGIC_NUMBER);
  }

  // Split the dataset based on which server data belongs to.
  // First we split the movies
  // XXX consider optimizing this
  for (const auto& sample : ds.data_) {
    for (const auto& w : sample) {
      int server_index = w.first % this->num_servers;
      int data_index = (w.first - server_index) / num_servers;
      //std::cout << "[converted] " << w.first << " to " << data_index << std::endl;
      store_value<uint32_t>(msg_lst[server_index], data_index);
      num_items_lst[server_index]++;
    }
  }

  // we get the model subset with just the right amount of weights
  for (int i = 0; i < config.get_num_ps(); i++) {
    msg_lst[i] = msg_begin_lst[i];
    store_value<uint32_t>(msg_lst[i], num_items_lst[i]);
    // Here we split the users into their proper ranges
    psint[i]->get_mf_sparse_model_inplace_sharded(model, config, msg_begin_lst[i], num_items_lst[i], i, this->minibatch_fraction);
  }

  // Cleanup our arrays
  for (int i = 0; i < this->num_servers; i++) {
    delete[] msg_begin_lst[i];
  }
  delete[] msg_begin_lst;
  delete[] msg_lst;
  delete[] num_items_lst;
  return model;
}

std::unique_ptr<CirrusModel> MultiplePSInterface::get_full_model(bool isCollaborative) {
  if (isCollaborative) {

    std::unique_ptr<CirrusModel> model = std::make_unique<MFModel>(0, 0, 0);

    for (int i = 0; i < num_servers; i++) {
      model = psint[i]->get_full_model(isCollaborative, i, num_servers, std::move(model));
    }

    return model;
  } else {
    std::unique_ptr<CirrusModel> model = std::make_unique<SparseLRModel>(0);
    for (int i = 0; i < num_servers; i++) {
      model = psint[i]->get_full_model(isCollaborative, i, num_servers, std::move(model)); //XXX: fix this - Andy
    }
    return model;
  }
}
