#include "Redis.h"
#include <iostream>

int main() {

   const char* id = "0";

   std::cout << "Connecting to redis" << std::endl;
   auto r = redis_connect("testing-redis-interf.lpfp73.0001.usw2.cache.amazonaws.com", 6379);

   if (r == NULL || r->err) {
     std::cout << "Connection error" << std::endl;
     return -1;
   }
   
   std::cout << "Putting object" << std::endl;
   redis_put_binary(r, id, "teste", strlen("teste"));
   redis_put_binary_numid(r, 1234, "1234", strlen("1234"));
   
   std::cout << "Getting object" << std::endl;
   int len1, len2;
   char* s1 = redis_get(r, id, &len1);
   char* s2 = redis_get_numid(r, 1234, &len2);

   std::cout << "1. Received: "
             << s1
             << " size: " << len1
             << std::endl;
   std::cout << "2. Received: "
             << s2
             << " size: " << len2
             << std::endl;
   free(s1);
   free(s2);

   return 0;
}
