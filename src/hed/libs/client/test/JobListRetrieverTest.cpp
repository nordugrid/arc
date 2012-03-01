#include <cppunit/extensions/HelperMacros.h>

#include <arc/Endpoint.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/EndpointRetriever.h>
#include <arc/client/TestACCControl.h>

//static Arc::Logger testLogger(Arc::Logger::getRootLogger(), "JobListRetrieverTest");

class JobListRetrieverTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobListRetrieverTest);
  CPPUNIT_TEST(PluginLoading);
  CPPUNIT_TEST(QueryTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobListRetrieverTest() {};

  void setUp() {}
  void tearDown() {}

  void PluginLoading();
  void QueryTest();
};

void JobListRetrieverTest::PluginLoading() {
  Arc::EndpointRetrieverPluginLoader<Arc::ComputingInfoEndpoint, Arc::Job> l;
  Arc::EndpointRetrieverPlugin<Arc::ComputingInfoEndpoint, Arc::Job>* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);
}


void JobListRetrieverTest::QueryTest() {
  Arc::EndpointQueryingStatus sInitial(Arc::EndpointQueryingStatus::SUCCESSFUL);

  Arc::JobListRetrieverPluginTESTControl::delay = 1;
  Arc::JobListRetrieverPluginTESTControl::status = sInitial;

  Arc::EndpointRetrieverPluginLoader<Arc::ComputingInfoEndpoint, Arc::Job> l;
  Arc::EndpointRetrieverPlugin<Arc::ComputingInfoEndpoint, Arc::Job>* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);

  Arc::UserConfig uc;
  Arc::ComputingInfoEndpoint endpoint;
  std::list<Arc::Job> jobs;
  Arc::EndpointQueryingStatus sReturned = p->Query(uc, endpoint, jobs, Arc::EndpointQueryOptions<Arc::Job>());
  CPPUNIT_ASSERT(sReturned == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobListRetrieverTest);
