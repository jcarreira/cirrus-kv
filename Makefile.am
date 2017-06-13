AUTOMAKE_OPTIONS = foreign serial-tests
SUBDIRS = src tests benchmarks
TESTS = ./tests/basic_test.py
source_dir = src/
DEFAULT_INCLUDES = -I$(source_dir)

check_style:
	find . -path ./third_party -prune -o  -name "*.cpp" | xargs python scripts/cpplint.py

benchmark:
	./benchmarks/run_benchmarks.py

docs:
	doxygen ./docs/Doxyfile
