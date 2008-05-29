pkglib_LTLIBRARIES = libaccCREAM.la

libaccCREAM_la_SOURCES  = TargetRetrieverCREAM.cpp TargetRetrieverCREAM.h
libaccCREAM_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libaccCREAM_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la
libaccCREAM_la_LDFLAGS  = -no-undefined -avoid-version -module
