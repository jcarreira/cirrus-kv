#ifndef CIRRUS_TERASORT_CONFIG_INSTANCE_HPP
#define CIRRUS_TERASORT_CONFIG_INSTANCE_HPP

#include <cstdint>

namespace cirrus_terasort {

	using INT_TYPE = uint64_t;

	namespace config_instance {

		static constexpr INT_TYPE total_processes = 9; // must match Makefile
		static constexpr INT_TYPE hash_nodes = 4;
		static constexpr INT_TYPE hash_bytes = 2;
		static constexpr INT_TYPE hash_modulus = 15;
		static constexpr INT_TYPE hash_threads = 16;
		static constexpr INT_TYPE hash_nodes_per_reader = 4;
		static constexpr char hash_output_dir[] = "/nscratch/nathreya/hash_output_dir";

		static constexpr char cirrus_port[] = "12345";
		static constexpr char cirrus_ip[] = "10.10.49.83";

		static constexpr INT_TYPE record_size = 100;
		static constexpr INT_TYPE num_records = 200000000;
	
		static constexpr const char* input_files[] = { "/nscratch/nathreya/terasort_data.txt" };
		static constexpr INT_TYPE num_input_files = 1;
		static constexpr INT_TYPE read_chunk_size = 1000;
		static constexpr INT_TYPE read_threads = 16;

		static constexpr INT_TYPE sort_nodes = 4;
		static constexpr INT_TYPE sort_threads = 16;
		static constexpr char sort_output_dir[] = "/nscratch/nathreya/sort_output_dir";
		
		static_assert((hash_nodes * hash_threads * hash_modulus) % sort_nodes == 0, "config error");
		static_assert(num_records % read_chunk_size == 0, "config error");
		static_assert((num_records / read_chunk_size) % hash_nodes == 0, "config error");
		static_assert(total_processes == hash_nodes + sort_nodes + num_input_files, "config error");
	 }
}

#endif
