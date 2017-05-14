#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include <sstream>

namespace cirrus {

template<typename ... Params>
std::string to_string(Params&& ... param) {

    std::ostringstream ss;
    auto f = [](std::ostringstream& ss, const auto& arg) {
        ss << " " << arg;
    };

    __attribute__((unused))
    int dummy[] = { 0, ( (void) f(ss, std::forward<Params>(param)), 0) ... }; 

    return ss.str();
}

template<typename T>
T string_to(const std::string& s) {
    std::istringstream iss(s);
    T v;
    iss >> v;
    return v;
}

} // namespace cirrus

#endif
