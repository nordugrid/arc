pkglib_LTLIBRARIES = libpaul.la
# noinst_PROGRAMS=test_r
if WIN32
UTIL_SRC=
else
UTIL_SRC=fsusage.c fsusage.h
endif
  
libpaul_la_SOURCES  = paul.cpp paul.h job.cpp job.h job_status.h job_status.cpp job_request.h job_request.cpp job_sched_meta.cpp job_sched_meta.h sysinfo.h sysinfo.cpp job_queue.h job_queue.cpp stage.cpp run.cpp $(UTIL_SRC)
libpaul_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libpaul_la_LIBADD   = $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la
libpaul_la_LDFLAGS=-no-undefined -avoid-version -module

#test_r_SOURCES  = test_r.cpp
#test_r_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#test_r_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
#test_r_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
