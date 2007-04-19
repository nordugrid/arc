lib_LTLIBRARIES= libmcchttp.la
noinst_PROGRAMS=test
SOURCES = MCCHTTP.cpp ../../PayloadHTTP.cpp ../../PayloadRaw.cpp ../../PayloadStream.cpp ../MCC.cpp

libmcchttp_ladir=$(includedir)
libmcchttp_la_HEADERS=MCCHTTP.h
libmcchttp_la_SOURCES=MCCHTTP.cpp ../../PayloadHTTP.cpp ../../PayloadRaw.cpp ../MCC.cpp
libmcchttp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmcchttp_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la 
#$(top_srcdir)/src/hed/mcc/libmcc.la
libmcchttp_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la
