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
#include "third_party/easylogging++.h"
#include "src/utils/TimerFunction.h"
#include "src/object_store/FullBladeObjectStore.h"

static const uint64_t GB = (1024*1024*1024);
const char PORT[] = "12345";

INITIALIZE_EASYLOGGINGPP

struct Dummy {
    char data[15];
    int id;
};

int main() {

    sirius::FullBladeObjectStore store("10.10.49.87", PORT);

    void* d = (Dummy*)new Dummy;
    ((Dummy*)d)->id = 42;
    store.put(&d, sizeof(Dummy), 1);

    void *d2;
    store.get(1, d2);

    // should be 42
    std::cout << "d2.id: " << ((Dummy*)d2)->id << std::endl;

    return 0;
}

