noinst_LTLIBRARIES = libmisc.la

if LDAP_ENABLED
libmisc_la_SOURCES = \
	ldapquery.cpp escaped.cpp canonical_dir.cpp proxy.cpp \
	ldapquery.h   escaped.h   canonical_dir.h   proxy.h
else
libmisc_la_SOURCES = \
	escaped.cpp canonical_dir.cpp proxy.cpp \
	escaped.h   canonical_dir.h   proxy.h
endif

libmisc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_GSSAPI_GSI_CFLAGS)
libmisc_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LDAP_LIBS) -lpthread
