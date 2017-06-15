#ifndef _AUTHENTICATION_TOKEN_H_
#define _AUTHENTICATION_TOKEN_H_

#include "GrantingKey.h"

namespace cirrus {

class AuthenticationToken {
 public:
    explicit AuthenticationToken(bool allow);

    static AuthenticationToken create_default_deny_token();
    static AuthenticationToken create_default_allow_token();

    // crypto key
    GrantingKey gkey;
    // let application know if
    // access was granted
    char allow;
};

};  // namespace cirrus

#endif  // _AUTHENTICATION_TOKEN_H_
