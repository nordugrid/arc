#ifndef AREXCLIENTTEST2_H
#define AREXCLIENTTEST2_H

#include <cppunit/extensions/HelperMacros.h>

#include <arc/client/ClientInterface.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>

#define NOT
#include "TestClasses.h"
#undef NOT

#define TEST
#ifndef AREXCLIENT_CPP
#define AREXCLIENT_CPP
#include "../AREXClient.cpp"
#endif
#undef TEST

class AREXClientTest2 : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( AREXClientTest2 );
  CPPUNIT_TEST( testArexClientsstat );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void testArexClientsstat();
};

#endif  // AREXCLIENTTEST2_H

