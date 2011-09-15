pkglib_LTLIBRARIES = libaccARC1.la
noinst_LTLIBRARIES = libarexclient.la

libaccARC1_la_SOURCES  = TargetRetrieverARC1.cpp TargetRetrieverARC1.h \
	SubmitterARC1.cpp SubmitterARC1.h \
	JobControllerARC1.cpp JobControllerARC1.h \
	AREXClient.cpp AREXClient.h \
	DescriptorsARC1.cpp JobStateARC1.h JobStateARC1.cpp \
	TargetRetrieverBES.cpp TargetRetrieverBES.h \
	SubmitterBES.cpp SubmitterBES.h \
	JobControllerBES.cpp JobControllerBES.h \
	JobStateBES.cpp JobStateBES.h
libaccARC1_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccARC1_la_LIBADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la\
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccARC1_la_LDFLAGS = -no-undefined -avoid-version -module

libarexclient_la_SOURCES = AREXClient.cpp AREXClient.h
libarexclient_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarexclient_la_LIBADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
