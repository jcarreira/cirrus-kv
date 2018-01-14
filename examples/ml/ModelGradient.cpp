#include <examples/ml/ModelGradient.h>
#include <iostream>
#include <algorithm>
#include "utils/Log.h"

LRGradient::LRGradient(LRGradient&& other) {
    weights = std::move(other.weights);
    version = other.version;
}

LRGradient::LRGradient(const std::vector<FEATURE_TYPE>& data) :
    weights(data) {
}

LRGradient::LRGradient(int d) {
    weights.resize(d);
    version = 0;
}

LRGradient& LRGradient::operator=(LRGradient&& other) {
    weights = std::move(other.weights);
    version = other.version;
    return *this;
}

void LRGradient::loadSerialized(const void* mem) {
    version = *reinterpret_cast<const uint32_t*>(mem);
    mem = reinterpret_cast<const void*>(
            (reinterpret_cast<const char*>(mem) + sizeof(uint32_t)));
    const FEATURE_TYPE* data = reinterpret_cast<const FEATURE_TYPE*>(mem);
    std::copy(data, data + weights.size(), weights.begin());
}

/** Format:
  * version (uint32_t)
  * vector of weights (FEATURE_TYPE * n)
  */
void LRGradient::serialize(void* mem) const {
    *reinterpret_cast<uint32_t*>(mem) = version;
    mem = reinterpret_cast<void*>(
            (reinterpret_cast<char*>(mem) + sizeof(uint32_t)));
    FEATURE_TYPE* data = reinterpret_cast<FEATURE_TYPE*>(mem);

    for (const auto& w : weights) {
      if (w == 0) {
        throw std::runtime_error("0 weight");
      }
    }

    std::copy(weights.begin(), weights.end(), data);
}

uint64_t LRGradient::getSerializedSize() const {
    return weights.size() * sizeof(FEATURE_TYPE) + sizeof(uint32_t);
}

void LRGradient::print() const {
    std::cout << "Printing LRGradient. version: " << version << std::endl;
    for (const auto &v : weights) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

void LRGradient::check_values() const {
    for (const auto& w : weights) {
        if (std::isnan(w) || std::isinf(w)) {
            throw std::runtime_error("LRGradient::check_values error");
        }
    }
}

/** 
  * SOFTMAX
  *
  *
  */

SoftmaxGradient::SoftmaxGradient(uint64_t nclasses, uint64_t d) {
    weights.resize(d);
    for (auto& v : weights) {
        v.resize(nclasses);
    }
}

SoftmaxGradient::SoftmaxGradient(const std::vector<std::vector<FEATURE_TYPE>>& w) {
    weights = w;
}

void SoftmaxGradient::serialize(void* mem) const {
    *reinterpret_cast<uint32_t*>(mem) = version;
    mem = reinterpret_cast<void*>(
            (reinterpret_cast<char*>(mem) + sizeof(uint32_t)));
    FEATURE_TYPE* data = reinterpret_cast<FEATURE_TYPE*>(mem);

    for (const auto& v : weights) {
        std::copy(v.begin(), v.end(), data);
        data += v.size();
    }
}

uint64_t SoftmaxGradient::getSerializedSize() const {
    return weights.size() * weights[0].size() * sizeof(FEATURE_TYPE)
        + sizeof(uint32_t);
}

void SoftmaxGradient::loadSerialized(const void* mem) {
    version = *reinterpret_cast<const uint32_t*>(mem);
    mem = reinterpret_cast<const void*>(
            (reinterpret_cast<const char*>(mem) + sizeof(uint32_t)));
    const FEATURE_TYPE* data = reinterpret_cast<const FEATURE_TYPE*>(mem);

    for (auto& v : weights) {
        std::copy(data, data + v.size(), v.begin());
        data += v.size();
    }
}

void SoftmaxGradient::print() const {
    std::cout
        << "SoftmaxGradient (" << weights.size() << "x"
        << weights[0].size() << "): " << std::endl;
    for (const auto &v : weights) {
        for (const auto &vv : v) {
            std::cout << vv << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void SoftmaxGradient::check_values() const {
    for (const auto &v : weights) {
        for (const auto &vv : v) {
            if (std::isnan(vv) || std::isinf(vv)) {
                throw std::runtime_error("SoftmaxGradient::check_values error");
            }
        }
    }
}

/** 
  * MFGradient
  *
  */

MFGradient::MFGradient(uint64_t nclasses, uint64_t d) {
    weights.resize(d);
    for (auto& v : weights) {
        v.resize(nclasses);
    }
}

MFGradient::MFGradient(const std::vector<std::vector<FEATURE_TYPE>>& w) {
    weights = w;
}

void MFGradient::serialize(void* mem) const {
    *reinterpret_cast<uint32_t*>(mem) = version;
    mem = reinterpret_cast<void*>(
            (reinterpret_cast<char*>(mem) + sizeof(uint32_t)));
    FEATURE_TYPE* data = reinterpret_cast<FEATURE_TYPE*>(mem);

    for (const auto& v : weights) {
        std::copy(v.begin(), v.end(), data);
        data += v.size();
    }
}

uint64_t MFGradient::getSerializedSize() const {
    return weights.size() * weights[0].size() * sizeof(FEATURE_TYPE)
        + sizeof(uint32_t);
}

void MFGradient::loadSerialized(const void* mem) {
    version = *reinterpret_cast<const uint32_t*>(mem);
    mem = reinterpret_cast<const void*>(
            (reinterpret_cast<const char*>(mem) + sizeof(uint32_t)));
    const FEATURE_TYPE* data = reinterpret_cast<const FEATURE_TYPE*>(mem);

    for (auto& v : weights) {
        std::copy(data, data + v.size(), v.begin());
        data += v.size();
    }
}

void MFGradient::print() const {
    std::cout
        << "MFGradient (" << weights.size() << "x"
        << weights[0].size() << "): " << std::endl;
    for (const auto &v : weights) {
        for (const auto &vv : v) {
            std::cout << vv << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void MFGradient::check_values() const {
    for (const auto &v : weights) {
        for (const auto &vv : v) {
            if (std::isnan(vv) || std::isinf(vv)) {
                throw std::runtime_error("MFGradient::check_values error");
            }
        }
    }
}
