pkglib_LTLIBRARIES = libdmclfc.la

libdmclfc_la_SOURCES = DMCLFC.cpp DataPointLFC.cpp \
                       DMCLFC.h   DataPointLFC.h
libdmclfc_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I/usr/include/lfc -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libdmclfc_la_LIBADD = $(LIBXML2_LIBS) $(GLIBMM_LIBS) -llfc -luuid
libdmclfc_la_LDFLAGS = -no-undefined -avoid-version -module
