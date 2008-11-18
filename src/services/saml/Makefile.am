if MYSQL_LIBRARY_ENABLED
pkglib_LTLIBRARIES = libaa.la libsaml2sp.la
libaa_la_SOURCES  = aaservice.cpp aaservice.h
libaa_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) $(MYSQL_CFLAGS) -I$(top_srcdir)/include
libaa_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                    $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                    $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
                    $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                    $(GLIBMM_LIBS) $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS) $(MYSQL_LIBS)
libaa_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module

libsaml2sp_la_SOURCES  = SPService.cpp SPService.h
libsaml2sp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
libsaml2sp_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                         $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                         $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
                         $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
libsaml2sp_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module


else
pkglib_LTLIBRARIES = libsaml2sp.la

libsaml2sp_la_SOURCES  = SPService.cpp SPService.h
libsaml2sp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
libsaml2sp_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                         $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                         $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
                         $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
libsaml2sp_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module

endif

noinst_PROGRAMS = test_service test_client test_client2shibboleth test_client2voms_saml_service
#test login_tests saml_tests

#login_tests_SOURCES = login_tests.cc
#login_tests_CXXFLAGS = $(LASSO_CFLAGS) -DTESTSDATADIR=\"$(srcdir)/data/\"
#login_tests_LDADD = $(LASSO_LIBS) 

#saml_tests_SOURCES = saml_tests.cc
#saml_tests_CXXFLAGS = $(LASSO_CFLAGS) -DTESTSDATADIR=\"$(srcdir)/data/\"
#saml_tests_LDADD = $(LASSO_LIBS)

#test_SOURCES  = test.cpp
#test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(LASSO_CFLAGS) -I$(top_srcdir)/include -DTESTSDATADIR=\"$(srcdir)/data/\"
#test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(LASSO_LIBS)

test_client_SOURCES  = test_client.cpp
test_client_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
test_client_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                       $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                       $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                       $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                       $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
                       $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_LIBS)

test_client2shibboleth_SOURCES  = test_client2shibboleth.cpp
test_client2shibboleth_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
test_client2shibboleth_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                       $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                       $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                       $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                       $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
                       $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                       $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_LIBS)

test_client2voms_saml_service_SOURCES  = test_client2voms_saml_service.cpp
test_client2voms_saml_service_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
test_client2voms_saml_service_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                       $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                       $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                       $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                       $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
                       $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                       $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_LIBS)

test_service_SOURCES  = test_service.cpp
test_service_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_service_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                        $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                        $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                        $(GLIBMM_LIBS) $(LIBXML2_LIBS)

