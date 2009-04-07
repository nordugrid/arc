bin_PROGRAMS=perftest perftest_saml2sso

perftest_SOURCES  = perftest.cpp
perftest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

perftest_saml2sso_SOURCES  = perftest_saml2sso.cpp
perftest_saml2sso_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_saml2sso_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_saml2sso_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

