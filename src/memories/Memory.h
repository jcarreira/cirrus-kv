/* Copyright Joao Carreira 2016 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

namespace sirius {

struct Memory {
    virtual uint64_t getReadLatency() const = 0;
    virtual uint64_t getWriteLatency() const = 0;
    virtual uint64_t getReadBandwidth() const = 0;
    virtual uint64_t getWriteBandwidth() const = 0;
};

}  // namespace sirius
