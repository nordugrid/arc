#ifndef CLIENTSTEST_H
#define CLIENTSTEST_H

#include <cppunit/extensions/HelperMacros.h>

class ClientsTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ClientsTest );
  CPPUNIT_TEST( testConstructor );
  CPPUNIT_TEST( testARCwsrf );
  CPPUNIT_TEST( testarccat );
  CPPUNIT_TEST( testarcclean );
  CPPUNIT_TEST( testarcget );
  CPPUNIT_TEST( testarcinfo );
  CPPUNIT_TEST( testarckill );
  CPPUNIT_TEST( testarcrenew );
  CPPUNIT_TEST( testarcresume );
  CPPUNIT_TEST( testarcsub );
  CPPUNIT_TEST( testarcsync );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void testConstructor();
  void testARCwsrf();
  void testarccat();
  void testarcclean();
  void testarcget();
  void testarcinfo();
  void testarckill();
  void testarcrenew();
  void testarcresume();
  void testarcsub();
  void testarcsync();

};

#endif  // CLIENTSTEST_H

