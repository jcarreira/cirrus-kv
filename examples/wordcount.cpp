/* Copyright Joao Carreira 2016 */

#include <stdlib.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <cctype>

#include "src/client/BladeClient.h"
#include "src/utils/easylogging++.h"

/*
 * Wordcount
 */

#define my_isalpha(a) ( ((a) >= 'A' && (a) <= 'Z') || \
        ((a) >= 'a' && (a) <= 'z'))

INITIALIZE_EASYLOGGINGPP

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";

// wordcount map
std::map<std::string, uint64_t> wc;

int main() {
    char* data = reinterpret_cast<char*>(malloc(10 * GB));
    if (!data)
        return 0;

    std::ifstream input("wordc.txt", std::ios::binary);

    std::copy(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>(),
            data);

    std::cout << "Copying done" << std::endl;

    sirius::BladeClient client;
    client.connect("10.10.49.87", PORT);
    sirius::AllocRec alloc1 = client.allocate(1 * GB);
    client.write_sync(alloc1, 0, 10 * GB, data);

    uint64_t index = 0;
    uint64_t count = 0;

    while (data[index]) {
        while (data[index] && !my_isalpha(data[index]))
            ++index;

        uint64_t start_of_word = index;
        while (data[index] && my_isalpha(data[index]))
            index++;
        count++;
    }

    std::cout << "Words counted" << std::endl;

    return 0;
}
