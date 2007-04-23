lib_LTLIBRARIES= libmcchttp.la
noinst_PROGRAMS=test http_test
SOURCES = MCCHTTP.cpp PayloadHTTP.cpp ../../PayloadRaw.cpp ../../PayloadStream.cpp ../MCC.cpp

libmcchttp_ladir=$(includedir)
libmcchttp_la_HEADERS=MCCHTTP.h PayloadHTTP.h
libmcchttp_la_SOURCES=MCCHTTP.cpp PayloadHTTP.cpp
libmcchttp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmcchttp_la_LIBADD=$(top_srcdir)/src/hed/libpayload.la $(top_srcdir)/src/hed/mcc/libmcc.la
libmcchttp_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la

http_test_SOURCES=PayloadHTTP.cpp ../../mcc/tcp/PayloadTCPSocket.cpp ../../PayloadRaw.cpp ../../PayloadStream.cpp ../../PayloadSOAP.cpp ../../SOAPMessage.cpp http_test.cpp
http_test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
http_test_LDFLAGS=$(LIBXML2_LIBS)
http_test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la

