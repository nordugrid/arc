bin_PROGRAMS = arcecho

arcecho_SOURCES  = arcecho.cpp
arcecho_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
arcecho_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

man_MANS = arcecho.1
