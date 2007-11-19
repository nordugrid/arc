pkglib_LTLIBRARIES = libarex2.la

libarex2_la_SOURCES  = arex2.cpp arex2.h rte.cpp rte.h job_state.cpp job_state.h lrms.cpp lrms.h

libarex2_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
# Needs real cleaning in respect to dependencies
libarex2_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libarex2_la_LDFLAGS=-no-undefined -avoid-version -module
