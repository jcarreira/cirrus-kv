include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign
SUBDIRS = schemas
noinst_LIBRARIES = libcommon.a
libcommon_a_SOURCES = AllocatorMessageGenerator.cpp Synchronization.cpp Future.cpp

libcommon_a_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/src
