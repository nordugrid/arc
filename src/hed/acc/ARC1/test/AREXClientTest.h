#ifndef AREXCLIENTTEST_H
#define AREXCLIENTTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <arc/client/ClientInterface.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>

#include "TestClasses.h"

#define TEST
#ifndef AREXCLIENT_CPP
#define AREXCLIENT_CPP
#include "../AREXClient.cpp"
#endif
#undef TEST

class ClientsTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ClientsTest );
  CPPUNIT_TEST( testArexClientsstat );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void testArexClientsstat();
};

#endif  // AREXCLIENTTEST_H

