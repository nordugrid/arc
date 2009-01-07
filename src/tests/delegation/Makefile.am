noinst_PROGRAMS=test_delegation_client test_client_with_delegation_sechandler

test_delegation_client_SOURCES =  test_delegation_client.cpp
test_delegation_client_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_delegation_client_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_delegation_client_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la

test_client_with_delegation_sechandler_SOURCES =  test_client_with_delegation_sechandler.cpp
test_client_with_delegation_sechandler_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_client_with_delegation_sechandler_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_client_with_delegation_sechandler_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la

