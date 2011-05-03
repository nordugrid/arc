#noinst_PROGRAMS = test testinterface_arc testinterface_xacml

pkglib_LTLIBRARIES = libarcshclegacy.la

#libarcshclegacy_la_SOURCES = groups_shc.cpp groups_pdp.cpp groups_map.cpp
libarcshclegacy_la_SOURCES = auth_file.cpp auth_subject.cpp \
                             auth_plugin.cpp auth_lcas.cpp auth_ldap.cpp \
                             auth_voms.cpp auth.cpp auth.h
libarcshclegacy_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_COMMON_CFLAGS)
libarcshclegacy_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)
libarcshclegacy_la_LDFLAGS = -no-undefined -avoid-version -module
