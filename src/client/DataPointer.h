/* Copyright 2016 Joao Carreira */

#ifndef _DATA_POINTER_H_
#define _DATA_POINTER_H_

#include <string>

namespace cirrus {

/** A class that allows us to share access to data.
  * Apps can pass around DataPointers instead of
  * shipping data
  */
class DataPointer {
    DataPointer(std::string blade_addr,
            std::string blade_port,
            uint64_t remote_addr);
    // FIX: we also need a key here

private:
    std::string blade_address_;
    std::string blade_port_;
    uint64_t remote_addr_;
};

}  // namespace cirrus

#endif // _DATA_POINTER_H_
