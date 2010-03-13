lib_LTLIBRARIES = libarccredentailstore.la
#noinst_PROGRAMS = test

libarccredentailstore_ladir = $(pkgincludedir)/myproxy
libarccredentailstore_la_HEADERS  = CredentialStore.h
libarccredentailstore_la_SOURCES  = CredentialStore.cpp
libarccredentailstore_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarccredentailstore_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                                    $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                                    $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                                    $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                                    $(GLIBMM_LIBS)

#test_SOURCES  = test.cpp
#test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
#test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
#test_LDADD    = ./libarccredentailstore.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
