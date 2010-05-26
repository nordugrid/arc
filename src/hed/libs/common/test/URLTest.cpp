// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/URL.h>

class URLTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(URLTest);
  CPPUNIT_TEST(TestGsiftpUrl);
  CPPUNIT_TEST(TestLdapUrl);
  CPPUNIT_TEST(TestHttpUrl);
  CPPUNIT_TEST(TestFileUrl);
  CPPUNIT_TEST(TestLdapUrl2);
  CPPUNIT_TEST(TestOptUrl);
  CPPUNIT_TEST(TestFtpUrl);
  CPPUNIT_TEST(TestRlsUrl);
  CPPUNIT_TEST(TestRlsUrl2);
  CPPUNIT_TEST(TestRlsUrl3);
  CPPUNIT_TEST(TestLfcUrl);
  CPPUNIT_TEST(TestSrmUrl);
  CPPUNIT_TEST(TestIP6Url);
  CPPUNIT_TEST(TestIP6Url2);
  CPPUNIT_TEST(TestIP6Url3);
  CPPUNIT_TEST(TestBadUrl);
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
  void TestLfcUrl();
  void TestSrmUrl();
  void TestIP6Url();
  void TestIP6Url2();
  void TestIP6Url3();
  void TestBadUrl();

private:
  Arc::URL *gsiftpurl, *gsiftpurl2, *ldapurl, *httpurl, *fileurl, *ldapurl2, *opturl, *ftpurl, *rlsurl, *rlsurl2, *rlsurl3, *lfcurl, *srmurl, *ip6url, *ip6url2, *ip6url3;
};


void URLTest::setUp() {
  gsiftpurl = new Arc::URL("gsiftp://hathi.hep.lu.se/public/test.txt");
  gsiftpurl2 = new Arc::URL("gsiftp://hathi.hep.lu.se:2811/public:/test.txt:checksumtype=adler32");
  ldapurl = new Arc::URL("ldap://grid.uio.no/o=grid/mds-vo-name=local");
  httpurl = new Arc::URL("http://www.nordugrid.org/monitor.php?debug=2&sort=yes");
  fileurl = new Arc::URL("file:/home/grid/runtime/TEST-ATLAS-8.0.5");
  ldapurl2 = new Arc::URL("ldap://grid.uio.no/mds-vo-name=local, o=grid");
  opturl = new Arc::URL("gsiftp://hathi.hep.lu.se;threads=10;autodir=yes/public/test.txt");
  ftpurl = new Arc::URL("ftp://user:secret@ftp.nordugrid.org/pub/files/guide.pdf");
  rlsurl = new Arc::URL("rls://rls.nordugrid.org/test.txt");
  rlsurl2 = new Arc::URL("rls://gsiftp://hagrid.it.uu.se/storage/test.txt|http://www.nordugrid.org/files/test.txt@rls.nordugrid.org/test.txt");
  rlsurl3 = new Arc::URL("rls://;exec=yes|gsiftp://hagrid.it.uu.se;threads=10/storage/test.txt|http://www.nordugrid.org;cache=no/files/test.txt@rls.nordugrid.org;readonly=yes/test.txt");
  lfcurl = new Arc::URL("lfc://atlaslfc.nordugrid.org;cache=no/grid/atlas/file1:guid=7d36da04-430f-403c-adfb-540b27506cfa:checksumtype=ad:checksumvalue=12345678");
  srmurl = new Arc::URL("srm://srm.nordugrid.org/srm/managerv2?SFN=/data/public:/test.txt:checksumtype=adler32");
  ip6url = new Arc::URL("ftp://[ffff:eeee:dddd:cccc:aaaa:9999:8888:7777]/path");
  ip6url2 = new Arc::URL("ftp://[ffff:eeee:dddd:cccc:aaaa:9999:8888:7777]:2021/path");
  ip6url3 = new Arc::URL("ftp://[ffff:eeee:dddd:cccc:aaaa:9999:8888:7777];cache=no/path");
}


void URLTest::tearDown() {
  delete gsiftpurl;
  delete gsiftpurl2;
  delete ldapurl;
  delete httpurl;
  delete fileurl;
  delete ldapurl2;
  delete opturl;
  delete ftpurl;
  delete rlsurl;
  delete rlsurl2;
  delete rlsurl3;
  delete lfcurl;
  delete srmurl;
  delete ip6url;
  delete ip6url2;
  delete ip6url3;
}


void URLTest::TestGsiftpUrl() {
  CPPUNIT_ASSERT(*gsiftpurl);
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp"), gsiftpurl->Protocol());
  CPPUNIT_ASSERT(gsiftpurl->Username().empty());
  CPPUNIT_ASSERT(gsiftpurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("hathi.hep.lu.se"), gsiftpurl->Host());
  CPPUNIT_ASSERT_EQUAL(2811, gsiftpurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/public/test.txt"), gsiftpurl->Path());
  CPPUNIT_ASSERT(gsiftpurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(gsiftpurl->Options().empty());
  CPPUNIT_ASSERT(gsiftpurl->Locations().empty());
  
  CPPUNIT_ASSERT(*gsiftpurl2);
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp"), gsiftpurl2->Protocol());
  CPPUNIT_ASSERT(gsiftpurl2->Username().empty());
  CPPUNIT_ASSERT(gsiftpurl2->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("hathi.hep.lu.se"), gsiftpurl2->Host());
  CPPUNIT_ASSERT_EQUAL(2811, gsiftpurl2->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/public:/test.txt"), gsiftpurl2->Path());
  CPPUNIT_ASSERT(gsiftpurl2->HTTPOptions().empty());
  CPPUNIT_ASSERT(gsiftpurl2->Options().empty());
  CPPUNIT_ASSERT(gsiftpurl2->Locations().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("adler32"), gsiftpurl2->MetaDataOption("checksumtype"));
}


void URLTest::TestLdapUrl() {
  CPPUNIT_ASSERT(*ldapurl);
  CPPUNIT_ASSERT_EQUAL(std::string("ldap"), ldapurl->Protocol());
  CPPUNIT_ASSERT(ldapurl->Username().empty());
  CPPUNIT_ASSERT(ldapurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("grid.uio.no"), ldapurl->Host());
  CPPUNIT_ASSERT_EQUAL(389, ldapurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("mds-vo-name=local, o=grid"), ldapurl->Path());
  CPPUNIT_ASSERT(ldapurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(ldapurl->Options().empty());
  CPPUNIT_ASSERT(ldapurl->Locations().empty());
}


void URLTest::TestHttpUrl() {
  CPPUNIT_ASSERT(*httpurl);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), httpurl->Protocol());
  CPPUNIT_ASSERT(httpurl->Username().empty());
  CPPUNIT_ASSERT(httpurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("www.nordugrid.org"), httpurl->Host());
  CPPUNIT_ASSERT_EQUAL(80, httpurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/monitor.php"), httpurl->Path());

  std::map<std::string, std::string> httpmap = httpurl->HTTPOptions();
  CPPUNIT_ASSERT_EQUAL((int)httpmap.size(), 2);

  std::map<std::string, std::string>::iterator mapit = httpmap.begin();
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("debug"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("2"));

  mapit++;
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("sort"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("yes"));

  CPPUNIT_ASSERT(httpurl->Options().empty());
  CPPUNIT_ASSERT(httpurl->Locations().empty());
}


void URLTest::TestFileUrl() {
  CPPUNIT_ASSERT(*fileurl);
  CPPUNIT_ASSERT_EQUAL(std::string("file"), fileurl->Protocol());
  CPPUNIT_ASSERT(fileurl->Username().empty());
  CPPUNIT_ASSERT(fileurl->Passwd().empty());
  CPPUNIT_ASSERT(fileurl->Host().empty());
  CPPUNIT_ASSERT_EQUAL(-1, fileurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/home/grid/runtime/TEST-ATLAS-8.0.5"), fileurl->Path());
  CPPUNIT_ASSERT(fileurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(fileurl->Options().empty());
  CPPUNIT_ASSERT(fileurl->Locations().empty());
}


void URLTest::TestLdapUrl2() {
  CPPUNIT_ASSERT(*ldapurl);
  CPPUNIT_ASSERT_EQUAL(std::string("ldap"), ldapurl2->Protocol());
  CPPUNIT_ASSERT(ldapurl2->Username().empty());
  CPPUNIT_ASSERT(ldapurl2->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("grid.uio.no"), ldapurl2->Host());
  CPPUNIT_ASSERT_EQUAL(389, ldapurl2->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("mds-vo-name=local, o=grid"), ldapurl2->Path());
  CPPUNIT_ASSERT(ldapurl2->HTTPOptions().empty());
  CPPUNIT_ASSERT(ldapurl2->Options().empty());
  CPPUNIT_ASSERT(ldapurl2->Locations().empty());
}


void URLTest::TestOptUrl() {
  CPPUNIT_ASSERT(*opturl);
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp"), opturl->Protocol());
  CPPUNIT_ASSERT(opturl->Username().empty());
  CPPUNIT_ASSERT(opturl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("hathi.hep.lu.se"), opturl->Host());
  CPPUNIT_ASSERT_EQUAL(2811, opturl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/public/test.txt"), opturl->Path());
  CPPUNIT_ASSERT(opturl->HTTPOptions().empty());
  CPPUNIT_ASSERT(opturl->Locations().empty());

  std::map<std::string, std::string> options = opturl->Options();
  CPPUNIT_ASSERT_EQUAL(2, (int)options.size());

  std::map<std::string, std::string>::iterator mapit = options.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("autodir"), mapit->first);
  CPPUNIT_ASSERT_EQUAL(std::string("yes"), mapit->second);

  mapit++;
  CPPUNIT_ASSERT_EQUAL(std::string("threads"), mapit->first);
  CPPUNIT_ASSERT_EQUAL(std::string("10"), mapit->second);
}


void URLTest::TestFtpUrl() {
  CPPUNIT_ASSERT(*ftpurl);
  CPPUNIT_ASSERT_EQUAL(std::string("ftp"), ftpurl->Protocol());
  CPPUNIT_ASSERT_EQUAL(std::string("user"), ftpurl->Username());
  CPPUNIT_ASSERT_EQUAL(std::string("secret"), ftpurl->Passwd());
  CPPUNIT_ASSERT_EQUAL(std::string("ftp.nordugrid.org"), ftpurl->Host());
  CPPUNIT_ASSERT_EQUAL(21, ftpurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/pub/files/guide.pdf"), ftpurl->Path());
  CPPUNIT_ASSERT(ftpurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(ftpurl->Options().empty());
  CPPUNIT_ASSERT(ftpurl->Locations().empty());
}


void URLTest::TestRlsUrl() {
  CPPUNIT_ASSERT(*rlsurl);
  CPPUNIT_ASSERT_EQUAL(std::string("rls"), rlsurl->Protocol());
  CPPUNIT_ASSERT(rlsurl->Username().empty());
  CPPUNIT_ASSERT(rlsurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("rls.nordugrid.org"), rlsurl->Host());
  CPPUNIT_ASSERT_EQUAL(39281, rlsurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/test.txt"), rlsurl->Path());
  CPPUNIT_ASSERT(rlsurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(rlsurl->Options().empty());
  CPPUNIT_ASSERT(rlsurl->Locations().empty());
}


void URLTest::TestRlsUrl2() {
  CPPUNIT_ASSERT(*rlsurl2);
  CPPUNIT_ASSERT_EQUAL(std::string("rls"), rlsurl2->Protocol());
  CPPUNIT_ASSERT(rlsurl2->Username().empty());
  CPPUNIT_ASSERT(rlsurl2->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("rls.nordugrid.org"), rlsurl2->Host());
  CPPUNIT_ASSERT_EQUAL(39281, rlsurl2->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/test.txt"), rlsurl2->Path());
  CPPUNIT_ASSERT(rlsurl2->HTTPOptions().empty());
  CPPUNIT_ASSERT(rlsurl2->Options().empty());

  std::list<Arc::URLLocation> locations = rlsurl2->Locations();
  CPPUNIT_ASSERT_EQUAL(2, (int)locations.size());

  std::list<Arc::URLLocation>::iterator urlit = locations.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://hagrid.it.uu.se:2811/storage/test.txt"), urlit->str());

  urlit++;
  CPPUNIT_ASSERT_EQUAL(std::string("http://www.nordugrid.org:80/files/test.txt"), urlit->str());
}


void URLTest::TestRlsUrl3() {
  CPPUNIT_ASSERT(*rlsurl3);
  CPPUNIT_ASSERT_EQUAL(std::string("rls"), rlsurl3->Protocol());
  CPPUNIT_ASSERT(rlsurl3->Username().empty());
  CPPUNIT_ASSERT(rlsurl3->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("rls.nordugrid.org"), rlsurl3->Host());
  CPPUNIT_ASSERT_EQUAL(39281, rlsurl3->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/test.txt"), rlsurl3->Path());
  CPPUNIT_ASSERT(rlsurl3->HTTPOptions().empty());

  std::map<std::string, std::string> options = rlsurl3->Options();
  CPPUNIT_ASSERT_EQUAL((int)options.size(), 1);

  std::map<std::string, std::string>::iterator mapit = options.begin();
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("readonly"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("yes"));

  options = rlsurl3->CommonLocOptions();
  CPPUNIT_ASSERT_EQUAL((int)options.size(), 1);

  mapit = options.begin();
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("exec"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("yes"));

  std::list<Arc::URLLocation> locations = rlsurl3->Locations();
  CPPUNIT_ASSERT_EQUAL(2, (int)locations.size());

  std::list<Arc::URLLocation>::iterator urlit = locations.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://hagrid.it.uu.se:2811;threads=10/storage/test.txt"), urlit->fullstr());

  urlit++;
  CPPUNIT_ASSERT_EQUAL(std::string("http://www.nordugrid.org:80;cache=no/files/test.txt"), urlit->fullstr());
}

void URLTest::TestLfcUrl() {
  CPPUNIT_ASSERT(*lfcurl);
  CPPUNIT_ASSERT_EQUAL(std::string("lfc"), lfcurl->Protocol());
  CPPUNIT_ASSERT(lfcurl->Username().empty());
  CPPUNIT_ASSERT(lfcurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("atlaslfc.nordugrid.org"), lfcurl->Host());
  CPPUNIT_ASSERT_EQUAL(5010, lfcurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/grid/atlas/file1"), lfcurl->Path());
  CPPUNIT_ASSERT(lfcurl->HTTPOptions().empty());

  std::map<std::string, std::string> options = lfcurl->Options();
  CPPUNIT_ASSERT_EQUAL(1, (int)options.size());

  std::map<std::string, std::string>::iterator mapit = options.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("cache"), mapit->first);
  CPPUNIT_ASSERT_EQUAL(std::string("no"), mapit->second);

  CPPUNIT_ASSERT(lfcurl->CommonLocOptions().empty());

  CPPUNIT_ASSERT(lfcurl->Locations().empty());

  CPPUNIT_ASSERT_EQUAL(3, (int)lfcurl->MetaDataOptions().size());
  CPPUNIT_ASSERT_EQUAL(std::string("7d36da04-430f-403c-adfb-540b27506cfa"), lfcurl->MetaDataOption("guid"));
  CPPUNIT_ASSERT_EQUAL(std::string("ad"), lfcurl->MetaDataOption("checksumtype"));
  CPPUNIT_ASSERT_EQUAL(std::string("12345678"), lfcurl->MetaDataOption("checksumvalue"));
  CPPUNIT_ASSERT_EQUAL(std::string("lfc://atlaslfc.nordugrid.org:5010;cache=no/grid/atlas/file1:checksumtype=ad:checksumvalue=12345678:guid=7d36da04-430f-403c-adfb-540b27506cfa"), lfcurl->fullstr());
  lfcurl->AddMetaDataOption("checksumvalue", "87654321", true);
  CPPUNIT_ASSERT_EQUAL(std::string("87654321"), lfcurl->MetaDataOption("checksumvalue"));
  CPPUNIT_ASSERT_EQUAL(std::string("lfc://atlaslfc.nordugrid.org:5010;cache=no/grid/atlas/file1:checksumtype=ad:checksumvalue=87654321:guid=7d36da04-430f-403c-adfb-540b27506cfa"), lfcurl->fullstr());
}

void URLTest::TestSrmUrl() {
  CPPUNIT_ASSERT(*srmurl);
  CPPUNIT_ASSERT_EQUAL(std::string("srm"), srmurl->Protocol());
  CPPUNIT_ASSERT(srmurl->Username().empty());
  CPPUNIT_ASSERT(srmurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("srm.nordugrid.org"), srmurl->Host());
  CPPUNIT_ASSERT_EQUAL(8443, srmurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/srm/managerv2"), srmurl->Path());
  CPPUNIT_ASSERT_EQUAL(std::string("/data/public:/test.txt"), srmurl->HTTPOption("SFN"));
  CPPUNIT_ASSERT(srmurl->Options().empty());
  CPPUNIT_ASSERT(srmurl->Locations().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("adler32"), srmurl->MetaDataOption("checksumtype"));
}


void URLTest::TestIP6Url() {
  CPPUNIT_ASSERT(*ip6url);
  CPPUNIT_ASSERT_EQUAL(std::string("ftp"), ip6url->Protocol());
  CPPUNIT_ASSERT_EQUAL(std::string("ffff:eeee:dddd:cccc:aaaa:9999:8888:7777"), ip6url->Host());
  CPPUNIT_ASSERT_EQUAL(21, ip6url->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/path"), ip6url->Path());
  CPPUNIT_ASSERT(ip6url->Options().empty());
}


void URLTest::TestIP6Url2() {
  CPPUNIT_ASSERT(*ip6url2);
  CPPUNIT_ASSERT_EQUAL(std::string("ftp"), ip6url2->Protocol());
  CPPUNIT_ASSERT_EQUAL(std::string("ffff:eeee:dddd:cccc:aaaa:9999:8888:7777"), ip6url2->Host());
  CPPUNIT_ASSERT_EQUAL(2021, ip6url2->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/path"), ip6url2->Path());
  CPPUNIT_ASSERT(ip6url2->Options().empty());
}


void URLTest::TestIP6Url3() {
  CPPUNIT_ASSERT(*ip6url3);
  CPPUNIT_ASSERT_EQUAL(std::string("ftp"), ip6url3->Protocol());
  CPPUNIT_ASSERT_EQUAL(std::string("ffff:eeee:dddd:cccc:aaaa:9999:8888:7777"), ip6url3->Host());
  CPPUNIT_ASSERT_EQUAL(21, ip6url3->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/path"), ip6url3->Path());
  CPPUNIT_ASSERT_EQUAL(1, (int)(ip6url3->Options().size()));
  std::map<std::string,std::string> options = ip6url3->Options();
  CPPUNIT_ASSERT_EQUAL(std::string("no"), options["cache"]);
  CPPUNIT_ASSERT_EQUAL(std::string("ftp://[ffff:eeee:dddd:cccc:aaaa:9999:8888:7777]:21;cache=no/path"), ip6url3->fullstr());
}


void URLTest::TestBadUrl() {
  Arc::URL *url = new Arc::URL("");
  CPPUNIT_ASSERT(!(*url));
  url = new Arc::URL("#url");
  CPPUNIT_ASSERT(!(*url));
  url = new Arc::URL("arc:file1");
  CPPUNIT_ASSERT(!(*url));
  url = new Arc::URL("http:/file1");
  CPPUNIT_ASSERT(!(*url));
  delete url;    
}

CPPUNIT_TEST_SUITE_REGISTRATION(URLTest);
