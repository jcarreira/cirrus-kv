/* Copyright Joao Carreira 2016 */

#include "src/memories/3DXPoint.h"

namespace sirius {

3DXPoint::getWriteBandwidth() {
    return write_bandwidth_;
}

3DXPoint::getReadBandwidth() {
    return read_bandwidth_;
}

3DXPoint::getReadLatency() {
    return read_latency_us_;
}

3DXPoint::getWriteLatency() {
    return write_latency_us_;
}

}  //  namespace sirius
