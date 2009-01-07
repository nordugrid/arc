#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/infosys/InfoFilter.h>


class InfoFilterTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(InfoFilterTest);
  CPPUNIT_TEST(TestInfoFilter);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestInfoFilter();
};


void InfoFilterTest::setUp() {
}


void InfoFilterTest::tearDown() {
}


class TestSecAttr: public Arc::SecAttr {
 public:
  TestSecAttr(const char* id):id_(id) { };
  virtual ~TestSecAttr(void) { };
  virtual operator bool(void) const { return true; };
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
 protected:
  virtual bool equal(const SecAttr &b) const;
 private:
  std::string id_;
};


static void add_subject_attribute(XMLNode item,const std::string& subject,const
char* id) {
   XMLNode attr = item.NewChild("ra:SubjectAttribute");
   attr=subject; attr.NewAttribute("Type")="string";
   attr.NewAttribute("AttributeId")=id;
}

bool TestSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  if(format != ARCAuth) return false;
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  val.Namespaces(ns); val.Name("ra:Request");
  XMLNode item = val.NewChild("ra:RequestItem");
  XMLNode subj = item.NewChild("ra:Subject");
  XMLNode attr = subj.NewChild("ra:SubjectAttribute");
  attr=id_;
  attr.NewAttribute("Type")="string";
  attr.NewAttribute("AttributeId")="urn:testID";
  return true;
}


void InfoFilterTest::TestInfoFilter() {

  // Service description document
  Arc::XMLNode infodoc("\
<?xml version=\"1.0\"?>\n\
<InfoDoc xmlns=\"urn:info\">\n\
  <Resource>\n\
    <Memory>A lot</Memory>\n\
    <Performance>Turltle-like</Performance>\n\
  </Resource>\n\
  <Owner>\n\
      <Name>Unknown</Name>\n\
  </Owner>\n\
  <Resource>\n\
      <Memory>640kb enough for everyone</Memory>\n\
      <Performance>Quantum computer</Performance>\n\
  </Resource>\n\
</InfoDoc>\n");

  // Service description with policies
  

  // Requestor's identifier
  MessageAuth user_id;
  TestSecAttr user_attr("USER1");
  TLSSecAttr* sattr = new TLSSecAttr(*tstream, config_);
  user_id.set("TEST",&user_attr);

  // Filter
  InfoFilter filter(user_id);

  // External policies
  Arc::NS ns;
  XMLNode policy1();
  std::list< std::pair<std::string,XMLNode> policies;

  // Service description for filtering
  XMLNode infodoc_filtered;
  infodoc.New(infodoc_filtered);



  bool filter.Filter(infodoc_filtered);
  /// Filter information document according to internal and external policies
  /** In provided document all policies and nodes which have their policies
     evaluated to negative result are removed. */
  bool Filter(XMLNode doc,const std::list< std::pair<std::string,XMLNode> >& policies,const Arc::NS& ns);


  // Service description document
  Arc::XMLNode infodoc("\
<?xml version=\"1.0\"?>\n\
<InfoDoc xmlns=\"urn:info\">\n\
  <Resource>\n\
    <Memory>A lot</Memory>\n\
    <Performance>Turltle-like</Performance>\n\
  </Resource>\n\
  <Owner>\n\
      <Name>Unknown</Name>\n\
  </Owner>\n\
  <Resource>\n\
      <Memory>640kb enough for everyone</Memory>\n\
      <Performance>Quantum computer</Performance>\n\
  </Resource>\n\
</InfoDoc>\n");

  // Creating service container
  Arc::InformationContainer container(infodoc);
  //std::cout<<"Document:\n"<<doc<<std::endl;

  // Creating client request
  std::list<std::string> name;
  name.push_back("Resource");
  //std::cout<<"Request for elements: "<<*(name.begin())<<std::endl;
  Arc::InformationRequest request(name);
  CPPUNIT_ASSERT((bool)request);

  // Processing request at server side
  Arc::SOAPEnvelope* response = container.Process(*(request.SOAP()));
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT((bool)*response);

  // Extracting result at client
  Arc::InformationResponse res(*response);
  CPPUNIT_ASSERT((bool)res);

  std::list<Arc::XMLNode> results = res.Result();
  CPPUNIT_ASSERT_EQUAL((int)results.size(), int(2));
  std::list<Arc::XMLNode>::iterator r = results.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)(*r)["Memory"], std::string("A lot"));
  ++r;
  CPPUNIT_ASSERT_EQUAL((std::string)(*r)["Performance"], std::string("Quantum computer"));
}



/*
void URLTest::TestGsiftpUrl() {
  CPPUNIT_ASSERT_EQUAL(gsiftpurl->Protocol(), std::string("gsiftp"));
  CPPUNIT_ASSERT(gsiftpurl->Username().empty());
  CPPUNIT_ASSERT(gsiftpurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(gsiftpurl->Host(), std::string("hathi.hep.lu.se"));
  CPPUNIT_ASSERT_EQUAL(gsiftpurl->Port(), 2811);
  CPPUNIT_ASSERT_EQUAL(gsiftpurl->Path(), std::string("public/test.txt"));
  CPPUNIT_ASSERT(gsiftpurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(gsiftpurl->Options().empty());
  CPPUNIT_ASSERT(gsiftpurl->Locations().empty());
}


void URLTest::TestLdapUrl() {
  CPPUNIT_ASSERT_EQUAL(ldapurl->Protocol(), std::string("ldap"));
  CPPUNIT_ASSERT(ldapurl->Username().empty());
  CPPUNIT_ASSERT(ldapurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(ldapurl->Host(), std::string("grid.uio.no"));
  CPPUNIT_ASSERT_EQUAL(ldapurl->Port(), 389);
  CPPUNIT_ASSERT_EQUAL(ldapurl->Path(), std::string("mds-vo-name=local, o=grid"));
  CPPUNIT_ASSERT(ldapurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(ldapurl->Options().empty());
  CPPUNIT_ASSERT(ldapurl->Locations().empty());
}


void URLTest::TestHttpUrl() {
  CPPUNIT_ASSERT_EQUAL(httpurl->Protocol(), std::string("http"));
  CPPUNIT_ASSERT(httpurl->Username().empty());
  CPPUNIT_ASSERT(httpurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(httpurl->Host(), std::string("www.nordugrid.org"));
  CPPUNIT_ASSERT_EQUAL(httpurl->Port(), 80);
  CPPUNIT_ASSERT_EQUAL(httpurl->Path(), std::string("monitor.php"));

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
  CPPUNIT_ASSERT_EQUAL(fileurl->Protocol(), std::string("file"));
  CPPUNIT_ASSERT(fileurl->Username().empty());
  CPPUNIT_ASSERT(fileurl->Passwd().empty());
  CPPUNIT_ASSERT(fileurl->Host().empty());
  CPPUNIT_ASSERT_EQUAL(fileurl->Port(), -1);
  CPPUNIT_ASSERT_EQUAL(fileurl->Path(), std::string("/home/grid/runtime/TEST-ATLAS-8.0.5"));
  CPPUNIT_ASSERT(fileurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(fileurl->Options().empty());
  CPPUNIT_ASSERT(fileurl->Locations().empty());
}


void URLTest::TestLdapUrl2() {
  CPPUNIT_ASSERT_EQUAL(ldapurl2->Protocol(), std::string("ldap"));
  CPPUNIT_ASSERT(ldapurl2->Username().empty());
  CPPUNIT_ASSERT(ldapurl2->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(ldapurl2->Host(), std::string("grid.uio.no"));
  CPPUNIT_ASSERT_EQUAL(ldapurl2->Port(), 389);
  CPPUNIT_ASSERT_EQUAL(ldapurl2->Path(), std::string("mds-vo-name=local, o=grid"));
  CPPUNIT_ASSERT(ldapurl2->HTTPOptions().empty());
  CPPUNIT_ASSERT(ldapurl2->Options().empty());
  CPPUNIT_ASSERT(ldapurl2->Locations().empty());
}


void URLTest::TestOptUrl() {
  CPPUNIT_ASSERT_EQUAL(opturl->Protocol(), std::string("gsiftp"));
  CPPUNIT_ASSERT(opturl->Username().empty());
  CPPUNIT_ASSERT(opturl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(opturl->Host(), std::string("hathi.hep.lu.se"));
  CPPUNIT_ASSERT_EQUAL(opturl->Port(), 2811);
  CPPUNIT_ASSERT_EQUAL(opturl->Path(), std::string("public/test.txt"));
  CPPUNIT_ASSERT(opturl->HTTPOptions().empty());
  CPPUNIT_ASSERT(opturl->Locations().empty());

  std::map<std::string, std::string> options = opturl->Options();
  CPPUNIT_ASSERT_EQUAL((int)options.size(), 2);

  std::map<std::string, std::string>::iterator mapit = options.begin();
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("ftpthreads"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("10"));

  mapit++;
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("upload"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("yes"));
}


void URLTest::TestFtpUrl() {
  CPPUNIT_ASSERT_EQUAL(ftpurl->Protocol(), std::string("ftp"));
  CPPUNIT_ASSERT_EQUAL(ftpurl->Username(), std::string("user"));
  CPPUNIT_ASSERT_EQUAL(ftpurl->Passwd(), std::string("secret"));
  CPPUNIT_ASSERT_EQUAL(ftpurl->Host(), std::string("ftp.nordugrid.org"));
  CPPUNIT_ASSERT_EQUAL(ftpurl->Port(), 21);
  CPPUNIT_ASSERT_EQUAL(ftpurl->Path(), std::string("pub/files/guide.pdf"));
  CPPUNIT_ASSERT(ftpurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(ftpurl->Options().empty());
  CPPUNIT_ASSERT(ftpurl->Locations().empty());
}


void URLTest::TestRlsUrl() {
  CPPUNIT_ASSERT_EQUAL(rlsurl->Protocol(), std::string("rls"));
  CPPUNIT_ASSERT(rlsurl->Username().empty());
  CPPUNIT_ASSERT(rlsurl->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(rlsurl->Host(), std::string("rls.nordugrid.org"));
  CPPUNIT_ASSERT_EQUAL(rlsurl->Port(), 39281);
  CPPUNIT_ASSERT_EQUAL(rlsurl->Path(), std::string("test.txt"));
  CPPUNIT_ASSERT(rlsurl->HTTPOptions().empty());
  CPPUNIT_ASSERT(rlsurl->Options().empty());
  CPPUNIT_ASSERT(rlsurl->Locations().empty());
}


void URLTest::TestRlsUrl2() {
  CPPUNIT_ASSERT_EQUAL(rlsurl2->Protocol(), std::string("rls"));
  CPPUNIT_ASSERT(rlsurl2->Username().empty());
  CPPUNIT_ASSERT(rlsurl2->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(rlsurl2->Host(), std::string("rls.nordugrid.org"));
  CPPUNIT_ASSERT_EQUAL(rlsurl2->Port(), 39281);
  CPPUNIT_ASSERT_EQUAL(rlsurl2->Path(), std::string("test.txt"));
  CPPUNIT_ASSERT(rlsurl2->HTTPOptions().empty());
  CPPUNIT_ASSERT(rlsurl2->Options().empty());

  std::list<Arc::URLLocation> locations = rlsurl2->Locations();
  CPPUNIT_ASSERT_EQUAL((int)locations.size(), 2);

  std::list<Arc::URLLocation>::iterator urlit = locations.begin();
  CPPUNIT_ASSERT_EQUAL(urlit->str(), std::string("gsiftp://hagrid.it.uu.se:2811/storage/test.txt"));

  urlit++;
  CPPUNIT_ASSERT_EQUAL(urlit->str(), std::string("http://www.nordugrid.org:80/files/test.txt"));
}


void URLTest::TestRlsUrl3() {
  CPPUNIT_ASSERT_EQUAL(rlsurl3->Protocol(), std::string("rls"));
  CPPUNIT_ASSERT(rlsurl3->Username().empty());
  CPPUNIT_ASSERT(rlsurl3->Passwd().empty());
  CPPUNIT_ASSERT_EQUAL(rlsurl3->Host(), std::string("rls.nordugrid.org"));
  CPPUNIT_ASSERT_EQUAL(rlsurl3->Port(), 39281);
  CPPUNIT_ASSERT_EQUAL(rlsurl3->Path(), std::string("test.txt"));
  CPPUNIT_ASSERT(rlsurl3->HTTPOptions().empty());

  std::map<std::string, std::string> options = rlsurl3->Options();
  CPPUNIT_ASSERT_EQUAL((int)options.size(), 1);

  std::map<std::string, std::string>::iterator mapit = options.begin();
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("local"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("yes"));

  options = rlsurl3->CommonLocOptions();
  CPPUNIT_ASSERT_EQUAL((int)options.size(), 1);

  mapit = options.begin();
  CPPUNIT_ASSERT_EQUAL(mapit->first, std::string("exec"));
  CPPUNIT_ASSERT_EQUAL(mapit->second, std::string("yes"));

  std::list<Arc::URLLocation> locations = rlsurl3->Locations();
  CPPUNIT_ASSERT_EQUAL((int)locations.size(), 2);

  std::list<Arc::URLLocation>::iterator urlit = locations.begin();
  CPPUNIT_ASSERT_EQUAL(urlit->fullstr(), std::string("gsiftp://hagrid.it.uu.se:2811;threads=10/storage/test.txt"));

  urlit++;
  CPPUNIT_ASSERT_EQUAL(urlit->fullstr(), std::string("http://www.nordugrid.org:80;cache=no/files/test.txt"));
}
*/


CPPUNIT_TEST_SUITE_REGISTRATION(InformationInterfaceTest);

