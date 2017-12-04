#include <stdio.h>
#include <iostream>

extern "C" {

#include <string.h>
#include "Redis.h"

//#define REDIS_DEBUG

    redisContext* redis_connect(const char* hostname, int port) {
        struct timeval timeout = { 1, 500000 }; // 1.5 seconds

#ifdef REDIS_DEBUG
        std::cout << "Connecting to hostname: " << hostname << std::endl;
#endif
        redisContext *c;
        c = redisConnectWithTimeout(hostname, port, timeout);

        return c;
    }

    void redis_put(redisContext* c, const char* id,
            const char* s) {
#ifdef REDIS_DEBUG
        std::cout << "Redis put"
            << " id: " << id
            << " id size: " << strlen(id)
            << std::endl;    
#endif
        redisReply* reply = (redisReply*) redisCommand(c,"SET key:%s %s", id, s);

#ifdef REDIS_DEBUG
        std::cout << "Redis put"
            << " reply type: " << reply->type
            << " status: " << reply->str
            << std::endl;    
#endif

        freeReplyObject(reply);
    }

    void redis_put_binary(redisContext* c, const char* id,
            const char* s, size_t size) {
#ifdef REDIS_DEBUG
        std::cout << "Redis put binary"
            << " id: " << id
            << " id size: " << strlen(id)
            << " data size: " << size
            << std::endl;    
#endif
        redisReply* reply = (redisReply*)
            redisCommand(c,"SET key:%s %b", id, s, size);

#ifdef REDIS_DEBUG
        std::cout << "Redis put binary"
            << " reply type: " << reply->type
            << " status: " << reply->str
            << " size: " << reply->len
            << std::endl;    
#endif

        freeReplyObject(reply);
    }

    void redis_put_binary_numid(redisContext* c, uint64_t id,
            const char* s, size_t size) {
#ifdef REDIS_DEBUG
        std::cout << "[REDIS] "
            << "redis put binary numid"
            << "id : " << id
            << " size: " << size << std::endl;
#endif

        char id_str[100];
        int ret = snprintf(id_str, 100, "%lu", id);
        if (ret < 0) {
            std::cout << "ERROR in sprintf" << std::endl;
        }
#ifdef REDIS_DEBUG
        std::cout << "[REDIS] "
            << "redis put binary2 with sprintf" 
            << "id_str:-" << id_str
            << "-" << std::endl;
#endif
        redis_put_binary(c, id_str, s, size);
#ifdef REDIS_DEBUG
        std::cout << "[REDIS] "
            << "redis put binary3 " << std::endl;
#endif
    }

    char* redis_get(redisContext* c, const char* id, int* len) {
#ifdef REDIS_DEBUG
        std::cout << "[REDIS] "
            << "redis get id: " << id << std::endl;
#endif
        redisReply* reply = (redisReply*)redisCommand(c,"GET key:%s", id);

        if (reply->type == REDIS_REPLY_NIL) {
            std::cout << "[REDIS] "
                << "redis returned nil"
                << " with id: " << id
                << std::endl;
            freeReplyObject(reply);
            return NULL;
        }

#ifdef REDIS_DEBUG
        std::cout << "[REDIS] "
            << "redis returned success len: " << reply->len << std::endl;
#endif

        char* ret = (char*)malloc(reply->len);
        memcpy(ret, reply->str, reply->len);

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
        if (ret) {
#ifdef REDIS_DEBUG
            std::cout << "[REDIS] "
                << "redis got"
                << "id : " << id_str
                << " len: " << *len
                << std::endl;
#endif
        } else {
#ifdef REDIS_DEBUG
            std::cout << "[REDIS] "
                << "redis get failed"
                << std::endl;
#endif
	    free(id_str);
#ifdef REDIS_DEBUG
            std::cout << "[REDIS] "
                << "redis return nullptr"
                << std::endl;
#endif
	    return nullptr;
        }

#ifdef REDIS_DEBUG
	std::cout << "[REDIS] "
		<< "freeing id_str"
		<< std::endl;
#endif
        free(id_str);
#ifdef REDIS_DEBUG
	std::cout << "[REDIS] "
		<< "return ret"
		<< std::endl;
#endif
        return ret;
    }

    void redis_ping(redisContext* c) {
        redisReply* reply = (redisReply*)redisCommand(c,"PING");
        freeReplyObject(reply);
    }

    void redis_delete(redisContext* c, const char* id) {
        redisReply* reply = (redisReply*)
            redisCommand(c,"DEL %s", id );
        freeReplyObject(reply);
    }

}

