#include <cppunit/extensions/HelperMacros.h>

#include <arc/URL.h>

#include <string>

class URLTest : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE( URLTest );
	CPPUNIT_TEST( TestGsiftpUrl );
	CPPUNIT_TEST( TestLdapUrl );
	CPPUNIT_TEST( TestHttpUrl );
	CPPUNIT_TEST( TestFileUrl );
	CPPUNIT_TEST( TestLdapUrl2 );
	CPPUNIT_TEST( TestOptUrl );
	CPPUNIT_TEST( TestFtpUrl );
	CPPUNIT_TEST( TestRlsUrl );
	CPPUNIT_TEST( TestRlsUrl2 );
	CPPUNIT_TEST( TestRlsUrl3 );
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp();
		void tearDown();
		void TestGsiftpUrl();
		void TestLdapUrl();
		void TestHttpUrl();
		void TestFileUrl();
		void TestLdapUrl2();
		void TestOptUrl();
		void TestFtpUrl();
		void TestRlsUrl();
		void TestRlsUrl2();
		void TestRlsUrl3();

	private:
		Arc::URL *gsiftpurl, *ldapurl, *httpurl, *fileurl, *ldapurl2, *opturl, *ftpurl, *rlsurl, *rlsurl2, *rlsurl3;
};


void URLTest::setUp() {
	gsiftpurl = new Arc::URL("gsiftp://hathi.hep.lu.se/public/test.txt");
	ldapurl = new Arc::URL("ldap://grid.uio.no/o=grid/mds-vo-name=local");
	httpurl = new Arc::URL("http://www.nordugrid.org/monitor.php?debug=2&sort=yes");
	fileurl = new Arc::URL("file:///home/grid/runtime/TEST-ATLAS-8.0.5");
	ldapurl2 = new Arc::URL("ldap://grid.uio.no/mds-vo-name=local, o=grid");
	opturl = new Arc::URL("gsiftp://hathi.hep.lu.se;ftpthreads=10;upload=yes/public/test.txt");
	ftpurl = new Arc::URL("ftp://user:secret@ftp.nordugrid.org/pub/files/guide.pdf");
	rlsurl = new Arc::URL("rls://rls.nordugrid.org/test.txt");
	rlsurl2 = new Arc::URL("rls://gsiftp://hagrid.it.uu.se/storage/test.txt|http://www.nordugrid.org/files/test.txt@rls.nordugrid.org/test.txt");
	rlsurl3 = new Arc::URL("rls://;exec=yes|gsiftp://hagrid.it.uu.se;threads=10/storage/test.txt|http://www.nordugrid.org;cache=no/files/test.txt@rls.nordugrid.org;local=yes/test.txt");
}


void URLTest::tearDown() {
	delete gsiftpurl;
	delete ldapurl;
	delete httpurl;
	delete fileurl;
	delete ldapurl2;
	delete opturl;
	delete ftpurl;
	delete rlsurl;
	delete rlsurl2;
	delete rlsurl3;
}


void URLTest::TestGsiftpUrl() {
	CPPUNIT_ASSERT_EQUAL( gsiftpurl->Protocol(), std::string("gsiftp") );
	CPPUNIT_ASSERT( gsiftpurl->Username().empty() );
	CPPUNIT_ASSERT( gsiftpurl->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( gsiftpurl->Host(), std::string("hathi.hep.lu.se") );
	CPPUNIT_ASSERT_EQUAL( gsiftpurl->Port(), 2811 );
	CPPUNIT_ASSERT_EQUAL( gsiftpurl->Path(), std::string("public/test.txt") );
	CPPUNIT_ASSERT( gsiftpurl->HTTPOptions().empty() );
	CPPUNIT_ASSERT( gsiftpurl->Options().empty() );
	CPPUNIT_ASSERT( gsiftpurl->Locations().empty() );
}


void URLTest::TestLdapUrl() {
	CPPUNIT_ASSERT_EQUAL( ldapurl->Protocol(), std::string("ldap") );
	CPPUNIT_ASSERT( ldapurl->Username().empty() );
	CPPUNIT_ASSERT( ldapurl->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( ldapurl->Host(), std::string("grid.uio.no") );
	CPPUNIT_ASSERT_EQUAL( ldapurl->Port(), 389 );
	CPPUNIT_ASSERT_EQUAL( ldapurl->Path(), std::string("mds-vo-name=local, o=grid") );
	CPPUNIT_ASSERT( ldapurl->HTTPOptions().empty() );
	CPPUNIT_ASSERT( ldapurl->Options().empty() );
	CPPUNIT_ASSERT( ldapurl->Locations().empty() );
}


void URLTest::TestHttpUrl() {
	CPPUNIT_ASSERT_EQUAL( httpurl->Protocol(), std::string("http") );
	CPPUNIT_ASSERT( httpurl->Username().empty() );
	CPPUNIT_ASSERT( httpurl->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( httpurl->Host(), std::string("www.nordugrid.org") );
	CPPUNIT_ASSERT_EQUAL( httpurl->Port(), 80 );
	CPPUNIT_ASSERT_EQUAL( httpurl->Path(), std::string("monitor.php") );

	std::map<std::string, std::string> httpmap = httpurl->HTTPOptions();
	CPPUNIT_ASSERT_EQUAL( (int)httpmap.size(), 2 );

	std::map<std::string, std::string>::iterator mapit = httpmap.begin();
	CPPUNIT_ASSERT_EQUAL( mapit->first, std::string("debug") );
	CPPUNIT_ASSERT_EQUAL( mapit->second, std::string("2") );

	mapit++;
	CPPUNIT_ASSERT_EQUAL( mapit->first, std::string("sort") );
	CPPUNIT_ASSERT_EQUAL( mapit->second, std::string("yes") );

	CPPUNIT_ASSERT( httpurl->Options().empty() );
	CPPUNIT_ASSERT( httpurl->Locations().empty() );
}


void URLTest::TestFileUrl() {
	CPPUNIT_ASSERT_EQUAL( fileurl->Protocol(), std::string("file") );
	CPPUNIT_ASSERT( fileurl->Username().empty() );
	CPPUNIT_ASSERT( fileurl->Passwd().empty() );
	CPPUNIT_ASSERT( fileurl->Host().empty() );
	CPPUNIT_ASSERT_EQUAL( fileurl->Port(), -1 );
	CPPUNIT_ASSERT_EQUAL( fileurl->Path(), std::string("/home/grid/runtime/TEST-ATLAS-8.0.5") );
	CPPUNIT_ASSERT( fileurl->HTTPOptions().empty() );
	CPPUNIT_ASSERT( fileurl->Options().empty() );
	CPPUNIT_ASSERT( fileurl->Locations().empty() );
}


void URLTest::TestLdapUrl2() {
	CPPUNIT_ASSERT_EQUAL( ldapurl2->Protocol(), std::string("ldap") );
	CPPUNIT_ASSERT( ldapurl2->Username().empty() );
	CPPUNIT_ASSERT( ldapurl2->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( ldapurl2->Host(), std::string("grid.uio.no") );
	CPPUNIT_ASSERT_EQUAL( ldapurl2->Port(), 389 );
	CPPUNIT_ASSERT_EQUAL( ldapurl2->Path(), std::string("mds-vo-name=local, o=grid") );
	CPPUNIT_ASSERT( ldapurl2->HTTPOptions().empty() );
	CPPUNIT_ASSERT( ldapurl2->Options().empty() );
	CPPUNIT_ASSERT( ldapurl2->Locations().empty() );
}


void URLTest::TestOptUrl() {
	CPPUNIT_ASSERT_EQUAL( opturl->Protocol(), std::string("gsiftp") );
	CPPUNIT_ASSERT( opturl->Username().empty() );
	CPPUNIT_ASSERT( opturl->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( opturl->Host(), std::string("hathi.hep.lu.se") );
	CPPUNIT_ASSERT_EQUAL( opturl->Port(), 2811 );
	CPPUNIT_ASSERT_EQUAL( opturl->Path(), std::string("public/test.txt") );
	CPPUNIT_ASSERT( opturl->HTTPOptions().empty() );
	CPPUNIT_ASSERT( opturl->Locations().empty() );

	std::map<std::string, std::string> options = opturl->Options();
	CPPUNIT_ASSERT_EQUAL( (int)options.size(), 2 );

	std::map<std::string, std::string>::iterator mapit = options.begin();
	CPPUNIT_ASSERT_EQUAL( mapit->first, std::string("ftpthreads") );
	CPPUNIT_ASSERT_EQUAL( mapit->second, std::string("10") );

	mapit++;
	CPPUNIT_ASSERT_EQUAL( mapit->first, std::string("upload") );
	CPPUNIT_ASSERT_EQUAL( mapit->second, std::string("yes") );
}


void URLTest::TestFtpUrl() {
	CPPUNIT_ASSERT_EQUAL( ftpurl->Protocol(), std::string("ftp") );
	CPPUNIT_ASSERT_EQUAL( ftpurl->Username(), std::string("user") );
	CPPUNIT_ASSERT_EQUAL( ftpurl->Passwd(), std::string("secret") );
	CPPUNIT_ASSERT_EQUAL( ftpurl->Host(), std::string("ftp.nordugrid.org") );
	CPPUNIT_ASSERT_EQUAL( ftpurl->Port(), 21 );
	CPPUNIT_ASSERT_EQUAL( ftpurl->Path(), std::string("pub/files/guide.pdf") );
	CPPUNIT_ASSERT( ftpurl->HTTPOptions().empty() );
	CPPUNIT_ASSERT( ftpurl->Options().empty() );
	CPPUNIT_ASSERT( ftpurl->Locations().empty() );
}


void URLTest::TestRlsUrl() {
	CPPUNIT_ASSERT_EQUAL( rlsurl->Protocol(), std::string("rls") );
	CPPUNIT_ASSERT( rlsurl->Username().empty() );
	CPPUNIT_ASSERT( rlsurl->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( rlsurl->Host(), std::string("rls.nordugrid.org") );
	CPPUNIT_ASSERT_EQUAL( rlsurl->Port(), 39281 );
	CPPUNIT_ASSERT_EQUAL( rlsurl->Path(), std::string("test.txt") );
	CPPUNIT_ASSERT( rlsurl->HTTPOptions().empty() );
	CPPUNIT_ASSERT( rlsurl->Options().empty() );
	CPPUNIT_ASSERT( rlsurl->Locations().empty() );
}


void URLTest::TestRlsUrl2() {
	CPPUNIT_ASSERT_EQUAL( rlsurl2->Protocol(), std::string("rls") );
	CPPUNIT_ASSERT( rlsurl2->Username().empty() );
	CPPUNIT_ASSERT( rlsurl2->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( rlsurl2->Host(), std::string("rls.nordugrid.org") );
	CPPUNIT_ASSERT_EQUAL( rlsurl2->Port(), 39281 );
	CPPUNIT_ASSERT_EQUAL( rlsurl2->Path(), std::string("test.txt") );
	CPPUNIT_ASSERT( rlsurl2->HTTPOptions().empty() );
	CPPUNIT_ASSERT( rlsurl2->Options().empty() );

	std::list<Arc::URLLocation> locations = rlsurl2->Locations();
	CPPUNIT_ASSERT_EQUAL( (int)locations.size(), 2 );

	std::list<Arc::URLLocation>::iterator urlit = locations.begin();
	CPPUNIT_ASSERT_EQUAL( urlit->str(), std::string("gsiftp://hagrid.it.uu.se:2811/storage/test.txt") );

	urlit++;
	CPPUNIT_ASSERT_EQUAL( urlit->str(), std::string("http://www.nordugrid.org:80/files/test.txt") );
}


void URLTest::TestRlsUrl3() {
	CPPUNIT_ASSERT_EQUAL( rlsurl3->Protocol(), std::string("rls") );
	CPPUNIT_ASSERT( rlsurl3->Username().empty() );
	CPPUNIT_ASSERT( rlsurl3->Passwd().empty() );
	CPPUNIT_ASSERT_EQUAL( rlsurl3->Host(), std::string("rls.nordugrid.org") );
	CPPUNIT_ASSERT_EQUAL( rlsurl3->Port(), 39281 );
	CPPUNIT_ASSERT_EQUAL( rlsurl3->Path(), std::string("test.txt") );
	CPPUNIT_ASSERT( rlsurl3->HTTPOptions().empty() );

	std::map<std::string, std::string> options = rlsurl3->Options();
	CPPUNIT_ASSERT_EQUAL( (int)options.size(), 1 );

	std::map<std::string, std::string>::iterator mapit = options.begin();
	CPPUNIT_ASSERT_EQUAL( mapit->first, std::string("local") );
	CPPUNIT_ASSERT_EQUAL( mapit->second, std::string("yes") );

	options = rlsurl3->CommonLocOptions();
	CPPUNIT_ASSERT_EQUAL( (int)options.size(), 1 );

	mapit = options.begin();
	CPPUNIT_ASSERT_EQUAL( mapit->first, std::string("exec") );
	CPPUNIT_ASSERT_EQUAL( mapit->second, std::string("yes") );

	std::list<Arc::URLLocation> locations = rlsurl3->Locations();
	CPPUNIT_ASSERT_EQUAL( (int)locations.size(), 2 );

	std::list<Arc::URLLocation>::iterator urlit = locations.begin();
	CPPUNIT_ASSERT_EQUAL( urlit->fullstr(), std::string("gsiftp://hagrid.it.uu.se:2811;threads=10/storage/test.txt") );

	urlit++;
	CPPUNIT_ASSERT_EQUAL( urlit->fullstr(), std::string("http://www.nordugrid.org:80;cache=no/files/test.txt") );
}


CPPUNIT_TEST_SUITE_REGISTRATION( URLTest );

