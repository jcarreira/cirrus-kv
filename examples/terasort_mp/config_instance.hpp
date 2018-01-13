#ifndef EXAMPLES_TERASORT_MP_CONFIG_INSTANCE_HPP_
#define EXAMPLES_TERASORT_MP_CONFIG_INSTANCE_HPP_

#include <cstdint>
#include <unordered_map>

namespace cirrus_terasort {

using INT_TYPE = uint64_t;

namespace config_instance {

static std::unordered_map<char, INT_TYPE> key_map = {
        { '0', 0 }, { '1', 1 }, { '2', 2 }, {'3', 3 }, { '4', 4 }, { '5', 5 },
        { '6', 6 }, { '7', 7 }, { '8', 8 }, { '9', 9 },

        { 'A', 10 }, { 'B', 11 }, { 'C', 12 }, { 'D', 13 }, { 'E', 14 },
        { 'F', 15 }, { 'G', 16 }, { 'H', 17 }, { 'I', 18 },
        { 'J', 19 }, { 'K', 20 }, { 'L', 21 }, { 'M', 22 }, { 'N', 23 },
        { 'O', 24 }, { 'P', 25 }, { 'Q', 26 }, { 'R', 27 },
        { 'S', 28 }, { 'T', 29 }, { 'U', 30 }, { 'V', 31 }, { 'W', 32 },
        { 'X', 33 }, { 'Y', 34 }, { 'Z', 35 },

        { 'a', 36 }, { 'b', 37 }, { 'c', 38 }, { 'd', 39 }, { 'e', 40 },
        { 'f', 41 }, { 'g', 42 }, { 'h', 43 }, { 'i', 44 },
        { 'j', 45 }, { 'k', 46 }, { 'l', 47 }, { 'm', 48 }, { 'n', 49 },
        { 'o', 50 }, { 'p', 51 }, { 'q', 52 }, { 'r', 53 },
        { 's', 54 }, { 't', 55 }, { 'u', 56 }, { 'v', 57 }, { 'w', 58 },
        { 'x', 59 }, { 'y', 60 }, { 'z', 61 },
};

static constexpr INT_TYPE total_processes = 37;  // must match runtime
static constexpr INT_TYPE hash_nodes = 4;
static constexpr INT_TYPE hash_bytes = 1;
static constexpr INT_TYPE hash_threads = 8;
static constexpr INT_TYPE hash_bulk_transfer = 50;
static constexpr char hash_output_dir[] = "/nscratch/nathreya/hash_output_dir";

static constexpr char cirrus_port[] = "12345";
static constexpr char cirrus_ip[] = "10.10.49.85";

static constexpr INT_TYPE record_size = 100;
static constexpr INT_TYPE num_records = 1'000'000'000;

static constexpr const char* input_files[] =
        { "/nscratch/nathreya/terasort_data_100GB.txt" };
static constexpr INT_TYPE num_input_files = 1;
static constexpr INT_TYPE read_chunk_size = 1000;
static constexpr INT_TYPE read_threads = 16;
static constexpr INT_TYPE total_read_keys = num_records / read_chunk_size;

static constexpr INT_TYPE sort_nodes = 32;
static constexpr INT_TYPE sort_threads = 2;
static constexpr INT_TYPE sort_bulk_transfer = 50;
static constexpr INT_TYPE sort_transfer_wait_ms = 1000;
static constexpr char sort_output_dir[] = "/nscratch/nathreya/sort_output_dir";

static constexpr INT_TYPE chars = 62;
static constexpr char sentinel[] = "$";
static constexpr char stats[] = "/nscratch/nathreya/stats";

static_assert(num_records % read_chunk_size == 0, "config error");
static_assert((num_records / read_chunk_size) % hash_nodes == 0,
        "config error");
static_assert(total_processes == hash_nodes + sort_nodes + num_input_files,
        "config error");
}  // namespace config_instance
}  // namespace cirrus_terasort

#endif  // EXAMPLES_TERASORT_MP_CONFIG_INSTANCE_HPP_
