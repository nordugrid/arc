lib_LTLIBRARIES=libarcdata2.la

SUBDIRS = . $(TEST_DIR)
DIST_SUBDIRS = test

dist_pkglibexec_SCRIPTS = cache-clean cache-list

libarcdata2_ladir = $(pkgincludedir)/data
libarcdata2_la_HEADERS  = DMC.h DMCLoader.h DataPoint.h DataPointDirect.h DataPointIndex.h DataBuffer.h \
                          CheckSum.h DataSpeed.h DataMover.h URLMap.h \
                          DataCallback.h DataHandle.h FileInfo.h DataStatus.h \
                          MkDirRecursive.h FileCache.h FileCacheHash.h
libarcdata2_la_SOURCES  = DMC.cpp DMCLoader.cpp DataPoint.cpp DataPointDirect.cpp \
                          DataPointIndex.cpp DataBuffer.cpp CheckSum.cpp DataSpeed.cpp \
                          DataMover.cpp URLMap.cpp DataStatus.cpp \
                          MkDirRecursive.cpp FileCache.cpp FileCacheHash.cpp
libarcdata2_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarcdata2_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(GLIBMM_LIBS) $(OPENSSL_LIBS)

man_MANS = cache-clean.1 cache-list.1
