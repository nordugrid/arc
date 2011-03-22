SUBDIRS = schema

if MYSQL_LIBRARY_ENABLED

pkglib_LTLIBRARIES = libaa.la libsaml2sp.la

libaa_la_SOURCES = aaservice.cpp aaservice.h
libaa_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) $(MYSQL_CFLAGS)
libaa_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) \
	$(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS) $(MYSQL_LIBS)
libaa_la_LDFLAGS = -no-undefined -avoid-version -module

libsaml2sp_la_SOURCES = SPService.cpp SPService.h
libsaml2sp_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
libsaml2sp_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/ws/libarcws.la \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
libsaml2sp_la_LDFLAGS = -no-undefined -avoid-version -module

else

pkglib_LTLIBRARIES = libsaml2sp.la

libsaml2sp_la_SOURCES = SPService.cpp SPService.h
libsaml2sp_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
libsaml2sp_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/ws/libarcws.la \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
libsaml2sp_la_LDFLAGS = -no-undefined -avoid-version -module

endif

noinst_PROGRAMS = test_service test_client \
	test_client2shibboleth test_client2voms_saml_service

test_client_SOURCES = test_client.cpp
test_client_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS)
test_client_LDADD = \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_LIBS)

test_client2shibboleth_SOURCES = test_client2shibboleth.cpp
test_client2shibboleth_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS)
test_client2shibboleth_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_LIBS)

test_client2voms_saml_service_SOURCES = test_client2voms_saml_service.cpp
test_client2voms_saml_service_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS)
test_client2voms_saml_service_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_LIBS)

test_service_SOURCES = test_service.cpp
test_service_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test_service_LDADD = \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)
