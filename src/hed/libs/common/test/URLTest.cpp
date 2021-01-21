// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/URL.h>

class URLTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(URLTest);
  CPPUNIT_TEST(TestGsiftpUrl);
  CPPUNIT_TEST(TestLdapUrl);
  CPPUNIT_TEST(TestHttpUrl);
  CPPUNIT_TEST(TestDavsUrl);
  CPPUNIT_TEST(TestFileUrl);
  CPPUNIT_TEST(TestLdapUrl2);
  CPPUNIT_TEST(TestOptUrl);
  CPPUNIT_TEST(TestFtpUrl);
  CPPUNIT_TEST(TestSrmUrl);
  CPPUNIT_TEST(TestRootUrl);
  CPPUNIT_TEST(TestIP6Url);
  CPPUNIT_TEST(TestIP6Url2);
  CPPUNIT_TEST(TestIP6Url3);
  CPPUNIT_TEST(TestBadUrl);
  CPPUNIT_TEST(TestWithDefaults);
  CPPUNIT_TEST(TestStringMatchesURL);
  CPPUNIT_TEST(TestOptions);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestGsiftpUrl();
  void TestLdapUrl();
  void TestHttpUrl();
  void TestDavsUrl();
  void TestFileUrl();
  void TestLdapUrl2();
  void TestOptUrl();
  void TestFtpUrl();
  void TestSrmUrl();
  void TestRootUrl();
  void TestIP6Url();
  void TestIP6Url2();
  void TestIP6Url3();
  void TestBadUrl();
  void TestWithDefaults();
  void TestStringMatchesURL();
  void TestOptions();

private:
  Arc::URL *gsiftpurl, *gsiftpurl2, *ldapurl, *httpurl, *davsurl, *fileurl, *ldapurl2, *opturl, *ftpurl, *srmurl, *rooturl, *ip6url, *ip6url2, *ip6url3;
};


void URLTest::setUp() {
  gsiftpurl = new Arc::URL("gsiftp://hathi.hep.lu.se/public/test.txt");
  gsiftpurl2 = new Arc::URL("gsiftp://hathi.hep.lu.se:2811/public:/test.txt:checksumtype=adler32");
  ldapurl = new Arc::URL("ldap://grid.uio.no/o=grid/mds-vo-name=local");
  httpurl = new Arc::URL("http://www.nordugrid.org/monitor:v1.php?debug=2&newpath=/path/to/file&sort=yes&symbols=() *!%\":guid=abcd");
  davsurl = new Arc::URL("davs://dav.ndgf.org:443/test/file1:checksumtype=adler32:checksumvalue=3d1ae86e");
  fileurl = new Arc::URL("file:/home/grid/runtime/TEST-ATLAS-8.0.5");
  ldapurl2 = new Arc::URL("ldap://grid.uio.no/mds-vo-name=local, o=grid");
  opturl = new Arc::URL("gsiftp://hathi.hep.lu.se;threads=10;autodir=yes/public/test.txt");
  ftpurl = new Arc::URL("ftp://user:secret@ftp.nordugrid.org/pub/files/guide.pdf");
  srmurl = new Arc::URL("srm://srm.nordugrid.org/srm/managerv2?SFN=/data/public:/test.txt:checksumtype=adler32");
  rooturl = new Arc::URL("root://xrootd.org:1094/mydata:disk/files/data.001.zip.1?xrdcl.unzip=file.1");
  ip6url = new Arc::URL("ftp://[ffff:eeee:dddd:cccc:aaaa:9999:8888:7777]/path");
  ip6url2 = new Arc::URL("ftp://[ffff:eeee:dddd:cccc:aaaa:9999:8888:7777]:2021/path");
  ip6url3 = new Arc::URL("ftp://[ffff:eeee:dddd:cccc:aaaa:9999:8888:7777];cache=no/path");
}


void URLTest::tearDown() {
  delete gsiftpurl;
  delete gsiftpurl2;
  delete ldapurl;
  delete httpurl;
  delete davsurl;
  delete fileurl;
  delete ldapurl2;
  delete opturl;
  delete ftpurl;
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
  CPPUNIT_ASSERT_EQUAL(std::string("/monitor:v1.php"), httpurl->Path());

  CPPUNIT_ASSERT_EQUAL(std::string("/monitor:v1.php?debug=2&newpath=/path/to/file&sort=yes&symbols=() *!%\""), httpurl->FullPath());
  CPPUNIT_ASSERT_EQUAL(std::string("/monitor%3Av1.php?debug=2&newpath=%2Fpath%2Fto%2Ffile&sort=yes&symbols=%28%29%20%2A%21%25%22"), httpurl->FullPathURIEncoded());

  std::map<std::string, std::string> httpmap = httpurl->HTTPOptions();
  CPPUNIT_ASSERT_EQUAL((int)httpmap.size(), 4);

  std::map<std::string, std::string>::iterator mapit = httpmap.begin();
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("debug"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("2"));

  mapit++;
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("newpath"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("/path/to/file"));

  mapit++;
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("sort"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("yes"));

  mapit++;
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("symbols"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("() *!%\""));

  CPPUNIT_ASSERT_EQUAL(std::string("abcd"), httpurl->MetaDataOption("guid"));
  CPPUNIT_ASSERT(httpurl->Options().empty());
  CPPUNIT_ASSERT(httpurl->Locations().empty());
}


void URLTest::TestDavsUrl() {
  CPPUNIT_ASSERT(*davsurl);
  CPPUNIT_ASSERT_EQUAL(std::string("davs"), davsurl->Protocol());
  CPPUNIT_ASSERT(davsurl->Username().empty());
  CPPUNIT_ASSERT(davsurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("dav.ndgf.org"), davsurl->Host());
  CPPUNIT_ASSERT_EQUAL(443, davsurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/test/file1"), davsurl->FullPathURIEncoded());
  CPPUNIT_ASSERT(davsurl->Options().empty());
  CPPUNIT_ASSERT(davsurl->Locations().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("adler32"), davsurl->MetaDataOption("checksumtype"));
  CPPUNIT_ASSERT_EQUAL(std::string("3d1ae86e"), davsurl->MetaDataOption("checksumvalue"));
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


void URLTest::TestSrmUrl() {
  CPPUNIT_ASSERT(*srmurl);
  CPPUNIT_ASSERT_EQUAL(std::string("srm"), srmurl->Protocol());
  CPPUNIT_ASSERT(srmurl->Username().empty());
  CPPUNIT_ASSERT(srmurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("srm.nordugrid.org"), srmurl->Host());
  // no default port is defined for SRM
  CPPUNIT_ASSERT_EQUAL(-1, srmurl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/srm/managerv2"), srmurl->Path());
  CPPUNIT_ASSERT_EQUAL(std::string("/data/public:/test.txt"), srmurl->HTTPOption("SFN"));
  CPPUNIT_ASSERT(srmurl->Options().empty());
  CPPUNIT_ASSERT(srmurl->Locations().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("adler32"), srmurl->MetaDataOption("checksumtype"));
}


void URLTest::TestRootUrl() {
  CPPUNIT_ASSERT(*rooturl);
  CPPUNIT_ASSERT_EQUAL(std::string("root"), rooturl->Protocol());
  CPPUNIT_ASSERT(rooturl->Username().empty());
  CPPUNIT_ASSERT(rooturl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(std::string("xrootd.org"), rooturl->Host());
  CPPUNIT_ASSERT_EQUAL(1094, rooturl->Port());
  CPPUNIT_ASSERT_EQUAL(std::string("/mydata:disk/files/data.001.zip.1"), rooturl->Path());
  CPPUNIT_ASSERT_EQUAL(std::string("file.1"), rooturl->HTTPOption("xrdcl.unzip"));
  CPPUNIT_ASSERT(rooturl->Options().empty());
  CPPUNIT_ASSERT(rooturl->Locations().empty());

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

void URLTest::TestWithDefaults() {
  Arc::URL url("http://example.org", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(123, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/test", url.Path());
  
  url = Arc::URL("http://example.org:321", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(321, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/test", url.Path());
  
  url = Arc::URL("http://example.org/testing", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(123, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/testing", url.Path());
  
  url = Arc::URL("http://example.org:321/testing", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(321, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/testing", url.Path());

  url = Arc::URL("http://[::1]", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(123, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/test", url.Path());
  
  url = Arc::URL("http://[::1]:321", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(321, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/test", url.Path());
  
  url = Arc::URL("http://[::1]/testing", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(123, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/testing", url.Path());
  
  url = Arc::URL("http://[::1]:321/testing", false, 123, "/test");
  CPPUNIT_ASSERT(url);
  CPPUNIT_ASSERT_EQUAL(321, url.Port());
  CPPUNIT_ASSERT_EQUAL((std::string)"/testing", url.Path());
}

void URLTest::TestStringMatchesURL() {
  std::string str;
  Arc::URL url;

  str = "example.org";
  url = (std::string)"http://example.org:8080/path";

  CPPUNIT_ASSERT(url.StringMatches(str));

  str += ":8080";
  CPPUNIT_ASSERT(url.StringMatches(str));

  str = "http://" + str;
  CPPUNIT_ASSERT(url.StringMatches(str));

  str += "/path";
  CPPUNIT_ASSERT(url.StringMatches(str));

  str = "example.org/";
  CPPUNIT_ASSERT(url.StringMatches(str));
}

void URLTest::TestOptions() {
  Arc::URL url("http://example.org:8080/path");
  CPPUNIT_ASSERT(url);

  CPPUNIT_ASSERT(!url.AddOption(std::string("attr1")));
  CPPUNIT_ASSERT(!url.AddOption(std::string(""), std::string("")));
  CPPUNIT_ASSERT_EQUAL(std::string(""), (url.Option("attr1")));

  CPPUNIT_ASSERT(url.AddOption(std::string("attr1"), std::string("value1")));
  CPPUNIT_ASSERT_EQUAL(std::string("value1"), url.Option("attr1"));

  CPPUNIT_ASSERT(!url.AddOption("attr1", "value2", false));
  CPPUNIT_ASSERT_EQUAL(std::string("value1"), url.Option("attr1"));
  CPPUNIT_ASSERT(url.AddOption("attr1", "value2", true));
  CPPUNIT_ASSERT_EQUAL(std::string("value2"), url.Option("attr1"));

  CPPUNIT_ASSERT(!url.AddOption("attr1=value1", false));
  CPPUNIT_ASSERT_EQUAL(std::string("value2"), url.Option("attr1"));
  CPPUNIT_ASSERT(url.AddOption("attr1=value1", true));
  CPPUNIT_ASSERT_EQUAL(std::string("value1"), url.Option("attr1"));
}

CPPUNIT_TEST_SUITE_REGISTRATION(URLTest);
