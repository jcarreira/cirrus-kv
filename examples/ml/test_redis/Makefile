all:  
	g++ test_redis.cpp Redis.cpp -I../../../third_party/hiredis/ -L../../../third_party/hiredis/ -lhiredis -static -o test_redis

clean:
	rm a.out test_redis
