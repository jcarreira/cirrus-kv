#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include "record.hpp"
#include "hash_input.hpp"
#include "config.hpp"

#define HASH_BYTES 1
#define HASH_MODULUS 5
#define HASH_NODES 16
#define INPUT_CHUNKS 10
#define SORT_NODES 16

int main(int argc, char* argv[]) {
	auto start_time = std::chrono::high_resolution_clock::now();
	std::cout << "Configuring..." << std::endl;
	cirrus_terasort::config conf(HASH_NODES, HASH_BYTES, HASH_MODULUS,
		cirrus_terasort::config::hash_mode::HASH_STREAMING,
		INPUT_CHUNKS, "hash_outputs", SORT_NODES, "sort_outputs");
	std::shared_ptr<cirrus_terasort::config> u_conf = std::make_shared<cirrus_terasort::config>(conf);
	std::shared_ptr<cirrus_terasort::hash_input> input = std::make_shared<cirrus_terasort::hash_input>(u_conf, "terasort_data.txt");
	std::cout << "Starting Read..." << std::endl;
	auto read_start = std::chrono::high_resolution_clock::now();
	std::vector<std::shared_ptr<cirrus_terasort::record>> recs, temp;
	while((temp = input->read_chunk()).size()) recs.insert(recs.end(), temp.begin(), temp.end());
	auto read_end = std::chrono::high_resolution_clock::now();
	std::cout << "Finishing Read..." << std::endl;
	std::cout << "Read time: " << std::chrono::duration_cast<std::chrono::seconds>(read_end - read_start).count() << " seconds" << std::endl;
	std::cout << "Starting Sort..." << std::endl;
	auto sort_start = std::chrono::high_resolution_clock::now();
	std::sort(recs.begin(), recs.end(), [](const std::shared_ptr<cirrus_terasort::record>& lhs, const std::shared_ptr<cirrus_terasort::record>& rhs) {
			return lhs->raw_data() < rhs->raw_data();
		});
	auto sort_end = std::chrono::high_resolution_clock::now();
	std::cout << "Sort time: " << std::chrono::duration_cast<std::chrono::seconds>(sort_end - sort_start).count() << " seconds" << std::endl;
	std::cout << "Ending Sort..." << std::endl;
	auto end_time = std::chrono::high_resolution_clock::now();
	std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << " seconds" << std::endl;
}
