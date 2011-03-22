TESTS = InformationInterfaceTest InfoFilterTest RegisteredServiceTest
check_PROGRAMS = $(TESTS)

InformationInterfaceTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	InformationInterfaceTest.cpp
InformationInterfaceTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
InformationInterfaceTest_LDADD = ../libinfosys.la $(CPPUNIT_LIBS)

InfoFilterTest_SOURCES = $(top_srcdir)/src/Test.cpp InfoFilterTest.cpp
InfoFilterTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
InfoFilterTest_LDADD = ../libinfosys.la $(CPPUNIT_LIBS)

RegisteredServiceTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	RegisteredServiceTest.cpp
RegisteredServiceTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
RegisteredServiceTest_LDADD = \
	../libinfosys.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(CPPUNIT_LIBS)
