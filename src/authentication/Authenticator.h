/* Copyright 2016 Joao Carreira */

#ifndef _AUTHENTICATOR_H_
#define _AUTHENTICATOR_H_

#include "src/authentication/ApplicationKey.h"

namespace sirius {

class Authenticator {
public:
    virtual bool allowApplication(const ApplicationKey& app_key) = 0;
private:
};

} // sirius

#endif  // _AUTHENTICATOR_H_
