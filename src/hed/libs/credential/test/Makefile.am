TESTS = CredentialTest VOMSUtilTest listfuncTest
check_PROGRAMS = $(TESTS)

CredentialTest_SOURCES = $(top_srcdir)/src/Test.cpp CredentialTest.cpp
CredentialTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
CredentialTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarccredential.la #$(top_srcdir)/src/hed/libs/client/libarcclient.la

VOMSUtilTest_SOURCES = $(top_srcdir)/src/Test.cpp VOMSUtilTest.cpp
VOMSUtilTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
VOMSUtilTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarccredential.la #$(top_srcdir)/src/hed/libs/client/libarcclient.la

listfuncTest_SOURCES = $(top_srcdir)/src/Test.cpp listfuncTest.cpp
listfuncTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
listfuncTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarccredential.la

noinst_DATA = ca_key.pem ca_cert.pem

ca_key.pem ca_cert.pem:
	openssl req -subj "/O=Grid/OU=ARC/CN=ARC CA" -set_serial 0 -passout pass:password -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem

# Make the CA credentials part of the distribution so as not to depend on openssl binary during testing
EXTRA_DIST = ca_key.pem ca_cert.pem
