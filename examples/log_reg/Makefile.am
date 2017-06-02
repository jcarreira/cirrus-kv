AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = log_reg

LIBS         = -lobjstore -l:libcityhash.a -lclient -libverbs -lrdmacm\
               -lauthentication -lutils -lcommon

log_reg_SOURCES  = log_reg.cpp
log_reg_LDFLAGS  = -pthread
log_reg_CPPFLAGS = -ggdb -I$(top_srcdir) -I$(top_srcdir)/third_party/sparsehash/src/ -O3 \
                   -I$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/
log_reg_LDADD    = $(LIBS)  \
                   -L$(top_srcdir)/src/utils/ \
                   -L$(top_srcdir)/src/client/ \
                   -L$(top_srcdir)/src/authentication \
                   -L$(top_srcdir)/src/common \
                   -L$(top_srcdir)/src/object_store \
                   -L$(top_srcdir)/third_party/libcuckoo/cityhash-1.1.1/src/.libs/ \
                   -lclient -lutils -lauthentication -lcommon

