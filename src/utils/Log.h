#ifndef SRC_UTILS_LOG_H_
#define SRC_UTILS_LOG_H_

#include <iostream>
#include <utility>
#include "utils/CirrusTime.h"
#include "utils/StringUtils.h"
#include "common/Synchronization.h"

#define INIT_SWITCH (1)
// by default logging is off
#define DEFAULT_SWITCH (0)

namespace cirrus {

enum { INFO_TAG = 1, ERROR_TAG = 2, PERF_TAG = 3};

struct INFO {
    static const int value = 1;
    constexpr static const char* prefix = "[INFO]";
};
struct ERROR {
    static const int value = 2;
    constexpr static const char* prefix = "[ERROR]";
};
struct PERF {
    static const int value = 3;
    constexpr static const char* prefix = "[PERF]";
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
    static int INFO_SWITCH  = INIT_SWITCH;
    static int ERROR_SWITCH = INIT_SWITCH;
    static int PERF_SWITCH  = INIT_SWITCH;
    if (INFO_SWITCH == INIT_SWITCH) {
        INFO_SWITCH  = DEFAULT_SWITCH;
        ERROR_SWITCH = DEFAULT_SWITCH;
        PERF_SWITCH  = DEFAULT_SWITCH;
        if (const char* env = std::getenv("CIRRUS_LOG")) {
            int v = string_to<int>(env);
            if (v) {
                INFO_SWITCH = v;
                ERROR_SWITCH = v;
            }
        }
        if (const char* env = std::getenv("CIRRUS_PERF")) {
            int v = string_to<int>(env);
            if (v) {
                PERF_SWITCH = v;
            }
        }
    }

    if ( (T::value == INFO_TAG  && INFO_SWITCH <= 0) ||
         (T::value == ERROR_TAG && ERROR_SWITCH <= 0) ||
         (T::value == PERF_TAG  && PERF_SWITCH <= 0)) {
        return true;
    }

    std::cout << T::prefix
        << " -";

#if __GNUC__ >= 7
    if constexpr (std::is_same_v<TT, TIME>) {
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
    if constexpr (std::is_same_v<K, FLUSH>) {
        std::cout << std::endl;
    }
#else
    std::cout << std::endl;
#endif

    return true;  // success
}

}  // namespace cirrus

#endif  // SRC_UTILS_LOG_H_
