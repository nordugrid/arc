lib_LTLIBRARIES= libmcctcp.la
noinst_PROGRAMS=test
SOURCES = MCCTCP.cpp ../MCC.cpp ../../PayloadStream.cpp PayloadTCPSocket.cpp ../../PayloadRaw.cpp

libmcctcp_ladir=$(includedir)
libmcctcp_la_HEADERS=MCCTCP.h PayloadTCPSocket.h
libmcctcp_la_SOURCES=MCCTCP.cpp PayloadTCPSocket.cpp
libmcctcp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmcctcp_la_LIBADD=$(top_srcdir)/src/hed/libpayload.la $(top_srcdir)/src/hed/mcc/libmcc.la
libmcctcp_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la
