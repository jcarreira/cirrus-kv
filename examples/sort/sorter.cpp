/**
  * The worker that actually does the sorting
  */

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

#include "object_store/FullBladeObjectStore.h"
#include "client/TCPClient.h"

#define BATCH_SIZE 1000
#define STRING_SIZE 10

#define IP

class StringSerializer : public cirrus::Serializer<std::string> {
 public:
    uint64_t size(const std::string& s) const override {
        return s.size();
    }
    void serialize(const std::string& s, void* mem) const override {
        std::memcpy(mem, s.data(), s.size());
    }
};

class StringDeserializer {
 public:
    std::string operator()(const void* data, uint64_t size) {
        return std::string((char*)data, size);
    }
};

std::string generate_rand_string() {
    static const char alphanum[] =
        "0123456789"
        "!@#$%^&*"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string ret;
    ret.resize(STRING_SIZE);
    for (uint64_t i = 0; i < STRING_SIZE; ++i) {
        ret[i] = alphanum[rand() % sizeof(alphanum) - 1];
    }
    return ret;
}

void load_random_data(
        cirrus::ostore::FullBladeObjectStoreTempl<std::string>& store,
        uint64_t size) {
    std::vector<std::string> batch(BATCH_SIZE);
    std::vector<uint64_t> oids(BATCH_SIZE);

    for (uint64_t j = 0; j < size; j+= BATCH_SIZE) {
        for (uint64_t i = 0; i < BATCH_SIZE; ++i) {
            batch[i] = generate_rand_string();
            oids[i] = j + i;
        }
        store.put_bulk(j, j + BATCH_SIZE - 1, batch.data());
    }
}

void print_arguments() {
    std::cerr << "./sort ip port" << std::endl;
}

int main(int argc, char*argv[]) {
    char* ip = nullptr;
    char* port = nullptr;
    char c = 0;

    while ((c = getopt(argc, argv, "i:p:")) != -1) {
        switch (c) {
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = optarg;
                break;
        }
    }

    if (ip == nullptr || port == nullptr) {
        print_arguments();
        exit(-1);
    }

    std::cout << "Using ip: " << ip
        << " and port: " << port
        << std::endl;

    StringSerializer ss;
    StringDeserializer sd;
    cirrus::TCPClient client;
    cirrus::ostore::FullBladeObjectStoreTempl<std::string>
        store(ip, port, &client, ss, sd);

    load_random_data(store, 100000);
}

