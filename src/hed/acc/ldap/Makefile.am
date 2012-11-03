pkglib_LTLIBRARIES = libaccldap.la

libaccldap_la_SOURCES  = Descriptors.cpp \
	JobListRetrieverPluginLDAPNG.cpp JobListRetrieverPluginLDAPNG.h \
	JobListRetrieverPluginLDAPGLUE2.cpp JobListRetrieverPluginLDAPGLUE2.h \
	TargetInformationRetrieverPluginLDAPNG.cpp TargetInformationRetrieverPluginLDAPNG.h \
	TargetInformationRetrieverPluginLDAPGLUE1.cpp TargetInformationRetrieverPluginLDAPGLUE1.h \
	TargetInformationRetrieverPluginLDAPGLUE2.cpp TargetInformationRetrieverPluginLDAPGLUE2.h \
	ServiceEndpointRetrieverPluginEGIIS.cpp ServiceEndpointRetrieverPluginEGIIS.h
libaccldap_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS)
libaccldap_la_LIBADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccldap_la_LDFLAGS = -no-undefined -avoid-version -module
