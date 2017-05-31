include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

LIBS         = -lobjstore
LINCLUDES    = -L$(top_srcdir)/src/object_store

noinst_LIBRARIES = iterator.a
iterator_a_SOURCES = iterator.cpp
iterator_a_CPPFLAGS = -ggdb -I$(top_srcdir)
