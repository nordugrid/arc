pkglib_LTLIBRARIES = libdmcldap.la

libdmcldap_la_SOURCES = DataPointLDAP.cpp LDAPQuery.cpp \
                        DataPointLDAP.h   LDAPQuery.h
libdmcldap_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)

if WIN32
THR_LIBS =
else
THR_LIBS = -lpthread
endif

libdmcldap_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(LDAP_LIBS) $(THR_LIBS)
libdmcldap_la_LDFLAGS = -no-undefined -avoid-version -module
