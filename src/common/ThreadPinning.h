/* Copyright 2016 Joao Carreira */

#include <pthread.h>
#include <thread>
#include "src/utils/logging.h"

namespace sirius {

class ThreadPinning {
public:
    static void pinThread(
            std::thread::native_handle_type thread_handle, int cpu) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);

        int ret = pthread_setaffinity_np(thread_handle,
                sizeof(cpu_set_t), &cpuset);
        if (ret != 0) {
            LOG<ERROR>("Error calling pthread_setaffinity_np: ", ret);
        }
    }
};

}
