noinst_LTLIBRARIES = libmisc.la

if LDAP_ENABLED
libmisc_la_SOURCES=ldapquery.cpp escaped.cpp canonical_dir.cpp ldapquery.h escaped.h canonical_dir.h proxy.cpp proxy.h
else
libmisc_la_SOURCES=escaped.cpp canonical_dir.cpp escaped.h canonical_dir.h proxy.cpp proxy.h
endif

libmisc_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_GSSAPI_GSI_CFLAGS) -I$(top_srcdir)/include
libmisc_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/credential/libarccredential.la $(top_srcdir)/src/hed/libs/credentialstore/libarccredentialstore.la $(LDAP_LIBS)
