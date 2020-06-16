#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ClientsTest.h"
#include <arc/communication/ClientInterface.h>

#define TEST
#include "../../clients/wsrf/arcwsrf.cpp"
#include "../../clients/compute/arccat.cpp"
#include "../../clients/compute/arcinfo.cpp"
#include "../../clients/compute/arcrenew.cpp"
#include "../../clients/compute/arcstat.cpp"
#include "../../clients/compute/arcclean.cpp"
#include "../../clients/compute/arckill.cpp"
#include "../../clients/compute/arcresub.cpp"
#include "../../clients/compute/arcsub.cpp"
#include "../../clients/compute/arcget.cpp"
#include "../../clients/compute/arcresume.cpp"
#include "../../clients/compute/arcsync.cpp"
#undef TEST


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
ClientsTest::testConstructor()
{
  const std::string command("echo 10");
  int ret;

  // Process
  ret=system(command.c_str());

  // Check
  CPPUNIT_ASSERT(!ret);
  //CPPUNIT_FAIL( "not implemented" );
}


void 
ClientsTest::testARCwsrf()
{
  int argc(2);
  char **argv;
  int ret;

  std::string url("http://arc-emi.grid.upjs.sk:60000/isis");

  char urlv[url.size()];
  for (int i=0;i<url.size();i++){
    urlv[i]=url[i];
  }

  char *_argv[1];
  _argv[0]=&urlv[0];
  argv=&_argv[0];


  // Process
  ret=test_arcwsrf_main(argc, argv);

  // Check
  CPPUNIT_ASSERT(!ret);
}

void
ClientsTest::testarccat()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcclean()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcget()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcinfo()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarckill()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcrenew()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcresub()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcresume()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcsub()
{
  CPPUNIT_FAIL( "not implemented" );
}

void
ClientsTest::testarcsync()
{
  CPPUNIT_FAIL( "not implemented" );
}

