pkglib_LTLIBRARIES = libaccCREAM.la

libaccCREAM_la_SOURCES  = DescriptorsCREAM.cpp \
	TargetRetrieverCREAM.cpp TargetRetrieverCREAM.h \
	SubmitterCREAM.cpp SubmitterCREAM.h \
	JobControllerCREAM.cpp JobControllerCREAM.h \
	CREAMClient.cpp CREAMClient.h \
	OpenSSLFunctions.cpp OpenSSLFunctions.h
libaccCREAM_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
	$(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libaccCREAM_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(OPENSSL_LIBS) \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la
libaccCREAM_la_LDFLAGS  = -no-undefined -avoid-version -module
