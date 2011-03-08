TESTS = PluginTest

noinst_LTLIBRARIES = libtestplugin.la
check_PROGRAMS = $(TESTS)

libtestplugin_la_SOURCES  = TestPlugin.cpp
libtestplugin_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libtestplugin_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcloader.la
libtestplugin_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module -rpath $(CURDIR)

PluginTest_SOURCES = $(top_srcdir)/src/Test.cpp PluginTest.cpp
PluginTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
PluginTest_LDADD = $(CPPUNIT_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcloader.la

