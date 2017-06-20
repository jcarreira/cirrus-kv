#include "authentication/GrantingKey.h"

#include <cstring>

namespace cirrus {

GrantingKey::GrantingKey() {
    std::memset(data_, 0, sizeof(data_));
}

}
