GACL_LOCATION = ../external/gacl-0.1.5

GACL_LIB = $(GACL_LOCATION)/libgacl.la

INCLUDES = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
  $(GLOBUS_IO_CFLAGS) $(LCMAPS_INCLUDES) \
  -I$(GACL_LOCATION) $(XML_INCLUDES) \
	$(LCAS_INCLUDES) -I$(top_srcdir)/arclib

if XMLSEC_ENABLED
GACL_AUTH_LIB = libgaclauth.la
GACL_MAP_LIB = libgaclmap.la
else
GACL_AUTH_LIB =
GACL_MAP_LIB =
endif

noinst_LTLIBRARIES = libauth.la $(GACL_AUTH_LIB) \
	libaccess.la libgaclaccess.la \
	libmap.la $(GACL_MAP_LIB)

libauth_la_SOURCES = auth.h auth.cpp auth_subject.cpp auth_file.cpp \
	auth_ldap.cpp auth_voms.cpp auth_lcas.cpp auth_plugin.cpp
libauth_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include

libauth_la_LIBADD = ../misc/libmisc.la ../run/librun.la ../conf/libconf.la \
  $(top_srcdir)/src/hed/libs/common/libarccommon.la

if XMLSEC_ENABLED
libaccess_la_SOURCES = identity.cpp identity.h \
	identity_gacl.cpp identity_gacl.h \
	identity_dn.cpp identity_dn.h identity_voms.cpp identity_voms.h \
	permission.cpp permission.h permission_gacl.cpp permission_gacl.h \
	object_access.cpp object_access.h \
	object_access_gacl.cpp object_access_gacl.h

libaccess_la_LIBADD = $(GACL_LIB) $(XML_LIBS)
else
libaccess_la_SOURCES = identity.cpp identity.h \
  identity_dn.cpp identity_dn.h identity_voms.cpp identity_voms.h \
  permission.cpp permission.h \
  object_access.cpp object_access.h
endif

if XMLSEC_ENABLED
libgaclauth_la_SOURCES = gacl_auth.cpp gacl_auth.h

libgaclauth_la_LIBADD = libauth.la $(GACL_LIB) $(XML_LIBS)

libgaclaccess_la_SOURCES = identity.cpp identity.h \
	identity_gacl.cpp identity_gacl.h \
	identity_dn.cpp identity_dn.h identity_voms.cpp identity_voms.h \
	permission.cpp permission.h permission_gacl.cpp permission_gacl.h \
	object_access.cpp object_access.h \
	object_access_gacl.cpp object_access_gacl.h \
	gacl_auth.cpp gacl_auth.h

libgaclaccess_la_LIBADD = $(GACL_LIB) $(XML_LIBS)
endif

libmap_la_SOURCES = unixmap.h unixmap.cpp unixmap_lcmaps.cpp \
	simplemap.h simplemap.cpp

libmap_la_LIBADD = libauth.la

if XMLSEC_ENABLED
libgaclmap_la_SOURCES = unixmap.h unixmap.cpp unixmap_lcmaps.cpp \
	simplemap.h simplemap.cpp

libgaclmap_la_LIBADD = libgaclauth.la
endif
