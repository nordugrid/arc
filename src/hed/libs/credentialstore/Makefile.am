lib_LTLIBRARIES = libarccredentialstore.la
#noinst_PROGRAMS = test

libarccredentialstore_ladir = $(pkgincludedir)/myproxy
libarccredentialstore_la_HEADERS = CredentialStore.h
libarccredentialstore_la_SOURCES = CredentialStore.cpp
libarccredentialstore_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
libarccredentialstore_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)
libarccredentialstore_la_LDFLAGS  = -version-info 1:0:0

#test_SOURCES = test.cpp
#test_CXXFLAGS = -I$(top_srcdir)/include \
#	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
#test_LDADD = \
#	libarccredentialstore.la \
#	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
#	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
