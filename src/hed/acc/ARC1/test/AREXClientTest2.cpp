#include "AREXClientTest2.h"
#include <arc/client/ClientInterface.h>

#include <arc/UserConfig.h>
#include <arc/message/MCC.h>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( AREXClientTest2 );


void
AREXClientTest2::setUp()
{
}

void 
AREXClientTest2::tearDown()
{
}

void
AREXClientTest2::testArexClientsstat()
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
  CPPUNIT_ASSERT( !_ac.sstat(status) );
}

