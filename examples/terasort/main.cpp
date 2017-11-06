#include "config.hpp"
#include "record.hpp"
#include "hash_input.hpp"
#include "hash_output.hpp"
#include "hash_lambda.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <thread>

#define HASH_BYTES 1
#define HASH_NODES 16
#define INPUT_CHUNKS 10
#define SORT_NODES 4

int main() {
	cirrus_terasort::config conf(HASH_NODES, HASH_BYTES,
		cirrus_terasort::config::hash_mode::HASH_STREAMING,
		INPUT_CHUNKS, "hash_outputs", SORT_NODES, "sort_outputs");
	std::shared_ptr<cirrus_terasort::config> u_conf = std::make_shared<cirrus_terasort::config>(conf);
	std::shared_ptr<cirrus_terasort::hash_input> input = std::make_shared<cirrus_terasort::hash_input>(u_conf, "terasort_data.txt");
	std::shared_ptr<cirrus_terasort::hash_output> output = std::make_shared<cirrus_terasort::hash_output>(u_conf);
	std::thread all[u_conf->hash_nodes()];
	for(uint32_t i = 0; i < u_conf->hash_nodes(); i++)
		all[i] = std::thread(cirrus_terasort::hash_lambda, input, output);
	for(uint32_t i = 0; i < u_conf->hash_nodes(); i++)
		all[i].join();
}
