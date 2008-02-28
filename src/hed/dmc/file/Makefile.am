pkglib_LTLIBRARIES = libdmcfile.la

libdmcfile_la_SOURCES  = DMCFile.cpp DataPointFile.cpp \
                         DMCFile.h   DataPointFile.h
libdmcfile_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libdmcfile_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libdmcfile_la_LDFLAGS  = -no-undefined -avoid-version -module
