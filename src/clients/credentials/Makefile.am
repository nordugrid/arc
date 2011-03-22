if XMLSEC_ENABLED
SLCS_CLIENT_PROGRAM = arcslcs
else
SLCS_CLIENT_PROGRAM =
endif
bin_PROGRAMS = arcproxy $(SLCS_CLIENT_PROGRAM)

noinst_PROGRAMS = test2vomsserver test2myproxyserver_put test2myproxyserver_get

arcproxy_SOURCES = arcproxy.cpp
arcproxy_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
arcproxy_LDADD = \
	$(top_srcdir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

if XMLSEC_ENABLED
arcslcs_SOURCES = arcslcs.cpp
arcslcs_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
arcslcs_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
endif

test2vomsserver_SOURCES = test2vomsserver.cpp
test2vomsserver_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test2vomsserver_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

test2myproxyserver_put_SOURCES = test2myproxyserver_put.cpp
test2myproxyserver_put_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test2myproxyserver_put_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) -lcrypto

test2myproxyserver_get_SOURCES = test2myproxyserver_get.cpp
test2myproxyserver_get_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test2myproxyserver_get_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) -lcrypto

man_MANS = arcproxy.1 arcslcs.1
