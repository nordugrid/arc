#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <list>

#include <arc/compute/Software.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/Thread.h>

#define SV Arc::Software
#define SR Arc::SoftwareRequirement

class SoftwareTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SoftwareTest);
  CPPUNIT_TEST(EqualityTest);
  CPPUNIT_TEST(ComparisonTest);
  CPPUNIT_TEST(BasicRequirementsTest);
  CPPUNIT_TEST(RequirementsAndTest);
  CPPUNIT_TEST(RequirementsAssignmentTest);
  CPPUNIT_TEST(RequirementsNotTest);
  CPPUNIT_TEST(RequirementsGreaterThanTest);
  CPPUNIT_TEST(RequirementsGreaterThanOrEqualTest);
  CPPUNIT_TEST(RequirementsLessThanTest);
  CPPUNIT_TEST(RequirementsLessThanOrEqualTest);
  CPPUNIT_TEST(ApplicationEnvironmentCastTest);
  CPPUNIT_TEST_SUITE_END();

public:
  SoftwareTest() {}
  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }
  void EqualityTest();
  void ComparisonTest();
  void BasicRequirementsTest();
  void RequirementsAndTest();
  void RequirementsAssignmentTest();
   void RequirementsNotTest();
  void RequirementsGreaterThanTest();
  void RequirementsGreaterThanOrEqualTest();
  void RequirementsLessThanTest();
  void RequirementsLessThanOrEqualTest();
  void ApplicationEnvironmentCastTest();

private:
  std::list<SV> versions;
};


void SoftwareTest::EqualityTest() {
  CPPUNIT_ASSERT(SV("XX") == SV("XX"));
  CPPUNIT_ASSERT(!(SV("XX") == SV("YY")));
  CPPUNIT_ASSERT(SV("XX-YY") == SV("XX-YY"));
  CPPUNIT_ASSERT(SV("XX-YY-1.3") == SV("XX-YY-1.3"));
  CPPUNIT_ASSERT(SV("XX-YY-1.3") == SV("XX-YY", "1.3"));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.3") == SV("XX-YY")));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.3") == SV("XX-YY-1.2")));
}

void SoftwareTest::ComparisonTest() {
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

  CPPUNIT_ASSERT(SV("XX-YY-1.3.2") > SV("XX-YY-1.2.3"));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.3.2") < SV("XX-YY-1.2.3")));
  CPPUNIT_ASSERT(SV("XX-YY-1.2.3") < SV("XX-YY-1.3.2"));
  CPPUNIT_ASSERT(!(SV("XX-YY-1.2.3") > SV("XX-YY-1.3.2")));
}

void SoftwareTest::BasicRequirementsTest() {
  versions.push_back(SV("A-1.03"));

  CPPUNIT_ASSERT(SR(versions.back(),  SV::EQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), SV::NOTEQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(),  SV::LESSTHANOREQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(),  SV::GREATERTHANOREQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), SV::GREATERTHAN).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), SV::LESSTHAN).isSatisfied(versions));

  CPPUNIT_ASSERT(!SR(SV("A-1.5"), SV::EQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(SV("A-1.5"), SV::NOTEQUAL).isSatisfied(versions));

  CPPUNIT_ASSERT(SR(versions.back(),  SV::EQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), SV::NOTEQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(),  SV::LESSTHANOREQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(versions.back(),  SV::GREATERTHANOREQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), SV::GREATERTHAN).isSatisfied(versions));
  CPPUNIT_ASSERT(!SR(versions.back(), SV::LESSTHAN).isSatisfied(versions));

  CPPUNIT_ASSERT(!SR(SV("A-1.5"), SV::EQUAL).isSatisfied(versions));
  CPPUNIT_ASSERT(SR(SV("A-1.5"),  SV::NOTEQUAL).isSatisfied(versions));
}

void SoftwareTest::RequirementsAndTest() {
  SR sr; sr.add(SV("A-1.03"), SV::EQUAL); sr.add(SV("B-2.12"), SV::EQUAL);

  versions.push_back(SV("A-1.03"));
  versions.push_back(SV("B-2.12"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(2, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
}

void SoftwareTest::RequirementsAssignmentTest() {
  SR sr;

  versions.push_back(SV("A-1.3"));
  versions.push_back(SV("A-1.4"));
  versions.push_back(SV("A-1.5"));
  versions.push_back(SV("A-2.1"));
  versions.push_back(SV("A-2.2"));
  versions.push_back(SV("A-2.3"));

  sr.add(SV("A-1.3"), SV::EQUAL);
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr.getSoftwareList().back());
  CPPUNIT_ASSERT(sr.isResolved());
  sr.clear();

  sr.add(SV("A-1.2"), SV::EQUAL);
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  sr.clear();
  
  sr.add(SV("A-1.3"), SV::EQUAL);
  sr.add(SV("A-2.4"), SV::EQUAL);
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(2, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT_EQUAL(SV("A-2.4"), sr.getSoftwareList().back());
  sr.clear();
}

void SoftwareTest::RequirementsNotTest() {
  SR sr;
  sr.add(SV("A-1.2"), SV::NOTEQUAL);

  versions.push_back(SV("A-1.3"));
  versions.push_back(SV("B-4.3"));

  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT(sr.isResolved());
  
  sr.add(SV("A-1.2"), SV::NOTEQUAL);
  versions.push_back(SV("A-1.2"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
}

void SoftwareTest::RequirementsGreaterThanTest() {
  SR sr;
  sr.add(SV("A-1.3"), Arc::Software::GREATERTHAN);

  // A-1.2 > A-1.3 => false.
  versions.push_back(SV("A-1.2"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());

  // {A-1.2 , A-1.3} > A-1.3 => false.
  versions.push_back(SV("A-1.3"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());

  // {A-1.2 , A-1.3 , A-1.4} > A-1.3 => true.
  versions.push_back(SV("A-1.4"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.4 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.4"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());

  sr.clear();
  sr.add(SV("A-1.3"), Arc::Software::GREATERTHAN);

  // {A-1.2 , A-1.3 , A-1.4, A-1.5} > A-1.3 => true.
  versions.push_back(SV("A-1.5"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.5 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.5"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());

  sr.clear();
  sr.add(SV("A-1"), Arc::Software::GREATERTHAN);

  // {A-1.2 , A-1.3 , A-1.4, A-1.5} > A => true.
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.5 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.5"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());

  sr.clear();
  sr.add(SV("A"), Arc::Software::GREATERTHAN);

  // {A-1.2 , A-1.3 , A-1.4, A-1.5} > A => true.
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.5 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.5"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
}

void SoftwareTest::RequirementsGreaterThanOrEqualTest() {
  SR sr;
  sr.add(SV("A-1.3"), Arc::Software::GREATERTHANOREQUAL);

  // A-1.2 >= A-1.3 => false.
  versions.push_back(SV("A-1.2"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());

  // {A-1.2 , A-1.3} >= A-1.3 => true.
  versions.push_back(SV("A-1.3"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.3 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());

  sr.clear();
  sr.add(SV("A-1.3"), Arc::Software::GREATERTHANOREQUAL);

  // {A-1.2 , A-1.3 , A-1.4} >= A-1.3 => true.
  versions.push_back(SV("A-1.4"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.4 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.4"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
}

void SoftwareTest::RequirementsLessThanTest() {
  SR sr;
  sr.add(SV("A-1.3"), Arc::Software::LESSTHAN);

  // A-1.4 < A-1.3 => false.
  versions.push_back(SV("A-1.4"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  
  // {A-1.4 , A-1.3} < A-1.3 => false.
  versions.push_back(SV("A-1.3"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  
  // {A-1.4 , A-1.3 , A-1.2} < A-1.3 => true.
  versions.push_back(SV("A-1.2"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.2 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.2"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
  
  sr.clear();
  sr.add(SV("A-1.3"), Arc::Software::LESSTHAN);
  
  // {A-1.4 , A-1.3 , A-1.2, A-1.1} < A-1.3 => true.
  versions.push_back(SV("A-1.1"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.2 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.2"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
}

void SoftwareTest::RequirementsLessThanOrEqualTest() {
  SR sr;
  sr.add(SV("A-1.3"), Arc::Software::LESSTHANOREQUAL);

  // A-1.4 <= A-1.3 => false.
  versions.push_back(SV("A-1.4"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  
  // {A-1.4 , A-1.3} <= A-1.3 => true.
  versions.push_back(SV("A-1.3"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.3 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
  
  sr.clear();
  sr.add(SV("A-1.3"), Arc::Software::LESSTHANOREQUAL);
  
  // {A-1.4 , A-1.3 , A-1.2} <= A-1.3 => true.
  versions.push_back(SV("A-1.2"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  // A-1.3 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
}

void SoftwareTest::ApplicationEnvironmentCastTest() {
  std::list<Arc::ApplicationEnvironment> appEnvs(1, Arc::ApplicationEnvironment("TEST", "1.0"));
  const std::list<Arc::Software>* sw = reinterpret_cast< const std::list<SV>* >(&appEnvs);
  CPPUNIT_ASSERT_EQUAL(static_cast<SV>(appEnvs.front()), sw->front());
}

CPPUNIT_TEST_SUITE_REGISTRATION(SoftwareTest);
