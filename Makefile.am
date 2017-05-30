AUTOMAKE_OPTIONS = foreign
SUBDIRS = src tests benchmarks

source_dir = src/
DEFAULT_INCLUDES = -I$(source_dir)

check_style:
	find . -path ./third_party -prune -o  -name "*.cpp" | xargs python scripts/cpplint.py
