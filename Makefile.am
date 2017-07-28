AUTOMAKE_OPTIONS = foreign serial-tests
SUBDIRS = src tests benchmarks

TESTS = ./tests/test_client_TCP.py ./tests/test_mem_exhaustion_TCP.py \
		./tests/test_store_simple_TCP.py ./tests/test_cache_manager_TCP.py \
		./tests/test_iterator_TCP.py ./tests/test_store_TCP.py \
    ./tests/test_mt_TCP.py ./tests/test_mult_clients_TCP.py

if USE_RDMA
TESTS += ./tests/test_client_RDMA.py ./tests/test_mem_exhaustion_RDMA.py  \
		 ./tests/test_store_simple_RDMA.py ./tests/test_cache_manager_RDMA.py \
		 ./tests/test_iterator_RDMA.py ./tests/test_store_RDMA.py \
     ./tests/test_mt_RDMA.py ./tests/test_mult_clients_RDMA.py
endif


source_dir = src/
DEFAULT_INCLUDES = -I$(source_dir)

.PHONY: docs

check_style:
	find . -path ./third_party -prune -o -path ./src/common/schemas -prune -o -name "*.h" -o -name "*.cpp"  | xargs cpplint

benchmark: all
	./benchmarks/run_benchmarks.py

docs:
	doxygen ./docs/Doxyfile
