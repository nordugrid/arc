extpkglib_LTLIBRARIES = libdmcgfal.la
pkglib_LTLIBRARIES = libdmcgfaldeleg.la

libdmcgfal_la_SOURCES = DataPointGFAL.cpp DataPointGFAL.h \
                        GFALTransfer3rdParty.cpp GFALTransfer3rdParty.h \
                        GFALUtils.cpp GFALUtils.h
libdmcgfal_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GFAL2_CFLAGS) $(AM_CXXFLAGS)
libdmcgfal_la_LIBADD = \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GFAL2_LIBS)
libdmcgfal_la_LDFLAGS = -no-undefined -avoid-version -module

libdmcgfaldeleg_la_SOURCES = DataPointGFALDelegate.cpp \
                                DataPointGFALDelegate.h
libdmcgfaldeleg_la_CXXFLAGS = -I$(top_srcdir)/include \
        $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)
libdmcgfaldeleg_la_LIBADD = \
        $(top_builddir)/src/hed/libs/data/libarcdata.la \
        $(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
        $(top_builddir)/src/hed/libs/loader/libarcloader.la \
        $(top_builddir)/src/hed/libs/common/libarccommon.la \
        $(LIBXML2_LIBS) $(GLIBMM_LIBS)
libdmcgfaldeleg_la_LDFLAGS = -no-undefined -avoid-version -module

