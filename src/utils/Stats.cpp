#include "src/utils/Stats.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <cassert>

namespace sirius {

Stats::Stats() {

}

void Stats::add(double d){
    sum += d;
    sq_sum += d * d;
    count++;
    data.push_back(d);
}

double Stats::avg() const {
    return sum / count;
}

double Stats::size() const {
    return count;
}

double Stats::sd() const {
    return sqrt(sq_sum / count - avg() * avg());
}

double Stats::total() const {
    return sum;
}

int Stats::getCount() const {
    return count;
}

double Stats::getPercentile(double p) const {
    if(data.size() == 0)
        throw std::runtime_error("No data");

    std::sort(data.begin(), data.end());
    uint32_t loc = static_cast<int>(p * data.size());
    assert(loc < data.size());
    return data[loc];
}

} // namespace sirius

