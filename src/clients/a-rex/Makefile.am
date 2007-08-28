noinst_PROGRAMS=arex_test
SOURCES = arex_client.cpp

arex_test_SOURCES=$(SOURCES) arex_test.cpp
arex_test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
arex_test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
arex_test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la
