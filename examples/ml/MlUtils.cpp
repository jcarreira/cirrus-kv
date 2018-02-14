#include <examples/ml/MlUtils.h>
#include <cmath>
#include <stdexcept>
#include <string>
#include <Utils.h>

namespace mlutils {

float s_1_float(float x) {
    float res = 1.0 / (1.0 + exp(-x));
    if (std::isnan(res) || std::isinf(res)) {
        throw std::runtime_error(
                std::string("s_1_float generated nan/inf x: " + std::to_string(x)
                    + " res: " + std::to_string(res)));
    }

    return res;
}

double log_aux(double v) {
    if (v == 0) {
        return -1000000;
    }

    double res = log(v);
    if (std::isnan(res) || std::isinf(res)) {
        throw std::runtime_error(
            std::string("log_aux generated nan/inf v: ") +
            to_string(v));
    }

    return res;
}

}  // namespace mlutils
