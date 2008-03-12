noinst_PROGRAMS=test
lib_LTLIBRARIES=libarcdata2.la

libarcdata2_ladir = $(pkgincludedir)/data
libarcdata2_la_HEADERS  = DMC.h DataPoint.h DataPointDirect.h DataPointIndex.h DataBufferPar.h \
                          CheckSum.h DataSpeed.h Cache.h DataMover.h DataCache.h URLMap.h \
                          DataCallback.h DataHandle.h FileInfo.h DataStatus.h
libarcdata2_la_SOURCES  = DMC.cpp DataPoint.cpp DataPointDirect.cpp \
                          DataPointIndex.cpp DataBufferPar.cpp CheckSum.cpp DataSpeed.cpp \
                          Cache.cpp DataMover.cpp DataCache.cpp URLMap.cpp \
                          MkDirRecursive.cpp MkDirRecursive.h CheckFile.h
libarcdata2_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcdata2_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(GLIBMM_LIBS) -lpthread

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDADD    = libarcdata2.la ../loader/libarcloader.la
