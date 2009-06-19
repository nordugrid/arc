pkglib_LTLIBRARIES = libdmclfc.la

libdmclfc_la_SOURCES  = DMCLFC.cpp DataPointLFC.cpp \
                        DMCLFC.h   DataPointLFC.h
libdmclfc_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(LFC_CFLAGS) -I$(top_srcdir)/include
libdmclfc_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LFC_LIBS) $(LIBXML2_LIBS) $(GLIBMM_LIBS) 
libdmclfc_la_LDFLAGS  = -no-undefined -avoid-version -module
