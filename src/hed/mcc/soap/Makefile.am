lib_LTLIBRARIES= libmccsoap.la
noinst_PROGRAMS=test
SOURCES = MCCSOAP.cpp ../../SOAPMessage.cpp ../../PayloadRaw.cpp ../../PayloadSOAP.cpp ../MCC.cpp

libmccsoap_ladir=$(includedir)
libmccsoap_la_HEADERS=MCCSOAP.h
libmccsoap_la_SOURCES=MCCSOAP.cpp
libmccsoap_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmccsoap_la_LIBADD=$(top_srcdir)/src/hed/libpayload.la $(top_srcdir)/src/hed/mcc/libmcc.la
libmccsoap_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la
