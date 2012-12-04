#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/compute/SubmissionStatus.h>

class SubmissionStatusTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SubmissionStatusTest);
  CPPUNIT_TEST(BasicTest);
  CPPUNIT_TEST(OrTest);
  CPPUNIT_TEST(AndTest);
  CPPUNIT_TEST(UnsetTest);
  CPPUNIT_TEST_SUITE_END();

public:
  SubmissionStatusTest() {}
  
  void setUp() {}
  void tearDown() {}
  
  void BasicTest();
  void OrTest();
  void AndTest();
  void UnsetTest();
};

void SubmissionStatusTest::BasicTest() {
  {
    Arc::SubmissionStatus s;
    CPPUNIT_ASSERT(s);
  }
  
  {
    Arc::SubmissionStatus s(Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(!s);
    CPPUNIT_ASSERT(s == Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(Arc::SubmissionStatus::NOT_IMPLEMENTED == s);
  }
  
  {
    Arc::SubmissionStatus s((unsigned int)Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(!s);
    CPPUNIT_ASSERT(Arc::SubmissionStatus::NOT_IMPLEMENTED == s);
  }
}

void SubmissionStatusTest::OrTest() {
  {
    Arc::SubmissionStatus s(Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(s == Arc::SubmissionStatus::NOT_IMPLEMENTED);
  
    s = s | Arc::SubmissionStatus::NO_SERVICES;
    CPPUNIT_ASSERT(s != Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NO_SERVICES));
    CPPUNIT_ASSERT(s == (unsigned int)(Arc::SubmissionStatus::NOT_IMPLEMENTED | Arc::SubmissionStatus::NO_SERVICES));
  }
  
  {
    Arc::SubmissionStatus s(Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(s == Arc::SubmissionStatus::NOT_IMPLEMENTED);
  
    s |= Arc::SubmissionStatus::NO_SERVICES;
    CPPUNIT_ASSERT(s != Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NO_SERVICES));
    CPPUNIT_ASSERT(s == (unsigned int)(Arc::SubmissionStatus::NOT_IMPLEMENTED | Arc::SubmissionStatus::NO_SERVICES));
  }
}

void SubmissionStatusTest::AndTest() {
  {
    Arc::SubmissionStatus s((unsigned int)(Arc::SubmissionStatus::NOT_IMPLEMENTED | Arc::SubmissionStatus::NO_SERVICES));
    CPPUNIT_ASSERT(s == (unsigned int)(Arc::SubmissionStatus::NOT_IMPLEMENTED | Arc::SubmissionStatus::NO_SERVICES));
  
    s = s & Arc::SubmissionStatus::NO_SERVICES;
    CPPUNIT_ASSERT(!s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));
    CPPUNIT_ASSERT(s == Arc::SubmissionStatus::NO_SERVICES);
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NO_SERVICES));
  }
  
  {
    Arc::SubmissionStatus s((unsigned int)(Arc::SubmissionStatus::NOT_IMPLEMENTED | Arc::SubmissionStatus::NO_SERVICES));
    CPPUNIT_ASSERT(s == (unsigned int)(Arc::SubmissionStatus::NOT_IMPLEMENTED | Arc::SubmissionStatus::NO_SERVICES));

    s &= Arc::SubmissionStatus::NO_SERVICES;
    CPPUNIT_ASSERT(!s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));
    CPPUNIT_ASSERT(s == Arc::SubmissionStatus::NO_SERVICES);
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NO_SERVICES));
  }
}

void SubmissionStatusTest::UnsetTest() {
  {
    Arc::SubmissionStatus s(Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(!s);
    CPPUNIT_ASSERT(s == Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));

    s.unset(Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(s);
    CPPUNIT_ASSERT(s != Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(!s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));
  }

  {
    Arc::SubmissionStatus s((unsigned int)(Arc::SubmissionStatus::NOT_IMPLEMENTED | Arc::SubmissionStatus::NO_SERVICES));
    CPPUNIT_ASSERT(!s);
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NO_SERVICES));

    s.unset(Arc::SubmissionStatus::NOT_IMPLEMENTED);
    CPPUNIT_ASSERT(!s);
    CPPUNIT_ASSERT(s == Arc::SubmissionStatus::NO_SERVICES);
    CPPUNIT_ASSERT(!s.isSet(Arc::SubmissionStatus::NOT_IMPLEMENTED));
    CPPUNIT_ASSERT(s.isSet(Arc::SubmissionStatus::NO_SERVICES));
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(SubmissionStatusTest);
