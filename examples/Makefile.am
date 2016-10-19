AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = wordcount

wordcount_SOURCES  = wordcount.cpp
wordcount_LDFLAGS  = -pthread
wordcount_CPPFLAGS = -ggdb -I$(top_srcdir)
wordcount_LDADD    = -libverbs -lrdmacm  \
                     -L$(top_srcdir)/src/utils/ -L$(top_srcdir)/src/client/ \
                     -L$(top_srcdir)/src/authentication \
                     -lclient -lutils -lauthentication
