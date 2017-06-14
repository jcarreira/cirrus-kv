#include "src/authentication/AuthenticationToken.h"

namespace cirrus {

AuthenticationToken::AuthenticationToken(bool allow) :
    allow(allow) {
}

AuthenticationToken AuthenticationToken::create_default_allow_token() {
    return AuthenticationToken(true);
}

AuthenticationToken AuthenticationToken::create_default_deny_token() {
    return AuthenticationToken(false);
}

}  // namespace cirrus
