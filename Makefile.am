AUTOMAKE_OPTIONS = foreign
SUBDIRS = src

source_dir = src/
DEFAULT_INCLUDES = -I$(source_dir)

check_style:
	find . -name "*.cpp" | xargs python scripts/cpplint.py
