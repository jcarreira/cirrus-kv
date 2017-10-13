#ifndef _CIRRUS_REDIS_H_
#define _CIRRUS_REDIS_H_

extern "C" {

#include <hiredis.h>
#include <string.h>
#include <stdlib.h>

redisContext* redis_connect(const char* hostname, int port);
void redis_put_binary(redisContext* c, const char* id,
                             const char* s, size_t size);
void redis_put_binary_numid(redisContext* c, uint64_t id,
                             const char* s, size_t size);
char* redis_get(redisContext* c, const char* id, int* len = nullptr);
char* redis_get_numid(redisContext* c, uint64_t id, int* len = nullptr);
void redis_ping(redisContext* c);
void redis_delete(redisContext* c, const char* id);

}

#endif // _CIRRUS_REDIS_H_
