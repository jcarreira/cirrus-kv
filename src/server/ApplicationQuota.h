/* Copyright 2016 Joao Carreira */

#ifndef _APPLICATION_QUOTA_H_
#define _APPLICATION_QUOTA_H_

/*
 * This describes how many resources an application
 * can allocate in this domain
 */

namespace cirrus {

class ApplicationQuota {
public:
    ApplicationQuota(uint64_t memory) :
        memory_(memmory) {}}

    virtual ~ApplicationQuota() {}

private:
    uint64_t memory_;
};

} // cirrus

#endif // _APPLICATION_QUOTA_H_

