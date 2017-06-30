#ifndef SRC_COMMON_FUTURE_H_
#define SRC_COMMON_FUTURE_H_

#include <string>
#include <memory>

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
           std::shared_ptr<cirrus::ErrorCodes> error_code);

    void wait();

    bool try_wait();

    bool get();
 private:
    /** Pointer to the result. */
    std::shared_ptr<bool> result;
    /** Boolean monitoring result state. */
    std::shared_ptr<bool> result_available;
    /** Sem for the result. */
    std::shared_ptr<cirrus::PosixSemaphore> sem;
     /** Any errors thrown. */
    std::shared_ptr<cirrus::ErrorCodes> error_code;
};

}  // namespace cirrus

#endif  // SRC_COMMON_FUTURE_H_
