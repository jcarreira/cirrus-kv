#include "Redis.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>


void test_put(redisContext* r, const char* id, const char*value) {
   redis_put_binary(r, id, value, strlen(value));
   std::cout << "Wrote: "
             << value
             << " size: " << strlen(value)
             << std::endl;
}

void test_get(redisContext* r, const char* id) {
   int len;
   char* s = redis_get(r, id, &len);

   if (s) {
	   std::cout << "2. Received: "
		   << s
		   << " size: " << len
		   << std::endl;
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
   for (int i = 0; i < 10000; ++i) {
       s+= "a";
       test_put(r, id.c_str(), s.c_str());
       test_get(r, id.c_str());
   }
}
