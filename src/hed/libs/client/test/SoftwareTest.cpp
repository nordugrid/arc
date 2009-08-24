#include <cppunit/extensions/HelperMacros.h>

#include <arc/Logger.h>
#include <arc/client/Software.h>

#define SV Arc::Software
#define SR Arc::SoftwareRequirement

class SoftwareTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SoftwareTest);
  CPPUNIT_TEST(TestSoftwareComparison);
  CPPUNIT_TEST(TestSoftwareRequirement);
  CPPUNIT_TEST_SUITE_END();

public:
  SoftwareTest()
    : logger(Arc::Logger::getRootLogger(), "SoftwareTest"),
      logcerr(std::cerr) {}
  void setUp();
  void tearDown();
  void TestSoftwareComparison();
  void TestSoftwareRequirement();

private:
  Arc::LogStream logcerr;
  Arc::Logger logger;
};


void SoftwareTest::setUp() {
}

void SoftwareTest::tearDown() {
}

void SoftwareTest::TestSoftwareComparison() {
  CPPUNIT_ASSERT(SV("XX-YY") == SV("XX-YY"));
  CPPUNIT_ASSERT(SV("XX-YY") != SV("XX.YY"));
  CPPUNIT_ASSERT(SV("XX-YY") != SV("xx-yy"));

  CPPUNIT_ASSERT(!(SV("XX-YY") < SV("XX-YY-ZZ")));
  CPPUNIT_ASSERT(!(SV("XX-YY") > SV("XX-YY-ZZ")));
  CPPUNIT_ASSERT(!(SV("XX-YY-ZZ") < SV("XX-YY")));
  CPPUNIT_ASSERT(!(SV("XX-YY-ZZ") > SV("XX-YY")));
  
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3.ZZ.4") > SV("XX-YY-1.2.3.ZZ.4")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3.ZZ.4") < SV("XX-YY-1.2.3.ZZ.4")));
  CPPUNIT_ASSERT(SV("XX-YY-1.4.3.ZZ.4") > SV("XX-YY-1.2.3.ZZ.4"));
  CPPUNIT_ASSERT(SV("XX-YY-1.2.3.ZZ.4") < SV("XX-YY-1.4.3.ZZ.4"));
  CPPUNIT_ASSERT(SV("XX-YY-1.2.3.ZZ.4") < SV("XX-YY-1.2.3.ZZ.10"));
  CPPUNIT_ASSERT(SV("XX-YY-1.3.3.ZZ.4") > SV("XX-YY-1.2.7.ZZ.4"));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3.ZZ1.4") > SV("XX-YY-1.2.3.ZZ2.4")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3.ZZ1.4") < SV("XX-YY-1.2.3.ZZ2.4")));

  CPPUNIT_ASSERT(SV("XX-YY-1.2.3") < SV("XX-YY-1-2-3-4"));
  CPPUNIT_ASSERT(SV("XX-YY-1.2.3.4") > SV("XX-YY-1-2-3"));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3.4") < SV("XX-YY-1-2-3")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3") > SV("XX-YY-1-2-3-4")));
  
  CPPUNIT_ASSERT(SV("XX-YY-1.2") < SV("XX-YY-1.2.3"));
  CPPUNIT_ASSERT(SV("XX-YY-1.2.3") > SV("XX-YY-1.2"));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.ZZ") > SV("XX-YY-1.2")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3") < SV("XX-YY-1.2.ZZ")));

  CPPUNIT_ASSERT(SV("XX-YY-1.3") > SV("XX-YY-1.2.3"));
  CPPUNIT_ASSERT(SV("XX-YY-1.2.3") < SV("XX-YY-1.3"));
  CPPUNIT_ASSERT(SV("XX-YY-1.2.3") > SV("XX-YY-1.2"));
  CPPUNIT_ASSERT(SV("XX-YY-1.2") < SV("XX-YY-1.2.3"));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2") > SV("XX-YY-1.2.3")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3") < SV("XX-YY-1.2")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2") < SV("XX-YY-1.2.ZZ")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2") > SV("XX-YY-1.2.ZZ")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.ZZ") < SV("XX-YY-1.2")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.ZZ") > SV("XX-YY-1.2")));
}

void SoftwareTest::TestSoftwareRequirement() {
  std::list<SV> versions;
  versions.push_back(SV("A-1.03"));

  CPPUNIT_ASSERT(SR(versions.back()).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), &SV::operator!=).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(), &SV::operator<=).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(), &SV::operator>=).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), &SV::operator> ).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), &SV::operator< ).isSatisfied(versions));

  CPPUNIT_ASSERT(!SR(SV("A-1.5")).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(SV("A-1.5"), &SV::operator !=).isSatisfied(versions));

  CPPUNIT_ASSERT(SR(versions.back(), &SV::operator==, false).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), &SV::operator!=, false).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(), &SV::operator<=, false).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(), &SV::operator>=, false).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), &SV::operator>, false).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), &SV::operator<, false).isSatisfied(versions));

  CPPUNIT_ASSERT(!SR(SV("A-1.5"), &SV::operator==, false).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(SV("A-1.5"), &SV::operator!=, false).isSatisfied(versions));


  {
    logger.msg(Arc::DEBUG, "a) job needs A-1.03 AND B-2.12");
    
    SR sr; sr.add(SV("A-1.03")); sr.add(SV("B-2.12"));

    versions.push_back(SV("B-2.12"));
    CPPUNIT_ASSERT(sr.isSatisfied(versions));
    versions.clear();
  }

  {
    logger.msg(Arc::DEBUG, "b) job needs A-3.83 OR A-3.84");

    SR sr(false); sr.add(SV("A-3.83")); sr.add(SV("A-3.84"));

    versions.push_back(SV("A-3.83"));
    CPPUNIT_ASSERT(sr.isSatisfied(versions));
  }

  {
    logger.msg(Arc::DEBUG, "e) job needs any A except of A-2.45");
  }
  
  {
    logger.msg(Arc::DEBUG, "f) job needs any A > A-4.17");
  }

  {
    logger.msg(Arc::DEBUG, "g) job needs any A-4.17 < A < A-5.0");
    logger.msg(Arc::DEBUG, "g') job needs any A-2.* (a variant of g), only with unknown upper margin )");
  }

  {
    logger.msg(Arc::DEBUG, "h) job needs any A > A-4.17 and B > B-0.15");
  }

  {
    logger.msg(Arc::DEBUG, "i) job needs (A-4.17 < A < A-5.0) OR (A > A-6.0 AND any B)");
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(SoftwareTest);
