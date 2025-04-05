#include "hash_lambda.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string>

namespace cirrus_terasort {

hash_lambda::hash_lambda(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                <std::string>> store,
        INT_TYPE p, INT_TYPE s, INT_TYPE e) : _start(s),
        _end(e), _write_buffer
                (std::vector<std::shared_ptr<std::vector<char*>>>{}),
        _write_buffer_indices
                (std::vector<std::shared_ptr<std::vector<INT_TYPE>>>{}),
        _write_buffer_sizes(std::vector<INT_TYPE>{}),
        _write_mutexes(std::vector<std::shared_ptr<std::mutex>>{}),
        _process_index(p), _counter_list(std::vector<INT_TYPE>{}),
        _background_buffer(std::queue<std::pair<
                std::shared_ptr<std::vector<INT_TYPE>>,
                std::shared_ptr<std::vector<char*>>
                >>{}), _finished(false),
        _background_threads(std::vector<std::thread>{}) {
        _read_counter = _start;

        for (INT_TYPE i = 0; i < config_instance::sort_nodes; i++) {
                _counter_list.push_back(0);

                _write_buffer.push_back(
                        std::make_shared<std::vector<char*>>());
                _write_buffer_indices.push_back(
                        std::make_shared<std::vector<INT_TYPE>>());
                for(INT_TYPE _ = 0; _ < config_instance::hash_bulk_transfer;
                        _++)
                        _write_buffer[i]->push_back((char*)
                                calloc((config_instance::record_size + 1)
                                * config_instance::read_chunk_size + 2,
                                sizeof(char)));
                _write_buffer_sizes.push_back(0);
                _write_mutexes.push_back(std::make_shared<std::mutex>());
        }
        
        for(INT_TYPE _ = 0; _ < config_instance::hash_num_background_threads;
                _++)
                _background_threads.push_back(std::thread(
                        &hash_lambda::background_buffer_writer, this, store));
}

void hash_lambda::background_buffer_writer(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                <std::string>> store) {
        while(!_finished || _background_buffer.size() > 0) {
                std::unique_lock<std::mutex> lock(_background_lock);
                while(_background_buffer.size() == 0) {
                        if(_finished) {
                                lock.unlock();
                                return;
                        }
                        _background_cv.wait_for(lock, std::chrono::milliseconds(
                                config_instance::hash_background_timeout_ms),
                                [this]() { return
                                        _finished ||
                                        _background_buffer.size() > 0; });
                        if(_finished && _background_buffer.size() == 0) {
                                lock.unlock();
                                return;
                        }
                }
                std::pair<std::shared_ptr<std::vector<INT_TYPE>>,
                        std::shared_ptr<std::vector<char*>>> p =
                        _background_buffer.front();

                _background_buffer.pop();
                
                lock.unlock();
                _background_cv.notify_one();

                std::vector<std::string> to_ins;
                for(const char* c : *p.second)
                        to_ins.push_back(c);

                store->put_bulk_fast(*p.first, to_ins);
                
                p.first.reset();
                for(char* c : *p.second)
                        free(c);
                p.second.reset();
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
                INT_TYPE key_offset2 = _counter_list[k]++ *
                        config_instance::sort_nodes;

                _write_buffer_indices[k]->push_back(start_offset + key_offset);
                std::vector<std::string> to_ins;
                for(INT_TYPE i = 0; i < _write_buffer_indices[k]->size(); i++)
                        to_ins.push_back((*_write_buffer[k])[i]);

                for(char* c : *_write_buffer[k])
                        free(c);

                _write_buffer_indices[k]->push_back(start_offset + key_offset2);
                to_ins.push_back(config_instance::sentinel);
                store->put_bulk_fast(*_write_buffer_indices[k], to_ins);
        }

        _finished = true;
        for(INT_TYPE _ = 0; _ < config_instance::hash_num_background_threads;
                _++)
                _background_threads[_].join();
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
                _write_buffer_indices[k]->push_back(start_offset +
                        key_offset);
                if(_write_buffer_indices[k]->size() >=
                        config_instance::hash_bulk_transfer) {
                        std::unique_lock<std::mutex> lock(_background_lock);
                        _background_buffer.push(std::make_pair(
                                _write_buffer_indices[k],
                                _write_buffer[k]));
                        lock.unlock();
                        _background_cv.notify_all();

                        _write_buffer_indices[k] =
                                std::make_shared<std::vector<INT_TYPE>>();

                        _write_buffer[k] =
                                std::make_shared<std::vector<char*>>();
                        for(INT_TYPE _ = 0;
                                _ < config_instance::hash_bulk_transfer; _++)
                                _write_buffer[k]->push_back((char*)
                                        calloc(
                                        (config_instance::record_size + 1)
                                        * config_instance::read_chunk_size + 2,
                                        sizeof(char)));
                }

                _write_buffer_sizes[k] = 0;
        }
        std::memcpy((*_write_buffer[k])[_write_buffer_indices[k]->size()] +
                _write_buffer_sizes[k] * (config_instance::record_size + 1),
                v.data() + substr_start, substr_len);
        (*_write_buffer[k])[_write_buffer_indices[k]->size()][
                _write_buffer_sizes[k] * (config_instance::record_size + 1)
                + substr_len] = '\n';
        _write_buffer_sizes[k]++;

        _write_mutexes[k]->unlock();
}

INT_TYPE hash_lambda::next_counter() {
        return (_read_counter += config_instance::hash_bulk_transfer)
                - config_instance::hash_bulk_transfer;
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


bool hasher_smart_get(std::shared_ptr<hash_lambda> hl,
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl
                <std::string>> store, const INT_TYPE& start,
        const INT_TYPE& end, std::string* data) {
        try {
                store->get_bulk(start, end, data);
                return true;
        }
        catch (...) {
                return false;
        }
}

void hasher(std::shared_ptr<hash_lambda> hl,
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                store) {
        long double range = (long double) std::pow(config_instance::chars,
                config_instance::hash_bytes) /
                (long double) config_instance::sort_nodes;

        INT_TYPE curr, counter;
        std::string* data_array = new std::string
                [config_instance::hash_bulk_transfer];

        while ((curr = hl->next_counter()) < hl->end()) {
                auto get_start = std::chrono::high_resolution_clock::now();
                INT_TYPE start = curr,
                        end = std::min(
                                curr + config_instance::hash_bulk_transfer,
                                hl->end()) - 1;
                while(!hasher_smart_get(hl, store, start, end, data_array))
                        std::this_thread::sleep_for(std::chrono::milliseconds(
                                config_instance::hash_get_retry_ms));
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
