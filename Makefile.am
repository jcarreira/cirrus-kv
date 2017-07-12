AUTOMAKE_OPTIONS = foreign serial-tests
SUBDIRS = src tests benchmarks
TESTS = ./tests/basic_test.py ./tests/test_mem_exhaustion.py ./tests/test_store_simple.py \
	./tests/test_cache_manager.py ./tests/test_iterator.py ./tests/test_store.py \
	./tests/test_mt.py

source_dir = src/
DEFAULT_INCLUDES = -I$(source_dir)

.PHONY: docs

check_style:
	find . -path ./third_party -prune -o -path ./src/common/schemas -prune -o -name "*.h" -o -name "*.cpp"  | xargs cpplint

benchmark: all
	./benchmarks/run_benchmarks.py

docs:
	doxygen ./docs/Doxyfile
