TESTS = TargetRetrieverARC1Test AREXClientTest
check_PROGRAMS = $(TESTS)

TargetRetrieverARC1Test_SOURCES = $(top_srcdir)/src/Test.cpp TargetRetrieverARC1Test.cpp
TargetRetrieverARC1Test_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
TargetRetrieverARC1Test_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                                $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
                                $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                                $(top_srcdir)/src/hed/libs/message/libarcmessage.la\
                                $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                                $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                                $(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
                                $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
                                $(top_srcdir)/src/hed/libs/ws/libarcws.la \
                                ../.libs/libaccARC1_la-TargetRetrieverARC1.o ../.libs/libaccARC1_la-AREXClient.o ../.libs/libaccARC1_la-JobStateARC1.o ../.libs/libaccARC1_la-JobStateBES.o

AREXClientTest_SOURCES  = $(top_srcdir)/src/Test.cpp ../../../libs/client/test/SimulatorClasses.h ../../../libs/client/test/SimulatorClasses.cpp AREXClientTest.cpp ../AREXClient.cpp ../JobStateARC1.cpp ../JobStateBES.cpp
AREXClientTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include -DCPPUNITTEST
AREXClientTest_LDADD    = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                          $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                          $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                          $(top_srcdir)/src/hed/libs/message/libarcmessage.la\
                          $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
                          $(top_srcdir)/src/hed/libs/ws/libarcws.la
