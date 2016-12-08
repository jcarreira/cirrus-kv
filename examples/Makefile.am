AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = wordcount wordcountpar

wordcount_SOURCES  = wordcount.cpp
wordcount_LDFLAGS  = -pthread
wordcount_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/third_party/sparsehash/src/ -O3
wordcount_LDADD    = -libverbs -lrdmacm  \
                     -L$(top_srcdir)/src/utils/ -L$(top_srcdir)/src/client/ \
                     -L$(top_srcdir)/src/authentication \
                     -L$(top_srcdir)/src/common \
                     -lclient -lutils -lauthentication -lcommon

wordcountpar_SOURCES  = wordcountpar.cpp
wordcountpar_LDFLAGS  = -pthread -fopenmp
wordcountpar_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/third_party/sparsehash/src/ -O3 -fopenmp
wordcountpar_LDADD    = -libverbs -lrdmacm  \
                     -L$(top_srcdir)/src/utils/ -L$(top_srcdir)/src/client/ \
                     -L$(top_srcdir)/src/authentication \
                     -L$(top_srcdir)/src/common \
                     -lclient -lutils -lauthentication -lcommon
