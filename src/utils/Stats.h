#ifndef SRC_UTILS_STATS_H_
#define SRC_UTILS_STATS_H_

#include <vector>
#include <cstdint>

namespace cirrus {

class Stats {
 public:
    Stats();

    void add(double d);

    double min() const;
    double max() const;
    double avg() const;
    double size() const;
    double sd() const;
    double getPercentile(double p) const;
    double total() const;

    int getCount() const;

    void reserve(uint64_t);

    void print() const;

 private:
    mutable std::vector<double> data;

    double sum = 0;
    double sq_sum = 0;
    double count = 0;
};

}  // namespace cirrus

#endif  // SRC_UTILS_STATS_H_
