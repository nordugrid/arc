lib_LTLIBRARIES = libarcdata2.la

if WIN32
DIRS =
else
DIRS = . $(TEST_DIR)
endif

SUBDIRS = $(DIRS)
DIST_SUBDIRS = test

if WIN32
pkglibexec_SCRIPTS = 
else
pkglibexec_SCRIPTS = cache-clean cache-list
endif

libarcdata2_ladir = $(pkgincludedir)/data
libarcdata2_la_HEADERS = DataPoint.h DataPointDirect.h \
	DataPointIndex.h DataBuffer.h \
	CheckSum.h DataSpeed.h DataMover.h URLMap.h \
	DataCallback.h DataHandle.h FileInfo.h DataStatus.h \
	FileCache.h FileCacheHash.h
libarcdata2_la_SOURCES = DataPoint.cpp DataPointDirect.cpp \
	DataPointIndex.cpp DataBuffer.cpp \
	CheckSum.cpp DataSpeed.cpp DataMover.cpp URLMap.cpp \
	DataStatus.cpp \
	FileCache.cpp FileCacheHash.cpp
libarcdata2_la_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) \
	$(LIBXML2_CFLAGS) $(GTHREAD_CFLAGS) $(OPENSSL_CFLAGS) $(ZLIB_CFLAGS)
libarcdata2_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(GTHREAD_LIBS) \
	$(OPENSSL_LIBS) $(ZLIB_LIBS)
libarcdata2_la_LDFLAGS = -version-info 1:0:0

if WIN32
man_MANS = 
else
man_MANS = cache-clean.1 cache-list.1
endif

EXTRA_DIST = cache-clean cache-list
