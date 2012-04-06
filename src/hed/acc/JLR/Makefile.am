pkglib_LTLIBRARIES = libaccJLR.la

libaccJLR_la_SOURCES  = DescriptorsJLR.cpp \
	JobListRetrieverPluginLDAPNG.cpp JobListRetrieverPluginLDAPNG.h \
	JobListRetrieverPluginWSRFBES.cpp JobListRetrieverPluginWSRFBES.h \
	JobListRetrieverPluginWSRFGLUE2.cpp JobListRetrieverPluginWSRFGLUE2.h
libaccJLR_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccJLR_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccJLR_la_LDFLAGS = -no-undefined -avoid-version -module
