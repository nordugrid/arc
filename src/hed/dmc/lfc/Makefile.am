pkglib_LTLIBRARIES = libdmclfc.la

libdmclfc_la_SOURCES = DMCLFC.cpp DataPointLFC.cpp \
                       DMCLFC.h   DataPointLFC.h
libdmclfc_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I/usr/include/lfc -I$(top_srcdir)/include
libdmclfc_la_LIBADD = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la  $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la -llfc -luuid
libdmclfc_la_LDFLAGS = -no-undefined -avoid-version -module
