pkglib_LTLIBRARIES = libaccTIR.la

libaccTIR_la_SOURCES  = DescriptorsTIR.cpp \
	TargetInformationRetrieverPluginBES.cpp TargetInformationRetrieverPluginBES.h \
	TargetInformationRetrieverPluginWSRFGLUE2.cpp TargetInformationRetrieverPluginWSRFGLUE2.h \
	TargetInformationRetrieverPluginLDAPNG.cpp TargetInformationRetrieverPluginLDAPNG.h \
	TargetInformationRetrieverPluginLDAPGLUE1.cpp TargetInformationRetrieverPluginLDAPGLUE1.h \
	TargetInformationRetrieverPluginLDAPGLUE2.cpp TargetInformationRetrieverPluginLDAPGLUE2.h \
	TargetInformationRetrieverPluginEMIES.cpp TargetInformationRetrieverPluginEMIES.h
libaccTIR_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccTIR_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccTIR_la_LDFLAGS = -no-undefined -avoid-version -module
