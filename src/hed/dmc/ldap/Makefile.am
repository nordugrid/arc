pkglib_LTLIBRARIES = libdmcldap.la

libdmcldap_la_SOURCES  = DMCLDAP.cpp DataPointLDAP.cpp LDAPQuery.cpp \
                         DMCLDAP.h   DataPointLDAP.h   LDAPQuery.h
libdmcldap_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libdmcldap_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/common/libarccommon.la -lldap_r -llber
libdmcldap_la_LDFLAGS  = -no-undefined -avoid-version -module

AM_CPPFLAGS = -DLDAP_DEPRECATED
