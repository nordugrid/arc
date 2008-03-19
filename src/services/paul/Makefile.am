pkglib_LTLIBRARIES = libpaul.la

libpaul_la_SOURCES  = paul.cpp paul.h job.h job_status.h job_status.cpp job_request.h job_request.cpp sysinfo.h sysinfo.cpp
libpaul_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libpaul_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libpaul_la_LDFLAGS=-no-undefined -avoid-version -module
