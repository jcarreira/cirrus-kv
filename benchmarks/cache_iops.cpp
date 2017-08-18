#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <cctype>
#include <chrono>
#include <thread>
#include <random>
#include <memory>
#include <cmath>

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"
#include "utils/CirrusTime.h"
#include "utils/Stats.h"
#include "client/TCPClient.h"

#include "cache_manager/LRAddedEvictionPolicy.h"
#include "cache_manager/CacheManager.h"

// TODO(Tyler): Remove hardcoded IP and PORT
static const uint64_t KB = 1024;
// Size of items being fetched
static const uint64_t SIZE = 4 * KB;
// Capacity of the cache
static const uint64_t CACHE_SIZE = 500;
const char PORT[] = "12345";
const char IP[] = "127.0.0.1";
static const uint64_t MILLION = 1000000;
// Number of times items will be fetched from the store/ cache
static const uint64_t N_ITER = MILLION;
// Number of items that exist
static const uint64_t N_ITEMS = 10000;

void print_stats(std::ostream& out, uint64_t msg_sent, uint64_t elapsed_us,
    uint64_t size, bool is_cache) {
    if (is_cache) {
        out << "Results with caching" << std::endl;
    } else {
        out << "Results without caching" << std::endl;
    }
    out << "IO/s: " << (msg_sent * 1.0 / elapsed_us * MILLION)
        << std::endl;
    out << "bytes/s: " << (size * msg_sent * 1.0
        / elapsed_us * MILLION) << std::endl;
    out << std::endl;
}

void print_stats_zipf(std::ostream& out, uint64_t msg_sent, uint64_t elapsed_us,
    uint64_t size, bool is_cache) {
    if (is_cache) {
        out << "Zipf results with caching" << std::endl;
    } else {
        out << "Zipf results without caching" << std::endl;
    }
    out << "IO/s: " << (msg_sent * 1.0 / elapsed_us * MILLION)
        << std::endl;
    out << "bytes/s: " << (size * msg_sent * 1.0
        / elapsed_us * MILLION) << std::endl;
    out << std::endl;
}

/**
  * This benchmark aims to find the bandwidth of the cache vs the store.
  * It makes use of caching and a skewed random number generator to
  * demonstrate the benefits of caching heavily used IDs.
  */
void test_iops(std::ofstream& outfile) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(CACHE_SIZE);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, CACHE_SIZE);


    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < N_ITEMS; ++i) {
        store.put(i, d);
    }

    // Populate the vector if items to fetch
    std::vector<uint64_t> to_fetch(N_ITER);

    // Modify these parameters to change the distribution
    int mean = N_ITEMS/2;
    int std_dev = N_ITEMS/16;
    std::normal_distribution<> dist(mean, std_dev);

    std::random_device rd;
    std::mt19937 gen(rd());

    for (uint64_t i = 0; i < N_ITER; i++) {
        to_fetch[i] = std::round(dist(gen));
    }

    std::cout << "Warm up done" << std::endl;

    std::cout << "Measuring msgs/s.. w/ cache" << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;

    for (; i < N_ITER; i++) {
        cm.get(to_fetch[i]);
    }

    uint64_t elapsed_us = start.getUsElapsed();


    print_stats(outfile, i, elapsed_us, SIZE, true);

    print_stats(std::cout, i, elapsed_us, SIZE, true);


    std::cout << "Measuring msgs/s.. w/o cache" << std::endl;

    i = 0;
    cirrus::TimerFunction store_start;

    for (; i < N_ITER; i++) {
        store.get(to_fetch[i]);
    }

    uint64_t elapsed_us_store = store_start.getUsElapsed();
    print_stats(outfile, i, elapsed_us_store, SIZE, false);

    print_stats(std::cout, i, elapsed_us_store, SIZE, false);
}

/**
 * Zipf-like random distribution.
 * "Rejection-inversion to generate variates from monotone discrete
 * distributions", Wolfgang Hörmann and Gerhard Derflinger
 * ACM TOMACS 6.3 (1996): 169-184
 */
template<class IntType = uint64_t, class RealType = double>
class zipf_distribution {
 public:
    typedef RealType input_type;
    typedef IntType result_type;

    static_assert(std::numeric_limits<IntType>::is_integer, "");
    static_assert(!std::numeric_limits<RealType>::is_integer, "");

    zipf_distribution(const IntType n = std::numeric_limits<IntType>::max(),
                      const RealType q = 1.0)
        : n(n)
        , q(q)
        , H_x1(H(1.5) - 1.0)
        , H_n(H(n + 0.5))
        , dist(H_x1, H_n) {}

    IntType operator()(std::mt19937 *rng) {
        while (true) {
            const RealType u = dist(*rng);
            const RealType x = H_inv(u);
            const IntType  k = clamp<IntType>(std::round(x), 1, n);
            if (u >= H(k + 0.5) - h(k)) {
                return k;
            }
        }
    }

 private:
    /** Clamp x to [min, max]. */
    template<typename T>
    static constexpr T clamp(const T x, const T min, const T max) {
        return std::max(min, std::min(max, x));
    }

    /** exp(x) - 1 / x */
    static double
    expxm1bx(const double x) {
        return (std::abs(x) > epsilon)
            ? std::expm1(x) / x
            : (1.0 + x/2.0 * (1.0 + x/3.0 * (1.0 + x/4.0)));
    }

    /** H(x) = log(x) if q == 1, (x^(1-q) - 1)/(1 - q) otherwise.
     * H(x) is an integral of h(x).
     *
     * Note the numerator is one less than in the paper order to work with all
     * positive q.
     */
    const RealType H(const RealType x) {
        const RealType log_x = std::log(x);
        return expxm1bx((1.0 - q) * log_x) * log_x;
    }

    /** log(1 + x) / x */
    static RealType
    log1pxbx(const RealType x) {
        return (std::abs(x) > epsilon)
            ? std::log1p(x) / x
            : 1.0 - x * ((1/2.0) - x * ((1/3.0) - x * (1/4.0)));
    }

    /** The inverse function of H(x) */
    const RealType H_inv(const RealType x) {
        const RealType t = std::max(-1.0, x * (1.0 - q));
        return std::exp(log1pxbx(t) * x);
    }

    /** That hat function h(x) = 1 / (x ^ q) */
    const RealType h(const RealType x) {
        return std::exp(-q * std::log(x));
    }

    static constexpr RealType epsilon = 1e-8;

    IntType                                  n;     ///< Number of elements
    RealType                                 q;     ///< Exponent
    RealType                                 H_x1;  ///< H(x_1)
    RealType                                 H_n;   ///< H(n)
    std::uniform_real_distribution<RealType> dist;  ///< [H(x_1), H(n)]
};

/**
  * This benchmark aims to find the bandwidth of the cache vs the store.
  * It makes use of caching and a skewed random number generator to
  * demonstrate the benefits of caching heavily used IDs.
  */
void test_iops_zipf(std::ofstream& outfile) {
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<cirrus::Dummy<SIZE>>
        store(IP, PORT, &client,
            cirrus::serializer_simple<cirrus::Dummy<SIZE>>,
            cirrus::deserializer_simple<cirrus::Dummy<SIZE>,
                sizeof(cirrus::Dummy<SIZE>)>);

    cirrus::LRAddedEvictionPolicy policy(CACHE_SIZE);
    cirrus::CacheManager<cirrus::Dummy<SIZE>> cm(&store, &policy, CACHE_SIZE);


    struct cirrus::Dummy<SIZE> d(42);

    // warm up
    std::cout << "Warming up" << std::endl;
    for (uint64_t i = 0; i < N_ITEMS; ++i) {
        store.put(i, d);
    }

    // Populate the vector if items to fetch
    std::vector<uint64_t> to_fetch(N_ITER);

    std::random_device rd;
    std::mt19937 gen(rd());

    zipf_distribution<uint64_t, double> distribution(N_ITEMS - 1);

    for (uint64_t i = 0; i < N_ITER; i++) {
        to_fetch[i] = std::round(distribution(&gen));
    }

    std::cout << "Warm up done" << std::endl;

    std::cout << "Measuring msgs/s.. w/ cache" << std::endl;
    uint64_t i = 0;
    cirrus::TimerFunction start;

    for (; i < N_ITER; i++) {
        cm.get(to_fetch[i]);
    }

    uint64_t elapsed_us = start.getUsElapsed();


    print_stats_zipf(outfile, i, elapsed_us, SIZE, true);

    print_stats_zipf(std::cout, i, elapsed_us, SIZE, true);


    std::cout << "Measuring msgs/s.. w/o cache" << std::endl;

    i = 0;
    cirrus::TimerFunction store_start;

    for (; i < N_ITER; i++) {
        store.get(to_fetch[i]);
    }

    uint64_t elapsed_us_store = store_start.getUsElapsed();
    print_stats_zipf(outfile, i, elapsed_us_store, SIZE, false);

    print_stats_zipf(std::cout, i, elapsed_us_store, SIZE, false);
}

auto main() -> int {
    // test burst of sync writes
    std::ofstream outfile;
    outfile.open("cache_iops.log");
    test_iops(outfile);
    test_iops_zipf(outfile);
    outfile.close();

    return 0;
}
