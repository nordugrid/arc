#DIST_SUBDIRS = test
#SUBDIRS = $(TEST_DIR)

lib_LTLIBRARIES = libarcotokens.la

pgmpkglibdir = $(pkglibdir)
pgmpkglib_PROGRAMS =

libarcotokens_ladir = $(pkgincludedir)
libarcotokens_la_HEADERS = otokens.h openid_metadata.h
libarcotokens_la_SOURCES = jwse.cpp jwse_hmac.cpp jwse_ecdsa.cpp jwse_rsassapkcs1.cpp jwse_rsassapss.cpp jwse_keys.cpp openid_metadata.cpp jwse_private.h
libarcotokens_la_CXXFLAGS = -I$(top_srcdir)/include $(OPENSSL_CFLAGS) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)
libarcotokens_la_LIBADD = \
        $(top_builddir)/src/external/cJSON/libcjson.la \
        $(top_builddir)/src/hed/libs/common/libarccommon.la \
        $(top_builddir)/src/hed/libs/message/libarcmessage.la \
        $(top_builddir)/src/hed/libs/credential/libarccredential.la \
        $(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
        $(top_builddir)/src/hed/libs/communication/libarccommunication.la \
        $(OPENSSL_LIBS) $(GLIBMM_LIBS) $(LIBINTL)
libarcotokens_la_LDFLAGS = -version-info 3:0:0

