include $(top_srcdir)/common.mk

AUTOMAKE_OPTIONS = foreign

DEFINE_LOG=-DELPP_DISABLE_INFO_LOGS

bin_PROGRAMS = clientmain scalability_main
clientmain_SOURCES = main.cpp RDMAClient.cpp \
               BladeClient.cpp
clientmain_LDADD = -libverbs -lrdmacm
clientmain_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
clientmain_LDFLAGS = -pthread


scalability_main_SOURCES = scalability_main.cpp RDMAClient.cpp \
                          BladeClient.cpp
scalability_main_LDADD = -libverbs -lrdmacm
scalability_main_CPPFLAGS = -ggdb -I$(top_srcdir) $(DEFINE_LOG)
scalability_main_LDFLAGS = -pthread
