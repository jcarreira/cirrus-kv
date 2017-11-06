all: main

CC=g++
CC_FLAGS=-std=c++17

setup:
	mkdir build outputs hash_outputs

clean:
	rm -f build/* outputs/*
	find hash_outputs/ -maxdepth 1 -name '*.txt' -delete

config: config.cpp config.hpp
	$(CC) $(CC_FLAGS) -c config.cpp -o build/config.o

record: record.cpp record.hpp
	$(CC) $(CC_FLAGS) -c record.cpp -o build/record.o

hash_input: hash_input.cpp hash_input.hpp
	$(CC) $(CC_FLAGS) -c hash_input.cpp -o build/hash_input.o

hash_output: hash_output.cpp hash_output.hpp
	$(CC) $(CC_FLAGS) -c hash_output.cpp -o build/hash_output.o

hash_lambda: hash_output.cpp hash_lambda.hpp
	$(CC) $(CC_FLAGS) -c hash_lambda.cpp -o build/hash_lambda.o

main: config record hash_input hash_output hash_lambda main.cpp
	$(CC) $(CC_FLAGS) -c main.cpp -o build/main.o
	$(CC) build/*.o -o outputs/main -lpthread
