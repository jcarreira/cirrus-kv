#ifndef SRC_MEMORIES_3DXPOINT_H_
#define SRC_MEMORIES_3DXPOINT_H_

namespace cirrus {

class 3DXPoint : public Memory {
 public:
    uint64_t getReadLatency() const {}
    uint64_t getWriteLatency() const {}
    uint64_t getReadBandwidth() const {}
    uint64_t getWriteBandwidth() const {}
 private:
    const uint64_t GB = 1024*1024*1024;
    const uint64_t read_latency_us_ = 10;
    const uint64_t write_latency_us_ = 10;
    const uint64_t read_bandwidth_ = 10 * GB;
    const uint64_t write_bandwidth_ = 10 GB;
};

}  // namespace cirrus

#endif  // SRC_MEMORIES_3DXPOINT_H_
