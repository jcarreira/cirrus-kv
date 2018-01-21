#include <ProgressMonitor.h>

ProgressMonitor::ProgressMonitor(const std::string& redis_ip, int redis_port) {
  redis_con = redis_connect(redis_ip.c_str(), redis_port);
}

void ProgressMonitor::increment_batches(int* prev_batch) {
  redis_increment_counter(redis_con, PROGRESS_COUNTER, prev_batch);
}
