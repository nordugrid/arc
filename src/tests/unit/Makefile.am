bin_PROGRAMS = Test

Test_SOURCES  = Test.cpp ClientsTest.h ClientsTest.cpp
Test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
Test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
Test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                    $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                    $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                    $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
                /usr/lib/libcppunit.la \
                $(top_srcdir)/src/hed/libs/ws/libarcws.la

