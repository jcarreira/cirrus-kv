/* Copyright 2016 Joao Carreira */

#ifndef _DATA_POINTER_H_
#define _DATA_POINTER_H_

#include <string>

// We use a DataPointer to share access to data
// Apps can pass around DataPointers instead of
// shipping data

namespace sirius {

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

}  // namespace sirius

#endif // _DATA_POINTER_H_
