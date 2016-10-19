/* Copyright 2016 Joao Carreira */

#ifndef _AUTHENTICATION_TOKEN_H_
#define _AUTHENTICATION_TOKEN_H_

#include "GrantingKey.h"

namespace sirius {

class AuthenticationToken {
public:
    AuthenticationToken(bool allow);
    
    static AuthenticationToken create_default_deny_token();
    static AuthenticationToken create_default_allow_token();
   
    // crypto key 
    GrantingKey gkey;
    // let application know if
    // access was granted
    char allow;
};

};

#endif // _AUTHENTICATION_TOKEN_H_
