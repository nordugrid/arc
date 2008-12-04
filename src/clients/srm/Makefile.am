bin_PROGRAMS = arcsrmping

arcsrmping_SOURCES  = arcsrmping.cpp
arcsrmping_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcsrmping_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

man_MANS = arcsrmping.1
