/* Copyright 2016 Joao Carreira */

#ifndef _DUMB_AUTHENTICATOR_H_
#define _DUMB_AUTHENTICATOR_H_

namespace cirrus {

class DumbAuthenticator : public Authenticator {
public:
    DumbAuthenticator(){}
    bool allowApplication(const AppId& /* app_id */) { 
        return true; 
    }
private:
};

} // cirrus

#endif // _DUMB_AUTHENTICATOR_H_
