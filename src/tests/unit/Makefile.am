bin_PROGRAMS = Test

Test_SOURCES  = Test.cpp ClientsTest.h ClientsTest.cpp
Test_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
Test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
Test_LDADD    = \
	$(top_srcdir)/src/hed/libs/ws/libarcws.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	-lcppunit
