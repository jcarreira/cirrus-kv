/* Copyright 2016 Joao Carreira */

#include "src/authentication/AuthenticationToken.h"

namespace sirius {

AuthenticationToken::AuthenticationToken(bool allow) :
    allow(allow) {
}

AuthenticationToken AuthenticationToken::create_default_allow_token() {
    return AuthenticationToken(true);
}

AuthenticationToken AuthenticationToken::create_default_deny_token() {
    return AuthenticationToken(false);
}

}  // namespace sirius
