pkglib_LTLIBRARIES = libpaul.la
noinst_PROGRAMS=test_sysinfo # test_r
noinst_LTLIBRARIES = libsysinfo.la
if WIN32
UTIL_SRC=
else
UTIL_SRC=fsusage.c fsusage.h
endif

libsysinfo_la_SOURCES = sysinfo.h sysinfo.cpp $(UTIL_SRC)
libsysinfo_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libsysinfo_la_LIBADD   = $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la 

libpaul_la_SOURCES  = paul.cpp paul.h job.cpp job.h job_status.h job_status.cpp job_request.h job_request.cpp job_sched_meta.cpp job_sched_meta.h job_queue.h job_queue.cpp stage.cpp run.cpp 
libpaul_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libpaul_la_LIBADD   = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la libsysinfo.la
libpaul_la_LDFLAGS=-no-undefined -avoid-version -module

#test_r_SOURCES  = test_r.cpp
#test_r_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#test_r_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
#test_r_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(SOCKET_LIB) $(EXTRA_LIBS)
#
test_sysinfo_SOURCES  = test_sysinfo.cpp 
test_sysinfo_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_sysinfo_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_sysinfo_LDADD	  = libsysinfo.la
