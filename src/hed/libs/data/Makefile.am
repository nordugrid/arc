lib_LTLIBRARIES = libarcdata.la

DIRS = $(TEST_DIR) examples

SUBDIRS = $(DIRS)
DIST_SUBDIRS = test examples
EXTRA_DIST = cache-clean cache-list

pkglibexec_SCRIPTS = cache-clean cache-list

libarcdata_ladir = $(pkgincludedir)/data
libarcdata_la_HEADERS = DataPoint.h DataPointDirect.h \
	DataPointIndex.h DataBuffer.h \
	DataSpeed.h DataMover.h URLMap.h \
	DataCallback.h DataHandle.h FileInfo.h DataStatus.h \
	FileCache.h FileCacheHash.h
libarcdata_la_SOURCES = DataPoint.cpp DataPointDirect.cpp \
	DataPointIndex.cpp DataBuffer.cpp \
	DataSpeed.cpp DataMover.cpp URLMap.cpp \
	DataStatus.cpp \
	FileCache.cpp FileCacheHash.cpp
libarcdata_la_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) \
	$(LIBXML2_CFLAGS) $(GTHREAD_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
libarcdata_la_LIBADD = \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(GTHREAD_LIBS) \
	$(OPENSSL_LIBS)
libarcdata_la_LDFLAGS = -version-info 3:0:0

man_MANS = cache-clean.1 cache-list.1
