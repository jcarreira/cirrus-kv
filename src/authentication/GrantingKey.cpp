/* Copyright 2016 Joao Carreira */

#include "src/authentication/GrantingKey.h"

#include <cstring>

namespace sirius {

GrantingKey::GrantingKey() {
    std::memset(data_, 0, sizeof(data_));
}

}
