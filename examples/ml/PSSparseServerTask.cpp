#include <examples/ml/Tasks.h>

#include "Serializers.h"
#include "Redis.h"
#include "Utils.h"
#include "async.h"
#include <climits>

#undef DEBUG

#define APPLY_GRADIENT_REQ (1)
#define GET_MODEL_REQ (2)
#define GET_FULL_MODEL_REQ (3)
#define REGISTER_WORKER_REQ (4)
#define GET_SERVER_CLOCK_REQ (5)

#define MAX_CONNECTIONS (21)
#define MAX_WORKERS (10)
    
static void update_model(auto&);

namespace PSSparseServerTaskGlobal {
  // used to monitor how much since last received gradient
  static auto prev_on_msg_time = get_time_us();

  static Configuration config;
  static std::mutex mp_start_lock; // used as barrier
 
  sem_t sem_new_model;
  static std::unique_ptr<SparseLRModel> model; // last computed model
  std::mutex model_lock; // used to coordinate access to the last computed model

  redisContext* redis_con;
  volatile uint64_t onMessageCount = 0;
}

auto PSSparseServerTask::connect_redis() {
  auto redis_con  = redis_connect(REDIS_IP, REDIS_PORT);
  if (redis_con == NULL || redis_con -> err) {
    throw std::runtime_error(
        "Error connecting to redis server. IP: " + std::string(REDIS_IP));
  }
  return redis_con;
}

PSSparseServerTask::PSSparseServerTask(const std::string& redis_ip, uint64_t redis_port,
    uint64_t MODEL_GRAD_SIZE, uint64_t MODEL_BASE,
    uint64_t LABEL_BASE, uint64_t GRADIENT_BASE,
    uint64_t SAMPLE_BASE, uint64_t START_BASE,
    uint64_t batch_size, uint64_t samples_per_batch,
    uint64_t features_per_sample, uint64_t nworkers,
    uint64_t worker_id) :
  MLTask(redis_ip, redis_port, MODEL_GRAD_SIZE, MODEL_BASE,
      LABEL_BASE, GRADIENT_BASE, SAMPLE_BASE, START_BASE,
      batch_size, samples_per_batch, features_per_sample,
      nworkers, worker_id) {
  std::cout << "PSSparseServerTask is built" << std::endl;
}

std::shared_ptr<char> PSSparseServerTask::serialize_model(const SparseLRModel& model, uint64_t* model_size) {
  *model_size = model.getSerializedSize();
  auto d = std::shared_ptr<char>(
      new char[*model_size], std::default_delete<char[]>());
  model.serializeTo(d.get());
  return d;
}

bool PSSparseServerTask::testRemove(struct pollfd x) {
  // If this pollfd will be removed, the index of the next location to insert
  // should be reduced by one correspondingly.
  if (x.fd == -1) {
    curr_index -= 1;
  }
  return x.fd == -1;
}

uint64_t PSSparseServerTask::get_clocks_average() const {
  uint64_t avg = 0;

  // this can happen for two reasons:
  // 1. the first time a worker registers
  // 2. if all workers die at the same time
  fd_to_clock_lock.lock();
  if (fd_to_clock.size() == 0) {
    fd_to_clock_lock.unlock();
    return 0;
  }

  for (const auto& v : fd_to_clock) {
    avg += v.second;
  }
  uint64_t size = fd_to_clock.size();
  fd_to_clock_lock.unlock();
  return avg / size;
}

uint64_t PSSparseServerTask::get_clocks_min() const {
  uint64_t min = INT_MAX;
  fd_to_clock_lock.lock();
  for (const auto& v : fd_to_clock) {
    if (v.second < min)
      min = v.second;
  }
  fd_to_clock_lock.unlock();
  return min;
}

bool PSSparseServerTask::is_sock_registered(int fd) const {
  bool ret = true;
  fd_to_clock_lock.lock();
  if (fd_to_clock.find(fd) == fd_to_clock.end()) {
    ret = false;
  }
  fd_to_clock_lock.unlock();
  return ret;
}

void PSSparseServerTask::gradient_f() {
  while (1) {
    sem_wait(&sem_new_req);
    to_process_lock.lock();
    Request& req = to_process.front();
    to_process_lock.unlock();

#ifdef DEBUG
    std::cout << "Processing request: " << req.req_id << std::endl;
#endif

    if (req.req_id == APPLY_GRADIENT_REQ) {
      std::vector<char>& buffer = req.vec;
      LRSparseGradient gradient(0);
      gradient.loadSerialized(buffer.data());

      PSSparseServerTaskGlobal::model_lock.lock();
      PSSparseServerTaskGlobal::model->sgd_update_adagrad(
          PSSparseServerTaskGlobal::config.get_learning_rate(), &gradient);
      PSSparseServerTaskGlobal::model_lock.unlock();
      sem_post(&PSSparseServerTaskGlobal::sem_new_model);
      PSSparseServerTaskGlobal::onMessageCount++;

      if (!is_sock_registered(req.sock)) {
        throw std::runtime_error("Client not registered");
      } else {
        fd_to_clock_lock.lock();
        fd_to_clock[req.sock]++; // increment worker's clock
        fd_to_clock_lock.unlock();
      }
    } else if (req.req_id == GET_MODEL_REQ) {
      // need to parse the buffer to get the indices of the model we want 
      // to send back to the client
      std::vector<char>& buffer = req.vec;
      const char* data = buffer.data();
      uint64_t num_entries = load_value<uint32_t>(data);

      uint32_t to_send_size = num_entries * sizeof(FEATURE_TYPE);
      auto data_to_send = std::shared_ptr<char>(
          new char[to_send_size], std::default_delete<char[]>());
      char* data_to_send_ptr = data_to_send.get();
#ifdef DEBUG
      std::cout << "Sending back: " << num_entries
        << " weights from model. Size: " << to_send_size
        << std::endl;
#endif
      for (uint32_t i = 0; i < num_entries; ++i) {
        uint32_t entry_index = load_value<uint32_t>(data);
#ifdef DEBUG
        std::cout << entry_index << " ";
#endif
        store_value<FEATURE_TYPE>(
            data_to_send_ptr,
            PSSparseServerTaskGlobal::model->get_nth_weight(entry_index));
      }
#ifdef DEBUG
      std::cout << std::endl;
#endif
      send_all(req.sock, data_to_send.get(), to_send_size);
    } else if (req.req_id == GET_FULL_MODEL_REQ) {
      PSSparseServerTaskGlobal::model_lock.lock();
      auto model_copy = *PSSparseServerTaskGlobal::model;
      PSSparseServerTaskGlobal::model_lock.unlock();
      uint32_t model_size = model_copy.getSerializedSize();

      auto d = std::shared_ptr<char>(
          new char[model_size], std::default_delete<char[]>());
      model_copy.serializeTo(d.get());
      //std::cout << "Sending model message size: " << model_size << std::endl;
      send_all(req.sock, d.get(), model_size);
    } else if (req.req_id == REGISTER_WORKER_REQ) {
      onClientStart(req.sock);
    } else if (req.req_id == GET_SERVER_CLOCK_REQ) {
      // the PS clock is the minimmum clock among all workers
      uint64_t server_clock = get_clocks_min();
      if (server_clock == INT_MAX) {
        // there are no active workers at the moment
        throw std::runtime_error("Error getting PS clock");
      }
      send_all(req.sock, &server_clock, sizeof(server_clock));
    } else {
      throw std::runtime_error("Unknown operation");
    }

    to_process_lock.lock();
    to_process.pop();
    to_process_lock.unlock();
  }
}

/**
  * FORMAT
  * operation (uint32_t)
  * incoming size (uint32_t)
  * buffer with previous size
  * return false to close socket
  */
bool PSSparseServerTask::process(int sock) {
#ifdef DEBUG
  std::cout << "Processing socket: " <<  sock << std::endl;
#endif
  std::vector<char> buffer;
  uint64_t current_buf_size = sizeof(uint32_t);
  buffer.reserve(current_buf_size);
  
  uint64_t bytes_read = 0;
  bool ret = read_from_client(buffer, sock, bytes_read);
  if (!ret) {
    return false;
  }

  uint32_t* operation_ptr = reinterpret_cast<uint32_t*>(buffer.data());
  uint32_t operation = *operation_ptr;
 
#ifdef DEBUG 
  std::cout << "Operation: " << operation << std::endl;
#endif

  if (operation == APPLY_GRADIENT_REQ || operation == GET_MODEL_REQ) { // GRADIENT
    uint64_t bytes_read = 0;
    bool ret = read_from_client(buffer, sock, bytes_read);
    if (!ret) {
      return false;
    }

    uint32_t* incoming_size_ptr = reinterpret_cast<uint32_t*>(buffer.data());
    uint32_t incoming_size = *incoming_size_ptr;
#ifdef DEBUG 
    std::cout << "incoming size: " << incoming_size << std::endl;
#endif
    if (incoming_size > current_buf_size) {
      buffer.resize(incoming_size);
    }

    try {
      read_all(sock, buffer.data(), incoming_size);
    } catch(...) {
      return false;
    }

    to_process_lock.lock();
    to_process.push(Request(operation, sock, std::move(buffer)));
    to_process_lock.unlock();
    sem_post(&sem_new_req);
  } else if (operation == GET_FULL_MODEL_REQ) {
    to_process_lock.lock();
    to_process.push(Request(operation, sock, std::vector<char>()));
    to_process_lock.unlock();
  } else if (operation == REGISTER_WORKER_REQ) {
    if (num_workers >= MAX_WORKERS) {
      std::cout << "Achieved max number of workers. Closing connection" << std::endl;
      return false;
    }
    to_process_lock.lock();
    to_process.push(Request(operation, sock, std::vector<char>()));
    to_process_lock.unlock();
    sem_post(&sem_new_req);
    sem_post(&sem_new_req);
  } else {
    throw std::runtime_error("Unknown operation");
  }
  return true;
}

bool PSSparseServerTask::read_from_client(
    std::vector<char>& buffer, int sock, uint64_t& bytes_read) {
  bool first_loop = true;
  while (bytes_read < static_cast<int>(sizeof(uint32_t))) {
    int retval = read(sock, buffer.data() + bytes_read,
        sizeof(uint32_t) - bytes_read);

    if (first_loop && retval == 0) {
      // Socket is closed by client if 0 bytes are available
      close(sock);
      return false;
    }
    if (retval < 0) {
      close(sock);
      return false; // likely because lambda worker died after 5 minutes
      throw std::runtime_error("Server issue in reading socket during size read.");
    }
    bytes_read += retval;
    first_loop = false;
  }
  return true;
}

void PSSparseServerTask::start_server() {
  PSSparseServerTaskGlobal::model.reset(new SparseLRModel(MODEL_GRAD_SIZE));
  PSSparseServerTaskGlobal::model->randomize();
  
  sem_init(&sem_new_req, 0, 0);

  server_thread = std::make_unique<std::thread>(
      std::bind(&PSSparseServerTask::start_server2, this));
  gradient_thread = std::make_unique<std::thread>(
      std::bind(&PSSparseServerTask::gradient_f, this));
}

void PSSparseServerTask::start_server2() {
  std::cout << "Starting server2" << std::endl;
  server_sock_ = socket(AF_INET, SOCK_STREAM, 0); 
  if (server_sock_ < 0) {
    throw cirrus::ConnectionException("Server error creating socket");
  }   

  int opt = 1;
  if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    throw std::runtime_error("Error forcing port binding");
  }
  if (setsockopt(server_sock_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
    throw std::runtime_error("Error setting socket options.");
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

  if (listen(server_sock_, SOMAXCONN) == -1) {
    throw std::runtime_error("Error listening on port " + to_string(port_));
  }
  fds.at(curr_index).fd = server_sock_;
  fds.at(curr_index++).events = POLLIN;
  loop();
}

void PSSparseServerTask::onSocketClose(int fd) {
  std::cout << "onSocketClose fd: " << fd << std::endl;
  fd_to_clock_lock.lock();
  if (fd_to_clock.find(fd) != fd_to_clock.end()) {
    fd_to_clock.erase(fd);
    num_workers--;
  }
  fd_to_clock_lock.unlock();
}

void PSSparseServerTask::onClientStart(int fd) {
  std::cout << "onClientStart fd: " << fd << std::endl;
  
  if (is_sock_registered(fd)) {
    throw std::runtime_error("This socket is already registered");
  }

  // get average of everyone's clocks. That becomes this worker's clock
  uint32_t avg = get_clocks_average();
  fd_to_clock_lock.lock();
  fd_to_clock[fd] = avg;
  fd_to_clock_lock.unlock();
  num_workers++;
}

void PSSparseServerTask::loop() {
  struct sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);

  std::cout << "Starting loop" << std::endl;
  while (1) {
    int poll_status = poll(fds.data(), curr_index, timeout);
    if (poll_status == -1) {
      throw std::runtime_error("Server error calling poll.");
    } else if (poll_status == 0) {
      std::cout << timeout << " ms elapsed" << std::endl;
    } else {
      // there is at least one pending event, find it.
      for (uint64_t i = 0; i < curr_index; i++) {
	struct pollfd& curr_fd = fds.at(i);
	// Ignore the fd if we've said we don't care about it
	if (curr_fd.fd == -1) {
	  continue;
	}
	if (curr_fd.revents != POLLIN) {
	  if (curr_fd.revents & POLLHUP) {
            std::cout << "PS closing connection " << num_connections << std::endl;
            num_connections--;
            onSocketClose(curr_fd.fd);
	    close(curr_fd.fd);
	    curr_fd.fd = -1;
	  }
	} else if (curr_fd.fd == server_sock_) {
          std::cout << "PS new connection!" << std::endl;
	  int newsock = accept(server_sock_,
	      reinterpret_cast<struct sockaddr*>(&cli_addr),
	      &clilen);
	  if (newsock < 0) {
	    throw std::runtime_error("Error accepting socket");
	  }
	  // If at capacity, reject connection
          if (num_connections > (MAX_CONNECTIONS - 1)) {
            std::cout << "Rejecting connection "
              << num_connections
              << std::endl;
            close(newsock);
          } else if (curr_index == max_fds) {
            throw std::runtime_error("We reached capacity");
	    close(newsock);
	  } else {
	    fds.at(curr_index).fd = newsock;
	    fds.at(curr_index).events = POLLIN;
	    curr_index++;
            num_connections++;
	  }
	} else {
#ifdef DEBUG
          std::cout << "Calling process" << std::endl;
#endif
	  if (!process(curr_fd.fd)) {
            onSocketClose(curr_fd.fd);
            num_connections--;
            std::cout << "PS closing connection " << num_connections << std::endl;
	    // do not make future alerts on this fd
	    curr_fd.fd = -1;
	  }
	}
	curr_fd.revents = 0;  // Reset the event flags
      }
    }
    // If at max capacity, try to make room
    if (curr_index == max_fds) {
      // Try to purge unused fds, those with fd == -1
      std::cout << "Purging" << std::endl;
      std::remove_if(fds.begin(), fds.end(),
          std::bind(&PSSparseServerTask::testRemove, this, std::placeholders::_1));
    }
  }
}

/**
  * This is the task that runs the parameter server
  * This task is responsible for
  * 1) sending the model to the workers
  * 2) receiving the gradient updates from the workers
  *
  */
void PSSparseServerTask::run(const Configuration& config) {
  std::cout
    << "PS task initializing model"
    << " MODEL_GRAD_SIZE: " << MODEL_GRAD_SIZE
    << std::endl;

  start_server();
  
  sem_init(&PSSparseServerTaskGlobal::sem_new_model, 0, 0);

  // initialize model
  PSSparseServerTaskGlobal::model.reset(new SparseLRModel(MODEL_GRAD_SIZE));
  PSSparseServerTaskGlobal::model->randomize();
  std::cout << "[PS] "
    << "PS publishing model at id: " << MODEL_BASE
    << " csum: " << PSSparseServerTaskGlobal::model->checksum() << std::endl;

  PSSparseServerTaskGlobal::config = config;

  PSSparseServerTaskGlobal::redis_con = connect_redis();
  //publish_model_redis();
  wait_for_start(PS_SPARSE_SERVER_TASK_RANK, PSSparseServerTaskGlobal::redis_con, nworkers);

  uint64_t start = get_time_us();
  uint64_t last_tick = get_time_us();
  while (1) {
    sem_wait(&PSSparseServerTaskGlobal::sem_new_model);
    publish_model_redis();

    auto now = get_time_us();
    auto elapsed_us = now - last_tick;
    auto since_start_sec = 1.0 * (now - start) / 1000000;
    if (elapsed_us > 1000000) {
      last_tick = now;
      std::cout << "Events in the last sec: " 
        << 1.0 * PSSparseServerTaskGlobal::onMessageCount / elapsed_us * 1000 * 1000
        << " since (sec): " << since_start_sec
        << " #conns: " << num_connections
        << " #workers: " << num_workers
        << std::endl;
      PSSparseServerTaskGlobal::onMessageCount = 0;
    }
  }
}

void PSSparseServerTask::publish_model_redis() {
#ifdef DEBUG
  static int publish_count = 0;
  std::cout << "[PS] "
    << "Publishing model at: " << get_time_us()
    << " publish_count: " << (++publish_count)
    << "\n";
#endif

  PSSparseServerTaskGlobal::model_lock.lock();
  auto model_copy = *PSSparseServerTaskGlobal::model;
  PSSparseServerTaskGlobal::model_lock.unlock();

#ifdef DEBUG
  std::cout << "Checking model" << std::endl;
  model_copy.check();
#endif
  
  uint64_t model_size;
  std::shared_ptr<char> data = serialize_model(model_copy, &model_size);

#ifdef DEBUG
  std::cout << "redisCommand PUBLISH MODEL" << std::endl;
  auto before_us = get_time_us();
#endif

  //redis_put_binary_numid(PSSparseServerTaskGlobal::redis_con, MODEL_BASE,
  //    reinterpret_cast<const char*>(data.get()), model_size);
#ifdef DEBUG
  auto elapsed_us = get_time_us() - before_us;
  std::cout 
    << "Put model elapsed (us): " << elapsed_us
    << " bw (MB/s): " << (1.0 * model_size / 1024 / 1024 / elapsed_us * 1000 * 1000)
    << "\n";
#endif
}

