noinst_LTLIBRARIES = libjsdl.la

libjsdl_la_SOURCES=jsdl_job.cpp jsdl_job.h
libjsdl_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
libjsdl_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la 
#../../files/libfiles.la ../../misc/libmisc.la ../../url/liburl.la
libjsdl_la_LDFLAGS=

EXTRA_DIST = *.xsd 
