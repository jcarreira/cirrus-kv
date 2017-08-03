#ifndef SRC_COMMON_FUTURE_H_
#define SRC_COMMON_FUTURE_H_

#include <string>
#include <memory>
#include <utility>

#include "common/Synchronization.h"
#include "common/Exception.h"

namespace cirrus {

using ObjectID = uint64_t;

/**
  * A class that all clients inherit from. Outlines the interface that the
  * store will use to interface with the network level. Used to monitor
  * operations that have a true/false result.
  */
class Future {
 public:
    Future(std::shared_ptr<bool> result,
           std::shared_ptr<bool> result_available,
           std::shared_ptr<cirrus::PosixSemaphore> sem,
           std::shared_ptr<cirrus::ErrorCodes> error_code,
           std::shared_ptr<std::shared_ptr<char>> data_ptr,
           std::shared_ptr<uint64_t> data_size);

    void wait();

    bool try_wait();

    bool get();

    std::pair<std::shared_ptr<char>, unsigned int> getDataPair();


 private:
    /** Pointer to the result. */
    std::shared_ptr<bool> result;
    /** Boolean monitoring result state. */
    std::shared_ptr<bool> result_available;
    /** Sem for the result. */
    std::shared_ptr<cirrus::PosixSemaphore> sem;
     /** Any errors thrown. */
    std::shared_ptr<cirrus::ErrorCodes> error_code;
    /** Pointer to a pointer to any mem for a read, if any. */
    std::shared_ptr<std::shared_ptr<char>> data_ptr;
    /** Size of the memory block for a read. */
    std::shared_ptr<uint64_t> data_size;
};

}  // namespace cirrus

#endif  // SRC_COMMON_FUTURE_H_
