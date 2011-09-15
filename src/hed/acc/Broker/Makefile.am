pkglib_LTLIBRARIES = libaccBroker.la

libaccBroker_la_SOURCES = \
	FastestQueueBroker.cpp FastestQueueBroker.h \
	RandomBroker.cpp RandomBroker.h \
	DescriptorsBroker.cpp \
	BenchmarkBroker.cpp BenchmarkBroker.h \
	DataBroker.cpp DataBroker.h 
libaccBroker_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccBroker_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccBroker_la_LDFLAGS  = -no-undefined -avoid-version -module

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
