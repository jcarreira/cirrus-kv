include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign
SUBDIRS = schemas
noinst_LIBRARIES = libcommon.a
libcommon_a_SOURCES = AllocatorMessageGenerator.cpp BladeMessageGenerator.cpp \
                      Synchronization.cpp \
                      BladeObjectStoreMessageGenerator.cpp
libcommon_a_CPPFLAGS = -ggdb -I$(top_srcdir)
