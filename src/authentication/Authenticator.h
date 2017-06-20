#ifndef _AUTHENTICATOR_H_
#define _AUTHENTICATOR_H_

#include "authentication/ApplicationKey.h"
#include "common/AllocatorMessage.h"

namespace cirrus {

class Authenticator {
 public:
    virtual bool allowApplication(const AppId& app_id) = 0;
 private:
};

}  // namespace cirrus

#endif  // _AUTHENTICATOR_H_
