#ifndef _STATS_H_
#define _STATS_H_

#include <vector>

namespace sirius {

class Stats {
public:
    Stats();

    void add(double d);

    double avg() const;
    double size() const;
    double sd() const;
    double getPercentile(double p) const;
    double total() const;

    int getCount() const;
private:
    mutable std::vector<double> data;


    double sum = 0;
    double sq_sum = 0;
    double count = 0;
};

}

#endif // _STATS_H_
