SUBDIRS = dmc

lib_LTLIBRARIES=libarcdata2.la
libarcdata2_la_SOURCES=DMC.cpp DataPoint.cpp DataPointDirect.cpp DataBufferPar.cpp CheckSum.cpp DataSpeed.cpp
libarcdata2_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs
libarcdata2_la_LIBADD=$(GLIBMM_LIBS) $(top_srcdir)/src/libs/common/libarccommon.la
