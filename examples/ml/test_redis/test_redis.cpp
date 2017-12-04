#include "Redis.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <chrono>


void test_put(redisContext* r, const char* id, const char*value) {
   redis_put_binary(r, id, value, strlen(value));
#ifdef DEBUG
   std::cout << "Wrote: "
             << value
             << " size: " << strlen(value)
             << std::endl;
#endif
}

void test_get(redisContext* r, const char* id) {
   int len;
   char* s = redis_get(r, id, &len);

   if (s) {
#ifdef DEBUG
	   std::cout << "2. Received: "
		   << s
		   << " size: " << len
		   << std::endl;
#endif
           free(s);
   } else {
      std::cout << "Object does not exist" << std::endl;
   }
}

int main() {
   std::cout << "Connecting to redis" << std::endl;
   auto r = redis_connect("testing-redis-interf.lpfp73.0001.usw2.cache.amazonaws.com", 6379);

   if (r == NULL || r->err) {
     std::cout << "Connection error" << std::endl;
     return -1;
   }

   std::string s;
   std::string id = "0";
   unsigned int data_size = 100000;
   for (int i = 0; i < data_size; ++i) {
       s+= "a";
   }
   test_put(r, id.c_str(), s.c_str());

   unsigned long long int cum_us = 0;
   for (int i = 0; i < 100; ++i) {
       auto start = std::chrono::high_resolution_clock::now();
       test_get(r, id.c_str());
       auto elapsed = std::chrono::high_resolution_clock::now() - start;
       long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
       cum_us += microseconds;
   }

   double avg_elapsed = cum_us / 100.0;
   double bw_MBps = data_size / avg_elapsed / 1024 / 1024 * 1000 * 1000;
   std::cout << "Average get bandwidth (MB/s): " << bw_MBps << std::endl;

   return 0;
}
