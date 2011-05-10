SUBDIRS = schema
DIST_SUBDIRS = schema

pkglib_LTLIBRARIES = libarcshclegacy.la
pkglibexec_PROGRAMS = arc-lcas arc-lcmaps

libarcshclegacy_la_SOURCES = auth_file.cpp auth_subject.cpp \
                             auth_plugin.cpp auth_lcas.cpp auth_ldap.cpp \
                             auth_voms.cpp auth.cpp auth.h \
                             simplemap.cpp simplemap.h \
                             unixmap_lcmaps.cpp unixmap.cpp unixmap.h \
                             ConfigParser.cpp ConfigParser.h \
                             LegacySecAttr.cpp LegacySecAttr.h \
                             LegacySecHandler.cpp LegacySecHandler.h \
                             LegacyPDP.cpp LegacyPDP.h \
                             LegacyMap.cpp LegacyMap.h \
                             plugin.cpp
libarcshclegacy_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libarcshclegacy_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)
libarcshclegacy_la_LDFLAGS = -no-undefined -avoid-version -module

arc_lcas_SOURCES  = arc_lcas.cpp
arc_lcas_CXXFLAGS = -I$(top_srcdir)/include \
        $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GTHREAD_CFLAGS) $(MYSQL_CFLAGS) $(GLOBUS_COMMON_CFLAGS)
arc_lcas_LDADD    = \
	$(top_srcdir)/src/hed/libs/globusutils/libarcglobusutils.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la

arc_lcmaps_SOURCES  = arc_lcmaps.cpp
arc_lcmaps_CXXFLAGS = -I$(top_srcdir)/include \
        $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GTHREAD_CFLAGS) $(MYSQL_CFLAGS) $(GLOBUS_COMMON_CFLAGS)
arc_lcmaps_LDADD    = \
	$(top_srcdir)/src/hed/libs/globusutils/libarcglobusutils.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la

