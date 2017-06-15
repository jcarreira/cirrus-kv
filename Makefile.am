AUTOMAKE_OPTIONS = foreign serial-tests
SUBDIRS = src tests benchmarks
TESTS = ./tests/basic_test.py ./tests/test_mem_exhaustion.py \
        ./tests/test_store_simple.py
source_dir = src/
DEFAULT_INCLUDES = -I$(source_dir)

.PHONY: docs

check_style:
	find . -path ./third_party -prune -o  -name "*.cpp" | xargs cpplint

benchmark:
	./benchmarks/run_benchmarks.py

docs:
	doxygen ./docs/Doxyfile
