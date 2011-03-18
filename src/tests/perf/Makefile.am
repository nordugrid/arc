if XMLSEC_ENABLED
noinst_PROGRAMS=perftest  perftest_saml2sso perftest_slcs perftest_deleg_bysechandler perftest_deleg_bydelegclient perftest_cmd_duration perftest_cmd_times perftest_msgsize perftest_samlaa
else 
if WIN32
noinst_PROGRAMS=perftest  perftest_deleg_bysechandler perftest_deleg_bydelegclient
else
noinst_PROGRAMS=perftest  perftest_deleg_bysechandler perftest_deleg_bydelegclient perftest_cmd_duration perftest_cmd_times perftest_msgsize
endif
endif

perftest_SOURCES  = perftest.cpp
perftest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

perftest_msgsize_SOURCES  = perftest_msgsize.cpp
perftest_msgsize_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_msgsize_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_msgsize_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

if XMLSEC_ENABLED
perftest_samlaa_SOURCES  = perftest_samlaa.cpp
perftest_samlaa_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
perftest_samlaa_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)  $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
perftest_samlaa_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la $(top_srcdir)/src/hed/libs/credential/libarccredential.la

perftest_saml2sso_SOURCES  = perftest_saml2sso.cpp
perftest_saml2sso_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
perftest_saml2sso_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)  $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
perftest_saml2sso_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la

perftest_slcs_SOURCES  = perftest_slcs.cpp
perftest_slcs_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
perftest_slcs_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)  $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
perftest_slcs_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la $(top_srcdir)/src/hed/libs/credential/libarccredential.la
endif

perftest_deleg_bysechandler_SOURCES  = perftest_deleg_bysechandler.cpp
perftest_deleg_bysechandler_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_deleg_bysechandler_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_deleg_bysechandler_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

perftest_deleg_bydelegclient_SOURCES  = perftest_deleg_bydelegclient.cpp
perftest_deleg_bydelegclient_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_deleg_bydelegclient_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_deleg_bydelegclient_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

perftest_cmd_duration_SOURCES  = perftest_cmd_duration.cpp
perftest_cmd_duration_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_cmd_duration_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_cmd_duration_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

perftest_cmd_times_SOURCES  = perftest_cmd_times.cpp
perftest_cmd_times_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_cmd_times_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_cmd_times_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

