if GACL_ENABLED
GACL_LTLIB = libgaclauth.la
GACL_LOCATION = ../external/gacl
GACL_LIB = $(GACL_LOCATION)/libgacl.la
GACL_SOURCES = identity_gacl.cpp identity_gacl.h \
               permission_gacl.cpp permission_gacl.h \
               object_access_gacl.cpp object_access_gacl.h
else
GACL_LTLIB =
GACL_LOCATION =
GACL_LIB =
GACL_SOURCES =
endif

INCLUDES = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
  $(GLOBUS_IO_CFLAGS) $(LCMAPS_INCLUDES) \
  -I$(GACL_LOCATION) $(XML_INCLUDES) \
	$(LCAS_INCLUDES) -I$(top_srcdir)/arclib


if GACL_ENABLED
noinst_LTLIBRARIES = libauth.la $(GACL_LTLIB) \
	libaccess.la libgaclaccess.la \
	libmap.la libgaclmap.la
else
noinst_LTLIBRARIES = libauth.la \
        libaccess.la \
        libmap.la
endif

libauth_la_SOURCES = auth.h auth.cpp auth_subject.cpp auth_file.cpp \
	auth_ldap.cpp auth_voms.cpp auth_lcas.cpp auth_plugin.cpp
libauth_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libauth_la_LIBADD = ../misc/libmisc.la ../run/librun.la ../conf/libconf.la \
  $(top_srcdir)/src/hed/libs/common/libarccommon.la

libaccess_la_SOURCES = identity.cpp identity.h \
	$(GACL_SOURCES) \
	identity_dn.cpp identity_dn.h identity_voms.cpp identity_voms.h \
	permission.cpp permission.h  \
	object_access.cpp object_access.h
libaccess_la_LIBADD = $(GACL_LIB) $(XML_LIBS)

libmap_la_SOURCES = unixmap.h unixmap.cpp unixmap_lcmaps.cpp \
        simplemap.h simplemap.cpp
libmap_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmap_la_LIBADD = libauth.la $(top_srcdir)/src/hed/libs/common/libarccommon.la

if GACL_ENABLED
libgaclauth_la_SOURCES = gacl_auth.cpp gacl_auth.h
libgaclauth_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgaclauth_la_LIBADD = $(GACL_LIB) $(XML_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la

libgaclaccess_la_SOURCES = identity.cpp identity.h \
	identity_gacl.cpp identity_gacl.h \
	identity_dn.cpp identity_dn.h identity_voms.cpp identity_voms.h \
	permission.cpp permission.h \
	object_access.cpp object_access.h \
	object_access_gacl.cpp object_access_gacl.h \
	gacl_auth.cpp gacl_auth.h
libgaclaccess_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgaclaccess_la_LIBADD = $(GACL_LIB) $(XML_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la

libgaclmap_la_SOURCES = unixmap.h unixmap.cpp unixmap_lcmaps.cpp \
        simplemap.h simplemap.cpp
libgaclmap_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgaclmap_la_LIBADD = $(GACL_LTLIB) $(top_srcdir)/src/hed/libs/common/libarccommon.la
endif
