#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include <cstdint>
#include <cstddef>
#include <memory>

uint32_t crc32(const void *buf, size_t size);
double checksum(std::shared_ptr<double> p, uint64_t size);

#endif  // _CHECKSUM_H_
