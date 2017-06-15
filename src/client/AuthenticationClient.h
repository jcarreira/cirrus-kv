#ifndef _AUTH_CLIENT_H_
#define _AUTH_CLIENT_H_

#include "src/client/RDMAClient.h"
#include "src/authentication/AuthenticationToken.h"

namespace cirrus {

class AuthenticationClient : public RDMAClient {
 public:
    explicit AuthenticationClient(int timeout_ms = 500);
    ~AuthenticationClient() = default;

    AuthenticationToken authenticate();

 private:
};

}  // namespace cirrus

#endif  // _AUTH_CLIENT_H_
