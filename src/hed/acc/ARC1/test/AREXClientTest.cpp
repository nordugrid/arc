/*
  This tests are good for the ArexClient's 19411 revision.
*/

#include <cppunit/extensions/HelperMacros.h>

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>

#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>

#include "../JobStateARC1.h"
#include "../AREXClient.h"
#include "../../../libs/client/test/SimulatorClasses.h"

#define BES_FACTORY_ACTIONS_BASE_URL "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/"

Arc::MCCConfig config;

class AREXClientTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(AREXClientTest);
  CPPUNIT_TEST(ProcessTest);
  CPPUNIT_TEST(SubmitTest);
  CPPUNIT_TEST(SubmitTestwithDelegation);
  CPPUNIT_TEST(StatTest);
  CPPUNIT_TEST(ServiceStatTest);
  CPPUNIT_TEST(ListServicesFromISISTest);
  CPPUNIT_TEST(KillTest);
  CPPUNIT_TEST(CleanTest);
  CPPUNIT_TEST(GetdescTest);
  CPPUNIT_TEST(MigrateTest);
  CPPUNIT_TEST(ResumeTest);
  CPPUNIT_TEST(CreateActivityIdentifierTest);
  CPPUNIT_TEST_SUITE_END();

public:
  AREXClientTest() : ac(Arc::URL("test://AREXClientTest.now"), config, -1, true) { config.AddProxy("Proxy value");}

  void setUp() {}
  void tearDown() {}

  void ProcessTest();
  void SubmitTest();
  void SubmitTestwithDelegation();
  void StatTest();
  void ServiceStatTest();
  void ListServicesFromISISTest();
  void KillTest();
  void CleanTest();
  void GetdescTest();
  void MigrateTest();
  void ResumeTest();
  void CreateActivityIdentifierTest();

private:
  Arc::AREXClient ac;
};

void AREXClientTest::ProcessTest()
{
  const std::string value = "Test response value";
  const std::string node = "Response";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(node) = value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running Check
  Arc::PayloadSOAP req(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  req.NewChild("TestNode") = "Test request value";
  bool delegate(false);
  Arc::XMLNode resp;

  CPPUNIT_ASSERT(ac.process(req,delegate,resp));

  //Response Check
  CPPUNIT_ASSERT(resp);
  CPPUNIT_ASSERT_EQUAL(node, resp.Name());
  CPPUNIT_ASSERT_EQUAL(value, (std::string)resp);
}

void AREXClientTest::SubmitTest()
{
  const std::string jobdesc = "";
  std::string jobid = "";

  const std::string value = "Test value";
  const std::string query = "CreateActivityResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(query).NewChild("ActivityIdentifier")  = value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running check
  CPPUNIT_ASSERT(ac.submit(jobdesc, jobid, false));

  //Response Check
  CPPUNIT_ASSERT_EQUAL((std::string)"<?xml version=\"1.0\"?>\n<ActivityIdentifier>Test value</ActivityIdentifier>\n", jobid);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
  CPPUNIT_ASSERT_EQUAL(jobdesc, (std::string)Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
}

void AREXClientTest::SubmitTestwithDelegation()
{
  const std::string jobdesc = "";
  std::string jobid = "";
  
  const std::string value = "Test value";
  const std::string query = "CreateActivityResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(query).NewChild("ActivityIdentifier")  = value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running Check
  CPPUNIT_ASSERT(ac.submit(jobdesc, jobid, true));

  //Response Check
  CPPUNIT_ASSERT_EQUAL((std::string)"<?xml version=\"1.0\"?>\n<ActivityIdentifier>Test value</ActivityIdentifier>\n", jobid);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
  CPPUNIT_ASSERT_EQUAL(jobdesc, (std::string)Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);

  //Delegation part
  const std::string id = "id";
  const std::string dvalue = "delegation";
  const std::string attribute = "x509";
  Arc::XMLNode delegation = Arc::ClientSOAPTest::request["CreateActivity"]["DelegatedToken"];

  CPPUNIT_ASSERT(delegation);
  CPPUNIT_ASSERT_EQUAL(attribute, (std::string)delegation.Attribute("Format"));
  CPPUNIT_ASSERT(delegation["Id"]);
  CPPUNIT_ASSERT_EQUAL(id, (std::string)delegation["Id"]);
  CPPUNIT_ASSERT(delegation["Value"]);
  CPPUNIT_ASSERT_EQUAL(dvalue, (std::string)delegation["Value"]);
}

void AREXClientTest::StatTest()
{
  const std::string node = "GetActivityStatusesResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("a-rex", "http://www.nordugrid.org/schemas/a-rex"));
  Arc::ClientSOAPTest::response->NewChild(node).NewChild("Response").NewChild("ActivityStatus").NewAttribute("state") = "state_value";
  (*Arc::ClientSOAPTest::response)[node]["Response"]["ActivityStatus"].NewChild("a-rex:State") = "nstate_value";
  (*Arc::ClientSOAPTest::response)[node]["Response"]["ActivityStatus"].NewChild("a-rex:State") = "2nd_value";
 
  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running Check
  std::string jobid = "<jobID>my_test_jobID_12345</jobID>";
  Arc::Job job;

  CPPUNIT_ASSERT(ac.stat(jobid, job));

  //Response Check
  CPPUNIT_ASSERT_EQUAL((std::string)"2nd_value:nstate_value", job.State());
}


void AREXClientTest::ServiceStatTest() {
  const std::string value = "Test value";
  const std::string query = "QueryResourcePropertiesResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(query) = value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running Check
  Arc::XMLNode status;
  CPPUNIT_ASSERT(ac.sstat(status));

  //Response Check
  CPPUNIT_ASSERT(status);
  CPPUNIT_ASSERT_EQUAL(query, status.Name());
  CPPUNIT_ASSERT_EQUAL(value, (std::string)status);
}

void AREXClientTest::ListServicesFromISISTest()
{
  std::string first_value = "test://AREXClientURL1.now";
  std::string third_value = "test://AREXClientURL3.now";
  const std::string node = "QueryResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("isis", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::XMLNode root =  Arc::ClientSOAPTest::response->NewChild(node);
  //1st entry
  Arc::XMLNode entry1 = root.NewChild("RegEntry");
  entry1.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.execution.arex";
  entry1["SrcAdv"].NewChild("EPR").NewChild("Address") = first_value;

  //2nd entry
  Arc::XMLNode entry2 = root.NewChild("RegEntry");
  entry2.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.infosys.isis";
  entry2.NewChild("MetaSrcAdv").NewChild("ServiceID") = "Test service ID";

  //3rd entry
  Arc::XMLNode entry3 = root.NewChild("RegEntry");
  entry3.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.execution.arex";
  entry3["SrcAdv"].NewChild("EPR").NewChild("Address") = third_value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running Check
  std::list< std::pair<Arc::URL, Arc::ServiceType> > services;
  CPPUNIT_ASSERT(ac.listServicesFromISIS(services));

  //Response Check
  CPPUNIT_ASSERT_EQUAL(services.front().first, Arc::URL(first_value));
  CPPUNIT_ASSERT_EQUAL(services.front().second, Arc::COMPUTING);

  CPPUNIT_ASSERT_EQUAL(services.back().first, Arc::URL(third_value));
  CPPUNIT_ASSERT_EQUAL(services.back().second, Arc::COMPUTING);
}

void AREXClientTest::KillTest()
{
   const std::string node = "TerminateActivitiesResponse";

   Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("a-rex", "http://www.nordugrid.org/schemas/a-rex"));
   Arc::ClientSOAPTest::response->NewChild(node).NewChild("Response").NewChild("Terminated") = "true";
 
   Arc::ClientSOAPTest::status = Arc::STATUS_OK;

   //Running Check
   std::string jobid;
   CPPUNIT_ASSERT(ac.kill(jobid));
}

void AREXClientTest::CleanTest()
{
    const std::string value = "Test value";
    const std::string node = "ChangeActivityStatusResponse";
 
    Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("a-rex", "http://www.nordugrid.org/schemas/a-rex"));
    Arc::ClientSOAPTest::response->NewChild(node).NewChild("Response") = value;
 
    Arc::ClientSOAPTest::status = Arc::STATUS_OK;
 
    //Running Check
    std::string jobid;
    CPPUNIT_ASSERT(ac.clean(jobid));
}

void AREXClientTest::GetdescTest()
{
     const std::string value = "Test value";
     const std::string testnode = "TestNode";
     const std::string node = "GetActivityDocumentsResponse";
 
     Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("a-rex", "http://www.nordugrid.org/schemas/a-rex"));
     Arc::ClientSOAPTest::response->NewChild(node).NewChild("Response").NewChild("JobDefinition").NewChild(testnode) = value;
 
     Arc::ClientSOAPTest::status = Arc::STATUS_OK;
 
     //Running Check
     std::string jobid;
     std::string jobdesc;
     CPPUNIT_ASSERT(ac.getdesc(jobid, jobdesc));

     //Response Check
     Arc::XMLNode xml(jobdesc);
     CPPUNIT_ASSERT_EQUAL((std::string)"JobDefinition", xml.Name());
     CPPUNIT_ASSERT_EQUAL(testnode, xml[testnode].Name());
     CPPUNIT_ASSERT_EQUAL(value, (std::string)xml[testnode]);
}

void AREXClientTest::MigrateTest()
{
      const std::string value = "Test value";
      const std::string testnode = "TestNode";
      const std::string node = "MigrateActivityResponse";
 
      Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("a-rex", "http://www.nordugrid.org/schemas/a-rex"));
      Arc::ClientSOAPTest::response->NewChild(node).NewChild("ActivityIdentifier").NewChild(testnode) = value;
 
      Arc::ClientSOAPTest::status = Arc::STATUS_OK;
 
      //Running Check
      std::string jobid;
      std::string jobdesc;
      bool forcemigration = false;
      std::string newjobid;
      bool delegate = false;
      CPPUNIT_ASSERT(ac.migrate(jobid, jobdesc, forcemigration, newjobid, delegate));

      //Response Check
      Arc::XMLNode xml(newjobid);
      CPPUNIT_ASSERT_EQUAL((std::string)"ActivityIdentifier", xml.Name());
      CPPUNIT_ASSERT_EQUAL(testnode, xml[testnode].Name());
      CPPUNIT_ASSERT_EQUAL(value, (std::string)xml[testnode]);
}

void AREXClientTest::ResumeTest()
{
      const std::string value = "Test value";
      const std::string node = "ChangeActivityStatusResponse";
 
      Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("a-rex", "http://www.nordugrid.org/schemas/a-rex"));
      Arc::ClientSOAPTest::response->NewChild(node).NewChild("Response") = value;
         
       Arc::ClientSOAPTest::status = Arc::STATUS_OK;
        
       //Running Check
       std::string jobid;
       CPPUNIT_ASSERT(ac.resume(jobid));
}

void AREXClientTest::CreateActivityIdentifierTest()
{
  Arc::URL url("test://AREXClientTest.now/jobid12345");
  std::string a_rex = "http://www.nordugrid.org/schemas/a-rex";
  std::string bes_factory = "http://schemas.ggf.org/bes/2006/08/bes-factory";
  std::string wsa = "http://www.w3.org/2005/08/addressing";
  std::string jsdl = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
  std::string jsdl_posix = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
  std::string jsdl_arc = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
  std::string jsdl_hpcpa = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
  const std::string root = "ActivityIdentifier";

  //Running
  std::string activityIdentifier;
  ac.createActivityIdentifier(url, activityIdentifier);

  Arc::XMLNode result(activityIdentifier);
  CPPUNIT_ASSERT_EQUAL(root, result.Name());

  //Namespaces checking
  CPPUNIT_ASSERT_EQUAL(a_rex, result.Namespaces()["a-rex"]);
  CPPUNIT_ASSERT_EQUAL(bes_factory, result.Namespaces()["bes-factory"]);
  CPPUNIT_ASSERT_EQUAL(wsa, result.Namespaces()["wsa"]);
  CPPUNIT_ASSERT_EQUAL(jsdl, result.Namespaces()["jsdl"]);
  CPPUNIT_ASSERT_EQUAL(jsdl_posix, result.Namespaces()["jsdl-posix"]);
  CPPUNIT_ASSERT_EQUAL(jsdl_arc, result.Namespaces()["jsdl-arc"]);
  CPPUNIT_ASSERT_EQUAL(jsdl_hpcpa, result.Namespaces()["jsdl-hpcpa"]);

  //Values checking
  CPPUNIT_ASSERT_EQUAL(std::string("Address"), result["Address"].Name());
  CPPUNIT_ASSERT_EQUAL(url.Protocol()+"://"+url.Host(), (std::string)result["Address"]);

  CPPUNIT_ASSERT_EQUAL(std::string("JobID"), (result["ReferenceParameters"]["JobID"]).Name());
  CPPUNIT_ASSERT_EQUAL(url.Path(), (std::string)result["ReferenceParameters"]["JobID"]);

}

CPPUNIT_TEST_SUITE_REGISTRATION(AREXClientTest);
