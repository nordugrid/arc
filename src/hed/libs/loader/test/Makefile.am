TESTS = PluginTest

check_LTLIBRARIES = libtestplugin.la
check_PROGRAMS = $(TESTS)

libtestplugin_la_SOURCES = TestPlugin.cpp
libtestplugin_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libtestplugin_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)
libtestplugin_la_LDFLAGS = -no-undefined -avoid-version -module -rpath $(CURDIR)

PluginTest_SOURCES = $(top_srcdir)/src/Test.cpp PluginTest.cpp
PluginTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
PluginTest_LDADD = \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS)
