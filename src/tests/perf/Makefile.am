if XMLSEC_ENABLED
bin_PROGRAMS=perftest  perftest_saml2sso  perftest_deleg_bysechandler perftest_deleg_bydelegclient perftest_cmd_duration perftest_cmd_times
else 
bin_PROGRAMS=perftest  perftest_deleg_bysechandler perftest_deleg_bydelegclient perftest_cmd_duration perftest_cmd_times
endif

perftest_SOURCES  = perftest.cpp
perftest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

if XMLSEC_ENABLED
perftest_saml2sso_SOURCES  = perftest_saml2sso.cpp
perftest_saml2sso_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_saml2sso_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_saml2sso_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la
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

