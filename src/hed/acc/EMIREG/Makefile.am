pkglib_LTLIBRARIES = libaccEMIREG.la

libaccEMIREG_la_SOURCES  = TargetRetrieverEMIREG.cpp TargetRetrieverEMIREG.h \
	DescriptorsEMIREG.cpp
libaccEMIREG_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccEMIREG_la_LIBADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la\
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccEMIREG_la_LDFLAGS = -no-undefined -avoid-version -module

