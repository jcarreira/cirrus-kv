#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "async.h"
#include "Constants.h"
#include "Checksum.h"

#undef DEBUG

#define MAX_CONNECTIONS (nworkers * 2 + 1) // (2 x # workers + 1)
    
PSSparseServerTaskUDP::PSSparseServerTaskUDP(
    uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
    uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
    uint64_t SAMPLE_BASE, uint64_t START_BASE,
    uint64_t batch_size, uint64_t samples_per_batch,
    uint64_t features_per_sample, uint64_t nworkers,
    uint64_t worker_id) :
  MLTask(MODEL_GRAD_SIZE, MODEL_BASE,
      LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
      batch_size, samples_per_batch, features_per_sample,
      nworkers, worker_id) {
  std::cout << "PSSparseServerTaskUDP is built" << std::endl;

  operation_to_name[0] = "SEND_LR_GRADIENT";
  operation_to_name[1] = "SEND_MF_GRADIENT";
  operation_to_name[2] = "GET_LR_FULL_MODEL";
  operation_to_name[3] = "GET_MF_FULL_MODEL";
  operation_to_name[4] = "GET_LR_SPARSE_MODEL";
  operation_to_name[5] = "GET_MF_SPARSE_MODEL";
  operation_to_name[6] = "SET_TASK_STATUS";
  operation_to_name[7] = "GET_TASK_STATUS";
}

std::shared_ptr<char> PSSparseServerTaskUDP::serialize_lr_model(
    const SparseLRModel& lr_model, uint64_t* model_size) const {
  *model_size = lr_model.getSerializedSize();
  auto d = std::shared_ptr<char>(
      new char[*model_size], std::default_delete<char[]>());
  lr_model.serializeTo(d.get());
  return d;
}

bool PSSparseServerTaskUDP::process_send_mf_gradient(const Request& req, std::vector<char>& thread_buffer) {
  uint32_t incoming_size = req.incoming_size;
#ifdef DEBUG 
  std::cout << "APPLY_GRADIENT_REQ incoming size: " << incoming_size << std::endl;
#endif
  if (incoming_size > thread_buffer.size()) {
    throw std::runtime_error("Not enough buffer");
  }
  if (read_all(req.sock, thread_buffer.data(), incoming_size) == 0) {
    return false;
  }

  MFSparseGradient gradient;
  gradient.loadSerialized(thread_buffer.data());

  model_lock.lock();
#ifdef DEBUG 
  std::cout << "Doing sgd update" << std::endl;
#endif
  mf_model->sgd_update(
      task_config.get_learning_rate(), &gradient);
#ifdef DEBUG 
  std::cout
    << "sgd update done"
    << " checksum: " << mf_model->checksum()
    << std::endl;
#endif
  model_lock.unlock();
  gradientUpdatesCount++;
  return true;
}
      
bool PSSparseServerTaskUDP::process_send_lr_gradient(const Request& req, std::vector<char>& /*thread_buffer*/) {
  //uint32_t incoming_size = req.incoming_size;
#ifdef DEBUG 
  std::cout << "APPLY_GRADIENT_REQ incoming size: " << incoming_size << std::endl;
#endif
  LRSparseGradient gradient(0);
  gradient.loadSerialized(req.buf);

  model_lock.lock();
  lr_model->sgd_update_adagrad(
      task_config.get_learning_rate(), &gradient);
  model_lock.unlock();
  gradientUpdatesCount++;
  return true;
}

// XXX we have to refactor this ASAP
// move this to SparseMFModel
bool PSSparseServerTaskUDP::process_get_mf_sparse_model(
    const Request& req, std::vector<char>& thread_buffer) {
  uint32_t k_items = 0;
  uint32_t base_user_id = 0;
  uint32_t minibatch_size = 0;
  uint32_t magic_value = 0;
      
  read_all(req.sock, &k_items, sizeof(uint32_t));
  read_all(req.sock, &base_user_id, sizeof(uint32_t));
  read_all(req.sock, &minibatch_size, sizeof(uint32_t));
  read_all(req.sock, &magic_value, sizeof(uint32_t));

  assert(k_items > 0);
  assert(minibatch_size > 0);
  if (magic_value != MAGIC_NUMBER) {
    throw std::runtime_error("Wrong message");
  }
  read_all(req.sock, thread_buffer.data(), k_items * sizeof(uint32_t));

#ifdef DEBUG
  std::cout << "k_items: " << k_items << std::endl;
  std::cout << "base_user_id: " << base_user_id << std::endl;
  std::cout << "minibatch_size: " << minibatch_size << std::endl;
#endif

  SparseMFModel sparse_mf_model((uint64_t)0, 0, 0);
  std::vector<char> data_to_send = sparse_mf_model.serializeFromDense(
      *mf_model, base_user_id, minibatch_size, k_items, thread_buffer.data());

  uint32_t to_send_size = data_to_send.size();
  if (send_all(req.sock, &to_send_size, sizeof(uint32_t)) == -1) {
    return false;
  }
  if (send_all(req.sock, data_to_send.data(), data_to_send.size()) == -1) {
    return false;
  }
  return true;
}

bool PSSparseServerTaskUDP::process_get_lr_sparse_model(
    const Request& req, std::vector<char>& /*thread_buffer*/) {
  // need to parse the buffer to get the indices of the model we want 
  // to send back to the client
  //uint32_t incoming_size = req.incoming_size;
#ifdef DEBUG 
  std::cout << "GET_MODEL_REQ incoming size: " << incoming_size << std::endl;
#endif
  const char* data = req.buf;
  uint64_t num_entries = load_value<uint32_t>(data);

  uint32_t to_send_size = num_entries * sizeof(FEATURE_TYPE);
  assert(to_send_size < 1024 * 1024);
  char data_to_send[1024*1024]; // 1MB
  char* data_to_send_ptr = data_to_send;
#ifdef DEBUG
  std::cout << "Sending back: " << num_entries
    << " weights from model. Size: " << to_send_size
    << std::endl;
#endif
  for (uint32_t i = 0; i < num_entries; ++i) {
    uint32_t entry_index = load_value<uint32_t>(data);
    store_value<FEATURE_TYPE>(
        data_to_send_ptr,
        lr_model->get_nth_weight(entry_index));
  }
  if (send_udp_all(req.sock, (struct sockaddr *) req.clientaddr, data_to_send, to_send_size) == -1) {
    return false;
  }
  return true;
}

bool PSSparseServerTaskUDP::process_get_mf_full_model(
    const Request& req, std::vector<char>& thread_buffer) {
  model_lock.lock();
  auto mf_model_copy = *mf_model;
  model_lock.unlock();
  uint32_t model_size = mf_model_copy.getSerializedSize();

  if (thread_buffer.size() < model_size) {
    std::cout << "thread_buffer.size(): " << thread_buffer.size()
      << " model_size: " << model_size << std::endl;
    throw std::runtime_error("Thread buffer too small");
  }

  mf_model_copy.serializeTo(thread_buffer.data());
  std::cout
    << "Serializing mf model"
    << " mode checksum: " << mf_model_copy.checksum()
    << " buffer checksum: " << crc32(thread_buffer.data(), model_size)
    << std::endl;
  if (send_all(req.sock, &model_size, sizeof(uint32_t)) == -1) {
    return false;
  }
  if (send_all(req.sock, thread_buffer.data(), model_size) == -1) {
    return false;
  }
  return true;
}
      
bool PSSparseServerTaskUDP::process_get_lr_full_model(
    const Request& req, std::vector<char>& thread_buffer) {
  model_lock.lock();
  auto lr_model_copy = *lr_model;
  model_lock.unlock();
  uint32_t model_size = lr_model_copy.getSerializedSize();
  
  lr_model_copy.serializeTo(thread_buffer.data());
  if (send_udp_all(req.sock, (struct sockaddr *) req.clientaddr, thread_buffer.data(), model_size) == -1)
    return false;
  return true;
}

#if 0
void PSSparseServerTaskUDP::gradient_f() {
  std::vector<char> thread_buffer;
  thread_buffer.resize(30 * 1024 * 1024); // 30 MB
  while (1) {
    sem_wait(&sem_new_req);
    to_process_lock.lock();
    Request req = std::move(to_process.front());
    to_process.pop();
    to_process_lock.unlock();

#ifdef DEBUG
    std::cout << "Processing request: " << req.req_id << std::endl;
#endif

    if (req.req_id == SEND_LR_GRADIENT) {
      if (!process_send_lr_gradient(req, thread_buffer)) {
        break;
      }
    }  else if (req.req_id == SEND_MF_GRADIENT) {
      if (!process_send_mf_gradient(req, thread_buffer)) {
        break;
      }
    } else if (req.req_id == GET_LR_SPARSE_MODEL) {
#ifdef DEBUG
      std::cout << "process_get_lr_sparse_model" << std::endl;
      auto before = get_time_us();
#endif
      if (!process_get_lr_sparse_model(req, thread_buffer)) {
        break;
      }
#ifdef DEBUG
      auto elapsed = get_time_us() - before;
      std::cout << "GET_LR_SPARSE_MODEL Elapsed(us): " << elapsed << std::endl;
#endif
    } else if (req.req_id == GET_MF_SPARSE_MODEL) {
      std::cout << "process_get_mf_sparse_model" << std::endl;
      if (!process_get_mf_sparse_model(req, thread_buffer)) {
        break;
      }
    } else if (req.req_id == GET_LR_FULL_MODEL) {
      if (!process_get_lr_full_model(req, thread_buffer))
        break;
    } else if (req.req_id == GET_MF_FULL_MODEL) {
      if (!process_get_mf_full_model(req, thread_buffer))
        break;
    } else {
      throw std::runtime_error("gradient_f: Unknown operation");
    }
    
    // We reactivate events from the client socket here
    req.poll_fd.events = POLLIN;
    //pthread_kill(main_thread, SIGUSR1);
    assert(write(pipefd[1], "a", 1) == 1); // wake up poll()
#ifdef DEBUG
    std::cout << "gradient_f done" << std::endl;
#endif
  }
}
#endif

/**
  * FORMAT
  * operation (uint32_t)
  * incoming size (uint32_t)
  * buffer with previous size
  */
bool PSSparseServerTaskUDP::process(const char* buf,
  struct sockaddr_in* clientaddr) {
  std::vector<char> thread_buffer;
  thread_buffer.resize(30 * 1024 * 1024); // 30 MB
#ifdef DEBUG
  std::cout << "Processing socket: " <<  sock << std::endl;
#endif
  uint32_t operation = load_value<uint32_t>(buf);
#ifdef DEBUG 
  std::cout << "Operation: " << operation << " - " << operation_to_name[operation] << std::endl;
#endif

  if (operation == SEND_LR_GRADIENT || operation == SEND_MF_GRADIENT ||
      operation == GET_LR_SPARSE_MODEL || operation == GET_MF_SPARSE_MODEL) {
    uint32_t incoming_size = load_value<uint32_t>(buf);
    //data_ptr = buffer.data();
#ifdef DEBUG 
    std::cout << "incoming size: " << incoming_size << std::endl;
#endif

    Request req(operation, server_sock_, incoming_size, buf, clientaddr);
    if (operation == SEND_LR_GRADIENT) {
      process_send_lr_gradient(req, thread_buffer);
    } else if (operation == GET_LR_SPARSE_MODEL) {
      process_get_lr_sparse_model(req, thread_buffer);
    } else if (operation == GET_LR_FULL_MODEL) {
      process_get_lr_full_model(req, thread_buffer);
    } else {
      throw std::runtime_error("gradient_f: Unknown operation");
    }
    
    //to_process_lock.lock();
    //poll_fd.events = 0; // explain this
    //to_process.push(Request(operation, sock, incoming_size, poll_fd));
    //to_process_lock.unlock();
    //sem_post(&sem_new_req);
  } else if (operation == GET_LR_FULL_MODEL || operation == GET_MF_FULL_MODEL) {
    assert(0);
    //to_process_lock.lock();
    //poll_fd.events = 0; // we disable events for this socket while we process this message
    //to_process.push(Request(operation, sock, 0, poll_fd));
    //to_process_lock.unlock();
    //sem_post(&sem_new_req);
  } else if (operation == GET_TASK_STATUS) {
#if 0
    uint32_t task_id;
    if (read_all(sock, &task_id, sizeof(uint32_t)) == 0) {
      return false;
    }
#ifdef DEBUG
    std::cout << "Get status task id: " << task_id << std::endl;
#endif
    assert(task_id < 10000);
    if (task_to_status.find(task_id) == task_to_status.end() ||
        task_to_status[task_id] == false) {
      uint32_t status = 0;
      send_all(sock, &status, sizeof(uint32_t));
    } else {
      uint32_t status = 1;
      send_all(sock, &status, sizeof(uint32_t));
    }
#endif
  } else if (operation == SET_TASK_STATUS) {
#if 0
    uint32_t data[2] = {0}; // id + status
    if (read_all(sock, data, sizeof(uint32_t) * 2) == 0) {
      return false;
    }
#ifdef DEBUG
    std::cout << "Set status task id: " << data[0] << " status: " << data[1] << std::endl;
#endif
    task_to_status[data[0]] = data[1];
#endif
  } else {
    std::string error = "process: Uknown operation " + std::to_string(operation);
    throw std::runtime_error(error);
  }
  return true;
}

void PSSparseServerTaskUDP::start_server() {
  lr_model.reset(new SparseLRModel(MODEL_GRAD_SIZE));
  lr_model->randomize();
  mf_model.reset(new MFModel(task_config.get_users(), task_config.get_items(), NUM_FACTORS));
  mf_model->randomize();
  
  sem_init(&sem_new_req, 0, 0);

  server_thread = std::make_unique<std::thread>(
      std::bind(&PSSparseServerTaskUDP::poll_thread_fn, this));

  for (uint32_t i = 0; i < n_threads; ++i) {
    gradient_thread.push_back(std::make_unique<std::thread>(
        std::bind(&PSSparseServerTaskUDP::gradient_f, this)));
  }
}

void PSSparseServerTaskUDP::poll_thread_fn() {
  std::cout << "Starting server2" << std::endl;

  poll_thread = pthread_self();

  server_sock_ = socket(AF_INET, SOCK_DGRAM, 0); 
  if (server_sock_ < 0) {
    throw cirrus::ConnectionException("Server error creating socket");
  }   

  int opt = 1;
  if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    throw std::runtime_error("Error forcing port binding");
  }

  if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
    throw std::runtime_error("Error forcing port binding");
  }   

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port_);
  std::memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));

  int ret = bind(server_sock_, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
  if (ret < 0) {
    throw std::runtime_error("Error binding in port " + to_string(port_));
  }

  loop();
}

void PSSparseServerTaskUDP::loop() {
  struct sockaddr_in clientaddr; /* client addr */
  socklen_t clientlen = sizeof(clientaddr);

  while (1) {
    int BUFSIZE = 100000;
    char* buf = new char[BUFSIZE];
    bzero(buf, BUFSIZE);
    struct sockaddr_in clientaddr; /* client addr */
    int n = recvfrom(server_sock_, buf, BUFSIZE, 0,
        (struct sockaddr *) &clientaddr, &clientlen);
    // make sure we always have leftover space
    assert(n < BUFSIZE - 1);
    process(buf, &clientaddr);
  }
}
  
/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void PSSparseServerTaskUDP::run(const Configuration& config) {
  std::cout
    << "PS task initializing model"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;

  task_config = config;
  start_server();

  //wait_for_start(PS_SPARSE_SERVER_TASK_RANK, redis_con, nworkers);

  uint64_t start = get_time_us();
  uint64_t last_tick = get_time_us();
  while (1) {
    auto now = get_time_us();
    auto elapsed_us = now - last_tick;
    auto since_start_sec = 1.0 * (now - start) / 1000000;
    if (elapsed_us > 1000000) {
      last_tick = now;
      std::cout << "Events in the last sec: " 
        << 1.0 * gradientUpdatesCount / elapsed_us * 1000 * 1000
        << " since (sec): " << since_start_sec
        << " #conns: " << num_connections
        << std::endl;
      gradientUpdatesCount = 0;
    }
    sleep(1);
  }
}

void PSSparseServerTaskUDP::checkpoint_model() const {
  uint64_t model_size;
  std::shared_ptr<char> data = serialize_lr_model(*lr_model, &model_size);

  std::ofstream fout("model_backup_file", std::ofstream::binary);
  fout.write(data.get(), model_size);
  fout.close();
}

