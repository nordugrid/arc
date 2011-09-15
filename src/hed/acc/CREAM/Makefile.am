pkglib_LTLIBRARIES = libaccCREAM.la

libaccCREAM_la_SOURCES = DescriptorsCREAM.cpp \
	TargetRetrieverCREAM.cpp TargetRetrieverCREAM.h \
	SubmitterCREAM.cpp SubmitterCREAM.h \
	JobControllerCREAM.cpp JobControllerCREAM.h \
	CREAMClient.cpp CREAMClient.h \
	JobStateCREAM.cpp JobStateCREAM.h
libaccCREAM_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS)
libaccCREAM_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(OPENSSL_LIBS)
libaccCREAM_la_LDFLAGS = -no-undefined -avoid-version -module
