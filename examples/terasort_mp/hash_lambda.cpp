#include "hash_lambda.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <string>

namespace cirrus_terasort {

hash_lambda::hash_lambda(INT_TYPE p, INT_TYPE s, INT_TYPE e) : _start(s),
        _end(e), _write_buffer(std::vector<std::string>{}),
        _write_buffer_sizes(std::vector<INT_TYPE>{}),
        _write_mutexes(std::vector<std::shared_ptr<std::mutex>>{}),
        _process_index(p), _counter_list(std::vector<INT_TYPE>{}) {
        _read_counter = _start;

        for (INT_TYPE i = 0; i < config_instance::sort_nodes; i++) {
                _counter_list.push_back(0);

                _write_buffer.push_back("");
                _write_buffer_sizes.push_back(0);
                _write_mutexes.push_back(std::make_shared<std::mutex>());
        }
}

void hash_lambda::finish(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                store) {
        for (INT_TYPE k = 0; k < config_instance::sort_nodes; k++) {
                INT_TYPE start_offset = (_process_index + 1) *
                        config_instance::total_read_keys + k;

                INT_TYPE key_offset = _counter_list[k]++ *
                        config_instance::sort_nodes;
                store->put(start_offset + key_offset, _write_buffer[k]);

                INT_TYPE key_offset2 = _counter_list[k]++ *
                        config_instance::sort_nodes;
                store->put(start_offset + key_offset2,
                        config_instance::sentinel);
        }
}

void hash_lambda::write(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                store,
        INT_TYPE k, std::string v) {
        if (k < 0 || k > config_instance::sort_nodes)
                throw std::runtime_error("hashing logic failed.");

        _write_mutexes[k]->lock();

        if (_write_buffer_sizes[k] >= config_instance::read_chunk_size) {
                INT_TYPE start_offset = (_process_index + 1) *
                        config_instance::total_read_keys + k;
                INT_TYPE key_offset = _counter_list[k]++ *
                        config_instance::sort_nodes;
                store->put(start_offset + key_offset, _write_buffer[k]);

                _write_buffer[k].clear();
                _write_buffer_sizes[k] = 0;
        }
        _write_buffer[k] += v + "\n";
        _write_buffer_sizes[k]++;

        _write_mutexes[k]->unlock();
}

INT_TYPE hash_lambda::next_counter() {
        return _read_counter++;
}

INT_TYPE hash_lambda::start() {
        return _start;
}

INT_TYPE hash_lambda::end() {
        return _end;
}

void hash_lambda::add_data(long double b, long double t) {
        _stats_lock.lock();

        _thread_bandwidths.push_back(std::make_pair(b, t));

        _stats_lock.unlock();
}

void hash_lambda::print_avg_stats() {
        long double total_bytes = 0, total_time = 0, avg = 0;
        for (std::pair<long double, long double> p : _thread_bandwidths) {
                avg += (p.first / 1e6) / (p.second / 1e3);
        }

        std::string to_print = "Hashing process " +
                std::to_string(_process_index) + " total hash rate: " +
                std::to_string(avg) + " MB/s\n";
        to_print += "Hashing process " + std::to_string(_process_index) +
                " average hash rate: " +
                std::to_string(avg / _thread_bandwidths.size()) + " MB/s\n";

        std::cout << to_print;
}

static inline INT_TYPE hash_helper(const std::string& s) {
        INT_TYPE ret = 0;
        for (INT_TYPE i = 0; i < config_instance::hash_bytes; i++)
                ret = config_instance::chars * ret +
                        config_instance::key_map[s[i]];
        return ret;
}

void hasher(std::shared_ptr<hash_lambda> hl,
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                store) {
        long double range = (long double) std::pow(config_instance::chars,
                config_instance::hash_bytes) /
                (long double) config_instance::sort_nodes;

        INT_TYPE curr, counter;
        auto hash_start = std::chrono::high_resolution_clock::now();
        while ((curr = hl->next_counter()) < hl->end()) {
                std::string chunk = store->get(curr);
                for (INT_TYPE i = 0; i < config_instance::read_chunk_size &&
                        (i + 1) * (config_instance::record_size + 1) <=
                                chunk.size(); i++, counter++) {
                        std::string str = chunk.substr(i *
                                (config_instance::record_size + 1),
                                        config_instance::record_size);
                        INT_TYPE h = (long double) hash_helper(str) / range;
                        hl->write(store, h, str);
                }
                store->remove(curr);
        }
        auto hash_end = std::chrono::high_resolution_clock::now();

        long double time =
                std::chrono::duration_cast<std::chrono::milliseconds>
                        (hash_end - hash_start).count(),
                bytes = counter * config_instance::record_size;
        hl->add_data(bytes, time);
}
}  // namespace cirrus_terasort
