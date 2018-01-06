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
        INT_TYPE k, const std::string& v, INT_TYPE substr_start,
        INT_TYPE substr_len) {
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
        _write_buffer[k] += v.substr(substr_start, substr_len) + "\n";
        _write_buffer_sizes[k]++;

        _write_mutexes[k]->unlock();
}

INT_TYPE hash_lambda::next_counter() {
        return (_read_counter += config_instance::hash_bulk_get)
                - config_instance::hash_bulk_get;
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

void hash_lambda::add_get_data(long double b, long double t) {
        _stats_lock.lock();

        _get_bandwidths.push_back(std::make_pair(b, t));

        _stats_lock.unlock();
}

void hash_lambda::print_avg_stats() {
        long double avg = 0, get_avg = 0;
        for (std::pair<long double, long double> p : _thread_bandwidths)
                avg += (p.first / 1e6) / (p.second / 1e6);

        for (std::pair<long double, long double> p : _get_bandwidths)
                get_avg += (p.first / 1e6) / (p.second / 1e6);

        std::string to_print = "Hashing process " +
                std::to_string(_process_index) + " total hash rate: " +
                std::to_string(avg) + " MB/s\n";
        to_print += "Hashing process " + std::to_string(_process_index) +
                " average hash rate: " +
                std::to_string(avg / _thread_bandwidths.size()) + " MB/s\n";
        to_print += "Hashing process " + std::to_string(_process_index) +
                " total get rate: " + std::to_string(get_avg) + " MB/s\n";
        to_print += "Hashing process " + std::to_string(_process_index) +
                " average get rate: " +
                std::to_string(get_avg / _get_bandwidths.size()) + " MB/s\n";

        std::cout << to_print;
}

static inline INT_TYPE hash_helper(const std::string& s, INT_TYPE start) {
        INT_TYPE ret = 0;
        for (INT_TYPE i = 0; i < config_instance::hash_bytes; i++)
                ret = config_instance::chars * ret +
                        config_instance::key_map[s[start + i]];
        return ret;
}

void hasher(std::shared_ptr<hash_lambda> hl,
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                store) {
        long double range = (long double) std::pow(config_instance::chars,
                config_instance::hash_bytes) /
                (long double) config_instance::sort_nodes;

        INT_TYPE curr, counter;
        std::string* data_array = new std::string
                [config_instance::hash_bulk_get];

        while ((curr = hl->next_counter()) < hl->end()) {
                auto get_start = std::chrono::high_resolution_clock::now();
                INT_TYPE start = curr,
                        end = std::min(curr + config_instance::hash_bulk_get,
                                hl->end()) - 1;
                store->get_bulk(start, end, data_array);
                auto get_end = std::chrono::high_resolution_clock::now();
                long double get_time = std::chrono::duration_cast
                        <std::chrono::microseconds>(get_end - get_start)
                        .count();
                long double bytes = 0;

                auto hash_start = std::chrono::high_resolution_clock::now();
                for (INT_TYPE _ = 0; _ < end - start + 1; _++) {
                        const std::string& chunk = data_array[_];
                        bytes += chunk.size();
                        for (INT_TYPE i = 0;
                                i < config_instance::read_chunk_size &&
                                (i + 1) * (config_instance::record_size + 1) <=
                                        chunk.size(); i++, counter++) {
                                INT_TYPE h = (long double) hash_helper(chunk,
                                        i * (config_instance::record_size + 1))
                                        / range;
                                hl->write(store, h, chunk,
                                        i * (config_instance::record_size + 1),
                                        config_instance::record_size);
                        }
                }
                auto hash_end = std::chrono::high_resolution_clock::now();
                long double hash_time = std::chrono::duration_cast
                        <std::chrono::microseconds>
                                (hash_end - hash_start).count();
                hl->add_data(bytes, hash_time);
                hl->add_get_data(bytes, get_time);
                store->removeBulk(start, end);
        }

        delete[] data_array;
}
}  // namespace cirrus_terasort
