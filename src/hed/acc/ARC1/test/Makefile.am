TESTS = TargetRetrieverARC1Test AREXClientTest
check_PROGRAMS = $(TESTS)

TargetRetrieverARC1Test_SOURCES = $(top_srcdir)/src/Test.cpp \
	TargetRetrieverARC1Test.cpp \
	../TargetRetrieverARC1.cpp ../AREXClient.cpp \
	../JobStateARC1.cpp ../JobStateBES.cpp
TargetRetrieverARC1Test_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
TargetRetrieverARC1Test_LDADD = \
	$(top_srcdir)/src/hed/libs/ws/libarcws.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
	$(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la\
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

AREXClientTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	../../../libs/client/test/SimulatorClasses.h \
	../../../libs/client/test/SimulatorClasses.cpp \
	AREXClientTest.cpp ../AREXClient.cpp \
	../JobStateARC1.cpp ../JobStateBES.cpp
AREXClientTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -DCPPUNITTEST
AREXClientTest_LDADD = \
	$(top_srcdir)/src/hed/libs/ws/libarcws.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)
