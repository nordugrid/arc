noinst_PROGRAMS=test
SOURCES = MCCTCP.cpp ../MCC.cpp ../../PayloadStream.cpp ../../PayloadTCPSocket.cpp ../../PayloadRaw.cpp

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la
