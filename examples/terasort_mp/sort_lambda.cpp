#include "sort_lambda.hpp"

#include <cmath>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace cirrus_terasort {

sort_lambda::sort_lambda(
        std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
                s, INT_TYPE p) : _process_index(p), _store(s) {
        for (INT_TYPE i = 0; i < config_instance::hash_nodes; i++)
                _index_list.push_back((i + 1) * config_instance::total_read_keys
                        + _process_index);

        _next_index = 0;
        _finished = false;
}

std::vector<std::shared_ptr<std::string>> sort_lambda::get_records() {
        std::vector<std::shared_ptr<std::string>> ret;

        _read_mutex.lock();

        auto get_start = std::chrono::high_resolution_clock::now();
        for (INT_TYPE n = _next_index; n < config_instance::hash_nodes &&
                !_finished; n++) {
                std::string chunk = _store->get(_index_list[n]);
                if (chunk == config_instance::sentinel) {
                        _finished = ++_next_index >=
                                config_instance::hash_nodes;
                } else {
                        _index_list[n] += config_instance::sort_nodes;
                        for (INT_TYPE i = 0; i <
                                config_instance::read_chunk_size && (i + 1) *
                                (config_instance::record_size + 1) <=
                                        chunk.size(); i++) {
                                std::string str = chunk.substr(i *
                                        (config_instance::record_size + 1),
                                        config_instance::record_size);
                                ret.push_back(
                                        std::make_shared<std::string>(str));
                        }
                        break;
                }
        }
        auto get_end = std::chrono::high_resolution_clock::now();

        long double bytes = ret.size() * (config_instance::record_size + 1),
                time = std::chrono::duration_cast<std::chrono::microseconds>
                        (get_end - get_start).count();
        _stats_lock.lock();
        _get_thread_bandwidths.push_back(std::make_pair(bytes, time));
        _stats_lock.unlock();

        _read_mutex.unlock();
        return ret;
}

void sort_lambda::add_sort_data(long double b, long double t) {
        _stats_lock.lock();

        _sort_thread_bandwidths.push_back(std::make_pair(b, t));

        _stats_lock.unlock();
}

void sort_lambda::add_merge_data(long double b, long double t) {
        _stats_lock.lock();

        _merge_thread_bandwidths.push_back(std::make_pair(b, t));

        _stats_lock.unlock();
}

void sort_lambda::add_sort_get_data(long double b, long double t) {
        _stats_lock.lock();

        _sort_get_thread_bandwidths.push_back(std::make_pair(b, t));

        _stats_lock.unlock();
}

void sort_lambda::print_avg_stats() {
        long double  sort_avg = 0, merge_avg = 0, sort_get_avg = 0;
        for (std::pair<long double, long double> p : _sort_thread_bandwidths)
                sort_avg += (p.first / 1e6) / (p.second / 1e3);

        for (std::pair<long double, long double> p : _merge_thread_bandwidths)
                merge_avg += (p.first / 1e6) / (p.second / 1e3);

        for (std::pair<long double, long double> p :
                _sort_get_thread_bandwidths)
                sort_get_avg += (p.first / 1e6) / (p.second / 1e3);

        std::string to_print = "Sorting/merging process " +
                std::to_string(_process_index) + " total get rate: " +
                std::to_string(sort_get_avg) + " MB/s\n";
        to_print += "Sorting/merging process " + std::to_string(_process_index)
                + " average get rate: " +
                std::to_string(sort_get_avg /
                        _sort_get_thread_bandwidths.size())
                + " MB/s\n";
        to_print += "Sorting/merging process " + std::to_string(_process_index)
                + " total sort rate: " + std::to_string(sort_avg) + " MB/s\n";
        to_print += "Sorting/merging process " + std::to_string(_process_index)
                + " average sort rate: " +
                std::to_string(sort_avg / _sort_thread_bandwidths.size())
                + " MB/s\n";
        to_print += "Sorting/merging process " + std::to_string(_process_index)
                + " total merge rate: " + std::to_string(merge_avg) + " MB/s\n";
        to_print += "Sorting/merging process " + std::to_string(_process_index)
                + " average merge rate: " + std::to_string(sort_avg /
                        _merge_thread_bandwidths.size()) + " MB/s\n";

        std::cout << to_print;

        std::ofstream of(std::string(config_instance::stats) + "/sort_rates_" +
                std::to_string(_process_index) + ".txt");
        for (std::pair<long double, long double> p : _get_thread_bandwidths)
                of << (p.first / 1e6) / (p.second / 1e6) << "\n";
        of.close();
}

void sorter(std::shared_ptr<sort_lambda> sl,
        std::promise<std::vector<std::shared_ptr<std::string>>> p) {
        std::vector<std::shared_ptr<std::string>> curr, recs;
        auto sort_get_start = std::chrono::high_resolution_clock::now();
        INT_TYPE target_size = std::ceil(
                (long double) config_instance::num_records /
                        (long double) config_instance::sort_nodes /
                                (long double) config_instance::sort_threads);
        while (recs.size() < 2 * target_size &&
                (curr = sl->get_records()).size() != 0)
                recs.insert(recs.end(), curr.begin(), curr.end());
        auto sort_get_end = std::chrono::high_resolution_clock::now();
        long double get_time =
                std::chrono::duration_cast<std::chrono::milliseconds>
                        (sort_get_end - sort_get_start).count(),
                get_bytes = recs.size() * config_instance::record_size;
        sl->add_sort_get_data(get_bytes, get_time);

        auto sort_start = std::chrono::high_resolution_clock::now();
        std::sort(recs.begin(), recs.end(), []
                (const std::shared_ptr<std::string>& lhs,
                const std::shared_ptr<std::string>& rhs) {
                return *lhs < *rhs;
        });
        auto sort_end = std::chrono::high_resolution_clock::now();

        long double time =
                std::chrono::duration_cast<std::chrono::milliseconds>
                        (sort_end - sort_start).count(),
                bytes = recs.size() * config_instance::record_size;
        sl->add_sort_data(bytes, time);
        p.set_value(std::move(recs));
}

void merger(std::shared_ptr<sort_lambda> sl,
        const std::vector<std::shared_ptr<std::string>>& vec1,
        const std::vector<std::shared_ptr<std::string>>& vec2,
        std::promise<std::vector<std::shared_ptr<std::string>>> p) {
        auto merge_start = std::chrono::high_resolution_clock::now();

        std::vector<std::shared_ptr<std::string>> ret;
        std::merge(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(),
                std::back_inserter(ret), []
                        (const std::shared_ptr<std::string>& lhs,
                        const std::shared_ptr<std::string>& rhs) {
                return *lhs < *rhs;
        });

        auto merge_end = std::chrono::high_resolution_clock::now();
        long double time =
                std::chrono::duration_cast<std::chrono::milliseconds>
                        (merge_end - merge_start).count(),
                bytes = ret.size() * config_instance::record_size;
        sl->add_merge_data(bytes, time);

        p.set_value(std::move(ret));
}
}  // namespace cirrus_terasort
