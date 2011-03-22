TESTS = CredentialTest VOMSUtilTest listfuncTest
check_PROGRAMS = $(TESTS)

CredentialTest_SOURCES = $(top_srcdir)/src/Test.cpp CredentialTest.cpp
CredentialTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
CredentialTest_LDADD = \
	../libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

VOMSUtilTest_SOURCES = $(top_srcdir)/src/Test.cpp VOMSUtilTest.cpp
VOMSUtilTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
VOMSUtilTest_LDADD = \
	../libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

listfuncTest_SOURCES = $(top_srcdir)/src/Test.cpp listfuncTest.cpp
listfuncTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
listfuncTest_LDADD = \
	../libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

noinst_DATA = ca_key.pem ca_cert.pem

ca_key.pem ca_cert.pem:
	openssl req -subj "/O=Grid/OU=ARC/CN=ARC CA" -set_serial 0 -passout pass:password -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem

# Make the CA credentials part of the distribution so as not to depend on openssl binary during testing
EXTRA_DIST = ca_key.pem ca_cert.pem
