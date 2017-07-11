#ifndef SRC_AUTHENTICATION_GRANTINGKEY_H_
#define SRC_AUTHENTICATION_GRANTINGKEY_H_

namespace cirrus {

class GrantingKey {
 public:
    GrantingKey();

    static constexpr int KEY_SIZE = 10;

    // if access granted access key goes
    // here. To be used when contacting
    // memory blades
    char data_[KEY_SIZE];
};

}  // namespace cirrus

#endif  // SRC_AUTHENTICATION_GRANTINGKEY_H_
