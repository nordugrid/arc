lib_LTLIBRARIES=libarcdata2.la

libarcdata2_ladir=$(pkgincludedir)/data
libarcdata2_la_HEADERS=DMC.h   DataPoint.h   DataPointDirect.h   \
	DataBufferPar.h   CheckSum.h   DataSpeed.h   DataHandle.h
libarcdata2_la_SOURCES=DMC.cpp DataPoint.cpp DataPointDirect.cpp \
	DataBufferPar.cpp CheckSum.cpp DataSpeed.cpp

libarcdata2_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcdata2_la_LIBADD=$(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la
