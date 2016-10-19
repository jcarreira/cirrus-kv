/* Copyright 2016 Joao Carreira */

#ifndef _GRANTING_KEY_H_
#define _GRANTING_KEY_H_

namespace sirius {

class GrantingKey {
public:
    GrantingKey();

    static constexpr int KEY_SIZE = 10;
    
    // if access granted access key goes
    // here. To be used when contacting
    // memory blades
    char data_[KEY_SIZE];
};

} // sirius

#endif // _GRANTING_KEY_H_
