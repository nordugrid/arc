#include "AREXClientTest.h"
#include <arc/client/ClientInterface.h>

#include <arc/UserConfig.h>
#include <arc/message/MCC.h>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ClientsTest );


void
ClientsTest::setUp()
{
}

void 
ClientsTest::tearDown()
{
}

void
ClientsTest::testArexClientsstat()
{
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  Arc::URL url;
  Arc::MCCConfig cfg;
  int timeout(1);
  Arc::AREXClient _ac(url, cfg, timeout, true);
  Arc::XMLNode status;

  //Running Check  
  CPPUNIT_ASSERT( _ac.sstat(status) );
  std::string s;
  status.GetXML(s,true);
  std::cout << "status: " << s << std::endl;
 
  //Response Check
  CPPUNIT_ASSERT( s  == "<QueryResourcePropertiesResponse>Test value</QueryResourcePropertiesResponse>" ); 
}

