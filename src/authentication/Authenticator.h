#ifndef SRC_AUTHENTICATION_AUTHENTICATOR_H_
#define SRC_AUTHENTICATION_AUTHENTICATOR_H_

#include "authentication/ApplicationKey.h"
#include "common/AllocatorMessage.h"

namespace cirrus {

class Authenticator {
 public:
    virtual bool allowApplication(const AppId& app_id) = 0;
 private:
};

}  // namespace cirrus

#endif  // SRC_AUTHENTICATION_AUTHENTICATOR_H_
