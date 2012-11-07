#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <arc/compute/Endpoint.h>
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/ExecutionTarget.h>

class ComputingServiceUniqTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ComputingServiceUniqTest);
  CPPUNIT_TEST(KeepServicesWithDifferentID);
  CPPUNIT_TEST(KeepOnlyOneServiceWithTheSameID);
  CPPUNIT_TEST(KeepHigherPriorityService);
  CPPUNIT_TEST_SUITE_END();

public:
  ComputingServiceUniqTest() {};

  void setUp() {}
  void tearDown() {}
  
  void KeepServicesWithDifferentID();
  void KeepOnlyOneServiceWithTheSameID();
  void KeepHigherPriorityService();
};

void ComputingServiceUniqTest::KeepServicesWithDifferentID() {
  Arc::ComputingServiceType cs1;
  cs1->ID = "ID1";
  Arc::ComputingServiceType cs2;
  cs2->ID = "ID2";
  Arc::ComputingServiceUniq csu;
  csu.addEntity(cs1);
  csu.addEntity(cs2);
  std::list<Arc::ComputingServiceType> services = csu.getServices();
  CPPUNIT_ASSERT(services.size() == 2);
  CPPUNIT_ASSERT(services.front()->ID == "ID1");
  CPPUNIT_ASSERT(services.back()->ID == "ID2");
}

void ComputingServiceUniqTest::KeepOnlyOneServiceWithTheSameID() {
  Arc::ComputingServiceType cs1;
  cs1->ID = "ID";
  Arc::ComputingServiceType cs2;
  cs2->ID = "ID";
  Arc::ComputingServiceUniq csu;
  csu.addEntity(cs1);
  csu.addEntity(cs2);
  std::list<Arc::ComputingServiceType> services = csu.getServices();
  CPPUNIT_ASSERT(services.size() == 1);
  CPPUNIT_ASSERT(services.front()->ID == "ID");
}

void ComputingServiceUniqTest::KeepHigherPriorityService() {
  Arc::ComputingServiceType cs1;
  cs1->ID = "ID";
  cs1->Type = "lower";
  Arc::Endpoint origin1;
  origin1.InterfaceName = "org.nordugrid.ldapng";
  cs1->OriginalEndpoint = origin1;
  Arc::ComputingServiceType cs2;
  cs2->ID = "ID";
  cs2->Type = "higher";
  Arc::Endpoint origin2;
  origin2.InterfaceName = "org.nordugrid.ldapglue2";
  cs2->OriginalEndpoint = origin2;
  Arc::ComputingServiceUniq csu;
  csu.addEntity(cs1);
  csu.addEntity(cs2);
  std::list<Arc::ComputingServiceType> services = csu.getServices();
  CPPUNIT_ASSERT(services.size() == 1);
  CPPUNIT_ASSERT(services.front()->Type == "higher");
}


CPPUNIT_TEST_SUITE_REGISTRATION(ComputingServiceUniqTest);
