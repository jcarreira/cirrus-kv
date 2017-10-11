extern "C" {

#include "Redis.h"

redisContext* redis_connect(const char* hostname, int port) {
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    
    redisContext *c;
    c = redisConnectWithTimeout(hostname, port, timeout);

    return c;
}

void redis_put_binary(redisContext* c, const char* id,
                             const char* s, size_t size) {
    redisReply* reply = (redisReply*)
                   redisCommand(c,"SET %b %b", id, strlen(id), s, size);
    freeReplyObject(reply);
}

void redis_put_binary_numid(redisContext* c, uint64_t id,
                             const char* s, size_t size) {
    char* id_str;
    asprintf(&id_str , "%lu", id);
    redis_put_binary(c, id_str, s, size);
    free(id_str);
}

char* redis_get(redisContext* c, const char* id, int* len) {
    redisReply* reply = (redisReply*)redisCommand(c,"GET %s", id);

    char* ret = strdup(reply->str);

    if (len != nullptr) {
       *len = reply->len;
    }

    freeReplyObject(reply);
    return ret;
}

char* redis_get_numid(redisContext* c, uint64_t id, int* len) {
    char* id_str;
    asprintf(&id_str , "%lu", id);

    char*ret = redis_get(c, id_str, len);
   
    free(id_str);
    return ret;
}

void redis_ping(redisContext* c) {
    redisReply* reply = (redisReply*)redisCommand(c,"PING");
    freeReplyObject(reply);
}

}

