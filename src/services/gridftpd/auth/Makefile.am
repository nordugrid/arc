if GACL_ENABLED
GACL_CFLAGS = -I../external/gacl
GACL_LIB = ../external/gacl/libgacl.la
GACL_SOURCES = identity_gacl.cpp identity_gacl.h \
               permission_gacl.cpp permission_gacl.h \
               object_access_gacl.cpp object_access_gacl.h
GACL_AUTH_SOURCES = gacl_auth.cpp gacl_auth.h
GACL_ACCESS_SOURCES = identity_gacl.cpp identity_gacl.h object_access_gacl.cpp object_access_gacl.h gacl_auth.cpp gacl_auth.h
GACL_MAP_SOURCES =
else
GACL_CFLAGS =
GACL_LIB =
GACL_SOURCES =
GACL_AUTH_SOURCES =
GACL_ACCESS_SOURCES =
GACL_MAP_SOURCES =
endif

noinst_LTLIBRARIES = libauth.la libaccess.la libmap.la

libauth_la_SOURCES = auth.h auth.cpp auth_subject.cpp auth_file.cpp \
	auth_ldap.cpp auth_voms.cpp auth_lcas.cpp auth_plugin.cpp \
	$(GACL_AUTH_SOURCES)
libauth_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_IO_CFLAGS) $(LCAS_INCLUDES) $(LCMAPS_INCLUDES) $(GACL_CFLAGS) -I$(top_srcdir)/include
libauth_la_LIBADD = ../misc/libmisc.la ../run/librun.la ../conf/libconf.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la $(GACL_LIB)

libaccess_la_SOURCES = identity.cpp identity.h \
	$(GACL_SOURCES) \
	identity_dn.cpp identity_dn.h identity_voms.cpp identity_voms.h \
	permission.cpp permission.h  \
	object_access.cpp object_access.h \
	$(GACL_ACCESS_SOURCES)
libaccess_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_IO_CFLAGS) $(LCAS_INCLUDES) $(LCMAPS_INCLUDES) $(GACL_CFLAGS) -I$(top_srcdir)/include
libaccess_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(GACL_LIB) $(XML_LIBS)

libmap_la_SOURCES = unixmap.h unixmap.cpp unixmap_lcmaps.cpp \
	simplemap.h simplemap.cpp \
	$(GACL_MAP_SOURCES)
libmap_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_IO_CFLAGS) $(LCAS_INCLUDES) $(LCMAPS_INCLUDES) $(GACL_CFLAGS) -I$(top_srcdir)/include
libmap_la_LIBADD = libauth.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
