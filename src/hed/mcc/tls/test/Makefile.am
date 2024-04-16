TESTS = GlobusSigningPolicyTest

check_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = srcdir=$(srcdir)

GlobusSigningPolicyTest_SOURCES = $(top_srcdir)/src/Test.cpp GlobusSigningPolicyTest.cpp ../GlobusSigningPolicy.cpp
GlobusSigningPolicyTest_CXXFLAGS = -I$(top_srcdir)/include $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)
GlobusSigningPolicyTest_LDADD = $(top_builddir)/src/hed/libs/common/libarccommon.la $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(OPENSSL_LIBS)
