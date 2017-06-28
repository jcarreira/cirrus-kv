#ifndef SRC_UTILS_LOG_H_
#define SRC_UTILS_LOG_H_

#include <iostream>
#include <utility>
#include "utils/Time.h"
#include "utils/StringUtils.h"
#include "common/Synchronization.h"

#define DEFAULT_THRESHOLD 10

namespace cirrus {

struct INFO {
    static const int value = 5;
    constexpr static const char* prefix = "[INFO]";
};
struct ERROR {
    static const int value = 10;
    constexpr static const char* prefix = "[ERROR]";
};

#if __GNUC__ >= 7
struct FLUSH { };
struct NO_FLUSH { };
struct TIME { };
struct NO_TIME { };

template<typename T,
          typename K = NO_FLUSH,
          typename TT = TIME,
          typename ... Params>
#else
template<typename T, typename ... Params>
#endif
bool LOG(Params&& ... param) {
    static int THRESHOLD = -1;
    if (THRESHOLD == -1) {
        if (const char* env = std::getenv("CIRRUS_LOG")) {
            int v = string_to<int>(env);
            if (v) {
                THRESHOLD = v;
            } else {
                THRESHOLD = DEFAULT_THRESHOLD;
            }
        } else {
            // if env not defined we default to not show logs
            THRESHOLD = DEFAULT_THRESHOLD;
        }
    }

    if (T::value < THRESHOLD)
        return true;

    std::cout << T::prefix
        << " -";

#if __GNUC__ >= 7
    if constexpr (std::is_same(TT, TIME)) {
        std::cout << getTimeNow();
    }
#else
    std::cout << getTimeNow();
#endif

    auto f = [](const auto& arg) {
        std::cout << " " << arg;
    };

     __attribute__((unused))
    int dummy[] = { 0, ( (void) f(std::forward<Params>(param)), 0) ... };

#if __GNUC__ >= 7
    if constexpr (std::is_same(K, FLUSH)) {
        std::cout << std::endl;
    }
#else
    std::cout << std::endl;
#endif

    return true;  // success
}

}  // namespace cirrus

#endif  // SRC_UTILS_LOG_H_
