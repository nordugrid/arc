bin_PROGRAMS = arcecho

arcecho_SOURCES = arcecho.cpp
arcecho_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcecho_LDADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

man_MANS = arcecho.1
