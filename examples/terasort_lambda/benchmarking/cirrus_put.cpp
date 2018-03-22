#include <aws/core/Aws.h>
#include <memory>
#include <cstdlib>
#include <vector>

#include "../S3.hpp"
#include "../config.hpp"
#include "serialization.hpp"

#include "object_store/FullBladeObjectStore.h"
#include "tests/object_store/object_store_internal.h"

static constexpr INT_TYPE num_loops = 50;

int main(int argc, char* argv[]) {
  
  cirrus::TCPClient tcp_client;
  string_serializer str_ser;
  std::shared_ptr<cirrus::ostore::FullBladeObjectStoreTempl<std::string>>
    store =
    std::make_shared<
    cirrus::ostore::FullBladeObjectStoreTempl<std::string>>(
        cirrus_ip, cirrus_port, &tcp_client, str_ser,
        string_deserializer);

  std::vector<INT_TYPE> obj_sizes { /*1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70, 80, 90,*/ 100 };
  for(INT_TYPE obj_size : obj_sizes) {
    std::string obj(obj_size * 100 * 1'000, 'a');
    long double total_time = 0, bytes = 0;
    for(INT_TYPE i = 0; i < num_loops; i++) {
      bytes += obj_size * 100 * 1'000;
      auto start = std::chrono::high_resolution_clock::now();
      store->put(obj_size, obj);
      auto end = std::chrono::high_resolution_clock::now();
      long double time = std::chrono::duration_cast
        <std::chrono::microseconds>(end - start).count();
      total_time += time;
    }
    std::cout << "Put rate for " << obj_size * 100 << " KB objects: " <<
      (bytes / 1e6) / (total_time / 1e6) << " MB/s" << '\n';
  }
}
