#ifndef _GRANTING_KEY_H_
#define _GRANTING_KEY_H_

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

} // cirrus

#endif // _GRANTING_KEY_H_
