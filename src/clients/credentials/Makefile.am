if XMLSEC_ENABLED
SLCS_CLIENT_PROGRAM = arcslcs
SLCS_CLIENT_MAN     = arcslcs.1
else
SLCS_CLIENT_PROGRAM =
SLCS_CLIENT_MAN     =
endif
if CANLXX_ENABLED
ARCPROXYALT_PROGRAM = arcproxyalt
else
ARCPROXYALT_PROGRAM =
endif
bin_PROGRAMS = arcproxy $(SLCS_CLIENT_PROGRAM) $(ARCPROXYALT_PROGRAM)

noinst_PROGRAMS = test2vomsserver test2myproxyserver_put test2myproxyserver_get

arcproxy_SOURCES = arcproxy.cpp
arcproxy_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
arcproxy_LDADD = \
	$(top_builddir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS)

if CANLXX_ENABLED
arcproxyalt_SOURCES = arcproxyalt.cpp
arcproxyalt_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(CANLXX_CFLAGS) \
	$(AM_CXXFLAGS)
arcproxyalt_LDADD = \
	$(top_builddir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(CANLXX_LIBS)
endif

if XMLSEC_ENABLED
arcslcs_SOURCES = arcslcs.cpp
arcslcs_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) $(AM_CXXFLAGS)
arcslcs_LDADD = \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
endif

test2vomsserver_SOURCES = test2vomsserver.cpp
test2vomsserver_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
test2vomsserver_LDADD = \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS)

test2myproxyserver_put_SOURCES = test2myproxyserver_put.cpp
test2myproxyserver_put_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
test2myproxyserver_put_LDADD = \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS)

test2myproxyserver_get_SOURCES = test2myproxyserver_get.cpp
test2myproxyserver_get_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
test2myproxyserver_get_LDADD = \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS)

man_MANS = arcproxy.1 $(SLCS_CLIENT_MAN)
