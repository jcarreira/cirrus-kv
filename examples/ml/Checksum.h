#ifndef EXAMPLES_ML_CHECKSUM_H_
#define EXAMPLES_ML_CHECKSUM_H_

#include <cstdint>
#include <cstddef>
#include <memory>

/**
  * Compute crc32 checksum of data in buf and with size length
  * @return crc32 checksum
  */
uint32_t crc32(const void *buf, size_t size);

/**
  * Compute crc32 checksum for array of doubles with length size
  * @return crc32 checksum
  */
double checksum(std::shared_ptr<double> p, uint64_t size);

#endif  // EXAMPLES_ML_CHECKSUM_H_
