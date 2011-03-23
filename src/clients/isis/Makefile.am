#noinst_PROGRAMS = isistest
bin_PROGRAMS = isistest

man_MANS = isistest.1

isistest_SOURCES = isistest.cpp
isistest_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
isistest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)
