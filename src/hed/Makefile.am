SUBDIRS=libs wsrf ws-addressing

noinst_PROGRAMS=test client http_test
SOURCES = XMLNode.cpp \
		  SOAPMessage.cpp \
		  PayloadRaw.cpp \
		  PayloadSOAP.cpp \
          PayloadStream.cpp \
		  PayloadTCPSocket.cpp \
		  PayloadHTTP.cpp

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(LIBXML2_CFLAGS)
test_LDFLAGS=$(LIBXML2_LIBS)
client_SOURCES=$(SOURCES) client.cpp
client_CPPFLAGS=$(LIBXML2_CFLAGS)
client_LDFLAGS=$(LIBXML2_LIBS)
http_test_SOURCES=$(SOURCES) http_test.cpp
http_test_CPPFLAGS=$(LIBXML2_CFLAGS)
http_test_LDFLAGS=$(LIBXML2_LIBS)
