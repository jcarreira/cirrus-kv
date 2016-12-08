/* Copyright Joao Carreira 2016 */

#include <stdlib.h>
#include <omp.h>
#include <google/dense_hash_map>
#include <cstdint>
#include <cctype>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <cstring>

#include "src/client/BladeClient.h"
#include "src/utils/TimerFunction.h"
#include "src/utils/logging.h"

/*
 * Wordcount
 */

#define my_isalpha(a) ( ((a) >= 'A' && (a) <= 'Z') || \
        ((a) >= 'a' && (a) <= 'z'))

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";

uint32_t MurmurHash1(const void* key, int len, uint32_t seed) {
    const unsigned int m = 0xc6a4a793;
    const int r = 16;
    unsigned int h = seed ^ (len * m);
    const unsigned char * data = (const unsigned char *)key;

    while (len >= 4) {
        unsigned int k = *(unsigned int *)data;
        h += k;
        h *= m;
        h ^= h >> 16;
        data += 4;
        len -= 4;
    }
    switch (len) {
        case 3:
            h += data[2] << 16;
        case 2:
            h += data[1] << 8;
        case 1:
            h += data[0];
            h *= m;
            h ^= h >> r;
    }
    h *= m;
    h ^= h >> 10;
    h *= m;
    h ^= h >> 17;
    return h;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

class MyString {
 public:
    explicit MyString(uint64_t a = 0, uint64_t b = 0, char* data = 0) :
        a_(a), b_(b), data_(data), val_(0) {
            if (data) {
                val_ = MurmurHash1(data_ + a_, b_ - a_, 42);
            }
        }

    bool operator<(const MyString& ms) const {
        int l1 = b_ - a_, l2 = ms.b_ - ms.a_;
        int min_l = MIN(l1, l2);
        int cmp = strncmp(data_ + a_, ms.data_ + ms.a_, min_l);
        if (cmp == 0) {
            if (l1 == l2) {
                return 0;
            } else {
                return l1 < l2;
            }
        } else {
            return cmp < 0;
        }
    }

    bool operator==(const MyString& ms) const {
        int l1 = b_ - a_, l2 = ms.b_ - ms.a_;
        if (l1 != l2)
            return false;

        bool ret = (strncmp(data_ + a_, ms.data_ + ms.a_, l1) == 0);
        return ret;
    }

    uint64_t a_;
    uint64_t b_;
    char* data_;
    size_t val_;
};

namespace std {

template<>
struct hash<MyString> {
    std::size_t operator()(MyString s) const {
        return s.val_;
    }
};

}  // namespace std

size_t get_file_size(std::string fname) {
    std::ifstream file(fname, std::ios::binary | std::ios::ate);
    return file.tellg();
}

int main() {
    google::dense_hash_map<MyString, int> wc2[50];
    for (int i = 0; i < 50; ++i)
        wc2[i].set_empty_key(MyString());

    std::string file = "wordc2.txt";
    size_t file_size = get_file_size(file);
    std::ifstream input(file, std::ios::binary);

    char* data = reinterpret_cast<char*>(malloc(file_size));
    if (!data)
        return 0;


    std::copy(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>(),
            data);

    sirius::LOG<sirius::INFO>("Copying done");

    sirius::LOG<sirius::INFO>("Data written");

    uint64_t count = 0;

#pragma omp parallel for num_threads(16)
     for (int i = 0; i < 16; ++i) {
        int tid = omp_get_thread_num();
        uint64_t size_partition = file_size / omp_get_num_threads();
        uint64_t l = tid * size_partition;
        uint64_t r = l + size_partition;
        uint64_t index = l;


        sirius::LOG<sirius::INFO>("Running thread ", omp_get_thread_num());
        sirius::LOG<sirius::INFO>("Num threads ", omp_get_num_threads());

        sirius::TimerFunction tf("Traverse data", true);

        while (index < r && data[index]) {
            while (index < r && data[index] == ' ')
                ++index;

            uint64_t start_of_word = index;
            while (index < r && data[index] && data[index] != ' ')
                index++;
            uint64_t last_of_word = index;
            MyString str(start_of_word, last_of_word, data);
            wc2[i][str]++;
        }
    }

    sirius::LOG<sirius::INFO>("Words counted");
    sirius::LOG<sirius::INFO>("count: ", count);

    return 0;
}
