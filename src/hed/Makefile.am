SUBDIRS=libs wsrf ws-addressing

noinst_PROGRAMS=test client http_test
SOURCES = SOAPMessage.cpp PayloadRaw.cpp PayloadSOAP.cpp PayloadStream.cpp PayloadTCPSocket.cpp PayloadHTTP.cpp

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la

client_SOURCES=$(SOURCES) client.cpp
client_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
client_LDFLAGS=$(LIBXML2_LIBS)
client_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la

http_test_SOURCES=$(SOURCES) http_test.cpp
http_test_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
http_test_LDFLAGS=$(LIBXML2_LIBS)
http_test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la
