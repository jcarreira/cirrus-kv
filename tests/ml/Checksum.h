#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include <cstdint>
#include <cstddef>

uint32_t crc32(const void *buf, size_t size);

#endif  // _CHECKSUM_H_
