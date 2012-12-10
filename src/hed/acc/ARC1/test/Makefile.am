TESTS = AREXClientTest
check_PROGRAMS = $(TESTS)

AREXClientTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	$(top_srcdir)/src/hed/libs/communication/test/SimulatorClasses.h \
	$(top_srcdir)/src/hed/libs/communication/test/SimulatorClasses.cpp \
	AREXClientTest.cpp $(srcdir)/../AREXClient.cpp \
	$(srcdir)/../JobStateARC1.cpp $(srcdir)/../JobStateBES.cpp
AREXClientTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -DCPPUNITTEST $(AM_CXXFLAGS)
AREXClientTest_LDADD = \
	$(top_builddir)/src/hed/libs/infosys/libarcinfosys.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)
