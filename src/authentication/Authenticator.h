/* Copyright 2016 Joao Carreira */

#ifndef _AUTHENTICATOR_H_
#define _AUTHENTICATOR_H_

#include "src/authentication/ApplicationKey.h"
#include "src/common/AllocatorMessage.h"

namespace cirrus {

class Authenticator {
public:
    virtual bool allowApplication(const AppId& app_id) = 0;
private:
};

} // cirrus

#endif  // _AUTHENTICATOR_H_
