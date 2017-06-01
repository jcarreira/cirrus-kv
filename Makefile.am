AUTOMAKE_OPTIONS = foreign serial-tests
SUBDIRS = src tests benchmarks
TESTS = ./tests/basic_test.py
source_dir = src/
DEFAULT_INCLUDES = -I$(source_dir)

check_style:
	find . -name "*.cpp" | xargs cpplint

benchmark:
	./benchmarks/run_benchmarks.py
