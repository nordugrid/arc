noinst_PROGRAMS=test
SOURCES = ../SOAPMessage.cpp WSA.cpp WSA.h
test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS) 
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la
