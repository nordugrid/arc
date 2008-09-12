pkglib_LTLIBRARIES = libaa.la
noinst_PROGRAMS = login_tests saml_tests test test_service test_client

libaa_la_SOURCES  = aaservice.cpp aaservice.h
libaa_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(LASSO_CFLAGS)$(MYSQL_CFLAGS) -I$(top_srcdir)/include
libaa_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                    $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                    $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                    $(GLIBMM_LIBS) $(LASSO_LIBS) $(MYSQL_LIBS)
libaa_la_LDFLAGS  = -no-undefined -avoid-version -module

login_tests_SOURCES = login_tests.cc
login_tests_CXXFLAGS = $(LASSO_CFLAGS) -DTESTSDATADIR=\"$(srcdir)/data/\"
login_tests_LDADD = $(LASSO_LIBS) 

saml_tests_SOURCES = saml_tests.cc
saml_tests_CXXFLAGS = $(LASSO_CFLAGS) -DTESTSDATADIR=\"$(srcdir)/data/\"
saml_tests_LDADD = $(LASSO_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(LASSO_CFLAGS) -I$(top_srcdir)/include -DTESTSDATADIR=\"$(srcdir)/data/\"
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(LASSO_LIBS)

test_client_SOURCES  = test_client.cpp
test_client_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(LASSO_CFLAGS) -I$(top_srcdir)/include
test_client_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                       $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                       $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                       $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                       $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(LASSO_LIBS)

test_service_SOURCES  = test_service.cpp
test_service_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(LASSO_CFLAGS) -I$(top_srcdir)/include
test_service_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                        $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                        $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                        $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(LASSO_LIBS)

