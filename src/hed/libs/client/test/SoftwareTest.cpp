#include <cppunit/extensions/HelperMacros.h>

#include <list>

#include <arc/client/Software.h>
#include <arc/client/ExecutionTarget.h>

#define SV Arc::Software
#define SR Arc::SoftwareRequirement

class SoftwareTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SoftwareTest);
  CPPUNIT_TEST(EqualityTest);
  CPPUNIT_TEST(ComparisonTest);
  CPPUNIT_TEST(BasicRequirementsTest);
  CPPUNIT_TEST(RequirementsAndTest);
  CPPUNIT_TEST(RequirementsOrTest);
  CPPUNIT_TEST(RequirementsNotTest);
  CPPUNIT_TEST(RequirementsGreaterThanTest);
  CPPUNIT_TEST(RequirementsLessThanTest);
  CPPUNIT_TEST(RequirementsRangeTest);
  CPPUNIT_TEST(RequirementsGreaterThanAndTest);
  CPPUNIT_TEST(RequirementsLessThanAndTest);
  CPPUNIT_TEST(ApplicationEnvironmentCastTest);
  CPPUNIT_TEST_SUITE_END();

public:
  SoftwareTest() {}
  void setUp() {}
  void tearDown() {}
  void EqualityTest();
  void ComparisonTest();
  void BasicRequirementsTest();
  void RequirementsAndTest();
  void RequirementsOrTest();
  void RequirementsNotTest();
  void RequirementsGreaterThanTest();
  void RequirementsLessThanTest();
  void RequirementsRangeTest();
  void RequirementsGreaterThanAndTest();
  void RequirementsLessThanAndTest();
  void ApplicationEnvironmentCastTest();

private:
  std::list<SV> versions;
};


void SoftwareTest::EqualityTest() {
  CPPUNIT_ASSERT(SV("XX") == SV("XX"));
  CPPUNIT_ASSERT(!(SV("XX") != SV("XX")));
  CPPUNIT_ASSERT(!(SV("XX-YY") != SV("XX.YY")));
  CPPUNIT_ASSERT(!(SV("XX") != SV("YY")));
  CPPUNIT_ASSERT(!(SV("XX-1.2") != SV("XX")));
  CPPUNIT_ASSERT(!(SV("XX") != SV("XX-1.2")));
  CPPUNIT_ASSERT(!(SV("XX-1.2") != SV("XX-1.2")));
  CPPUNIT_ASSERT(SV("XX-1.2") != SV("XX-1.3"));
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
}

void SoftwareTest::RequirementsAndTest() {
  SR sr(true); sr.add(SV("A-1.03")); sr.add(SV("B-2.12"));

  versions.push_back(SV("A-1.03"));
  versions.push_back(SV("B-2.12"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(2, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
}

void SoftwareTest::RequirementsOrTest() {
  SR sr(false);

  // Test: A-3.83 OR A-3.84
  sr.add(SV("A-3.83")); sr.add(SV("A-3.84"));
  versions.push_back(SV("A-3.83"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size()); // Only 1 software should be chosen.
  CPPUNIT_ASSERT_EQUAL(versions.front(), sr.getSoftwareList().front()); // The chosen software should equal the one from the list.
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
  sr.clear();

  // Test: Order should have no influence.
  sr.add(SV("A-3.83"));
  sr.add(SV("A-3.84"));
  versions.push_back(SV("A-3.84"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(versions.front(), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
  sr.clear();

  // Test if chosen software has highest version number.
  sr.add(SV("A-3.83"));
  sr.add(SV("A-3.84"));
  versions.push_back(SV("A-3.83"));
  versions.push_back(SV("A-3.84"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(versions.back(), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
}

void SoftwareTest::RequirementsNotTest() {
  SR sr(false);

  sr.add(SV("A-1.3"), &Arc::Software::operator!=);
  versions.push_back(SV("A-1.3"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("A-1.2"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.2"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
  sr.clear();

  versions.push_back(SV("B-2"));
  sr.add(SV("A-1.3"), &Arc::Software::operator!=);
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  sr.clear();

  versions.push_back(SV("B-3"));
  versions.push_back(SV("A-1.2"));
  versions.push_back(SV("A-1.3"));
  sr.setRequirement(true);
  sr.add(SV("A-1.3"), &Arc::Software::operator!=);
  sr.add(SV("B-2"), &Arc::Software::operator!=);
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(2, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT(SV("A-1.2") == sr.getSoftwareList().front() ||
                 SV("A-1.2") == sr.getSoftwareList().back());
  CPPUNIT_ASSERT(SV("B-3") == sr.getSoftwareList().front() ||
                 SV("B-3") == sr.getSoftwareList().back());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
}

void SoftwareTest::RequirementsGreaterThanTest() {
  SR sr1(true), sr2(true);

  sr1.add(SV("A-1.3"), &Arc::Software::operator>);
  sr2.add(SV("A-1.3"), &Arc::Software::operator>=);
  versions.push_back(SV("A-1.2"));
  // A-1.2 > A-1.3 => false.
  CPPUNIT_ASSERT(!sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr1.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr1.isResolved());
  // A-1.2 >= A-1.3 => false.
  CPPUNIT_ASSERT(!sr2.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr2.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr2.isResolved());
  versions.push_back(SV("A-1.3"));
  // {A-1.2 , A-1.3} > A-1.3 => false.
  CPPUNIT_ASSERT(!sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr1.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr1.isResolved());
  // {A-1.2 , A-1.3} >= A-1.3 => true.
  CPPUNIT_ASSERT(sr2.isSatisfied(versions));
  CPPUNIT_ASSERT(sr2.selectSoftware(versions));
  // A-1.3 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr2.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr2.getSoftwareList().front());
  CPPUNIT_ASSERT(sr2.isResolved());
  sr2.clear();
  sr2.add(SV("A-1.3"), &Arc::Software::operator>=);
  versions.push_back(SV("A-1.4"));
  // {A-1.2 , A-1.3 , A-1.4} > A-1.3 => true.
  CPPUNIT_ASSERT(sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(sr1.selectSoftware(versions));
  // A-1.4 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr1.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.4"), sr1.getSoftwareList().front());
  CPPUNIT_ASSERT(sr1.isResolved());
  // {A-1.2 , A-1.3 , A-1.4} >= A-1.3 => true.
  CPPUNIT_ASSERT(sr2.isSatisfied(versions));
  CPPUNIT_ASSERT(sr2.selectSoftware(versions));
  // A-1.4 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr2.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.4"), sr2.getSoftwareList().front());
  CPPUNIT_ASSERT(sr2.isResolved());

  sr1.clear();
  sr1.add(SV("A-1.3"), &Arc::Software::operator>);
  sr1.add(SV("A-1.4"), &Arc::Software::operator!=);
  /*
   * {A-1.2 , A-1.3 , A-1.4} >  A-1.3 &&
   *                         != A-1.4    => false.
   */
  CPPUNIT_ASSERT(!sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr1.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr1.isResolved());
  versions.push_back(SV("A-1.4.2"));
  /*
   * {A-1.2 , A-1.3 , A-1.4 , A-1.4.2} >  A-1.3 &&
   *                                   != A-1.4    => true.
   */
  CPPUNIT_ASSERT(sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(sr1.selectSoftware(versions));
  // A-1.4.2 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr1.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.4.2"), sr1.getSoftwareList().front());
  CPPUNIT_ASSERT(sr1.isResolved());
  sr1.clear();
  versions.clear();
}

void SoftwareTest::RequirementsLessThanTest() {
  SR sr1(true), sr2(true);

  sr1.add(SV("A-1.3"), &Arc::Software::operator<);
  sr2.add(SV("A-1.3"), &Arc::Software::operator<=);
  versions.push_back(SV("A-1.4"));
  // A-1.2 > A-1.3 => false.
  CPPUNIT_ASSERT(!sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr1.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr1.isResolved());
  // A-1.2 >= A-1.3 => false.
  CPPUNIT_ASSERT(!sr2.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr2.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr2.isResolved());
  versions.push_back(SV("A-1.3"));
  // {A-1.2 , A-1.3} > A-1.3 => false.
  CPPUNIT_ASSERT(!sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr1.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr1.isResolved());
  // {A-1.2 , A-1.3} >= A-1.3 => true.
  CPPUNIT_ASSERT(sr2.isSatisfied(versions));
  CPPUNIT_ASSERT(sr2.selectSoftware(versions));
  // A-1.3 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr2.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr2.getSoftwareList().front());
  CPPUNIT_ASSERT(sr2.isResolved());
  sr2.clear();
  sr2.add(SV("A-1.3"), &Arc::Software::operator<=);
  versions.push_back(SV("A-1.2"));
  // {A-1.2 , A-1.3 , A-1.4} > A-1.3 => true.
  CPPUNIT_ASSERT(sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(sr1.selectSoftware(versions));
  // A-1.2 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr1.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.2"), sr1.getSoftwareList().front());
  CPPUNIT_ASSERT(sr1.isResolved());
  // {A-1.2 , A-1.3 , A-1.4} >= A-1.3 => true.
  CPPUNIT_ASSERT(sr2.isSatisfied(versions));
  CPPUNIT_ASSERT(sr2.selectSoftware(versions));
  // A-1.3 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr2.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.3"), sr2.getSoftwareList().front());
  CPPUNIT_ASSERT(sr2.isResolved());

  sr1.clear();
  sr1.add(SV("A-1.3"), &Arc::Software::operator<);
  sr1.add(SV("A-1.2"), &Arc::Software::operator!=);
  /*
   * {A-1.2 , A-1.3 , A-1.4} <  A-1.3 &&
   *                         != A-1.2    => false.
   */
  CPPUNIT_ASSERT(!sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr1.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr1.isResolved());
  versions.push_back(SV("A-1.1.5"));
  /*
   * {A-1.1.5 , A-1.2 , A-1.3 , A-1.4} <  A-1.3 &&
   *                                   != A-1.2    => true.
   */
  CPPUNIT_ASSERT(sr1.isSatisfied(versions));
  CPPUNIT_ASSERT(sr1.selectSoftware(versions));
  // A-1.1.5 should be selected.
  CPPUNIT_ASSERT_EQUAL(1, (int) sr1.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-1.1.5"), sr1.getSoftwareList().front());
  CPPUNIT_ASSERT(sr1.isResolved());
  sr1.clear();
  versions.clear();
}

void SoftwareTest::RequirementsRangeTest() {
  SR sr(true);

  sr.add(SV("A-4.17"), &Arc::Software::operator>);
  sr.add(SV("A-5.0"), &Arc::Software::operator<);
  versions.push_back(SV("A-4.16"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("A-5.1"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("A-4.20"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(1, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(SV("A-4.20"), sr.getSoftwareList().front());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
  sr.clear();
}

void SoftwareTest::RequirementsGreaterThanAndTest() {
  SR sr(true);

  sr.add(SV("A-4.17"), &Arc::Software::operator>);
  sr.add(SV("B-0.15"), &Arc::Software::operator>);
  versions.push_back(SV("A-4.16"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("A-4.18"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("B-0.10"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("B-0.20"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(2, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT(SV("A-4.18") == sr.getSoftwareList().front() ||
                 SV("A-4.18") == sr.getSoftwareList().back());
  CPPUNIT_ASSERT(SV("B-0.20") == sr.getSoftwareList().front() ||
                 SV("B-0.20") == sr.getSoftwareList().back());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
  sr.clear();
}

void SoftwareTest::RequirementsLessThanAndTest() {
  SR sr(true);

  sr.add(SV("A-4.17"), &Arc::Software::operator<);
  sr.add(SV("B-0.15"), &Arc::Software::operator<);
  versions.push_back(SV("A-4.18"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("A-4.16"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("B-0.20"));
  CPPUNIT_ASSERT(!sr.isSatisfied(versions));
  CPPUNIT_ASSERT(!sr.selectSoftware(versions));
  CPPUNIT_ASSERT(!sr.isResolved());
  versions.push_back(SV("B-0.10"));
  CPPUNIT_ASSERT(sr.isSatisfied(versions));
  CPPUNIT_ASSERT(sr.selectSoftware(versions));
  CPPUNIT_ASSERT_EQUAL(2, (int) sr.getSoftwareList().size());
  CPPUNIT_ASSERT(SV("A-4.16") == sr.getSoftwareList().front() ||
                 SV("A-4.16") == sr.getSoftwareList().back());
  CPPUNIT_ASSERT(SV("B-0.10") == sr.getSoftwareList().front() ||
                 SV("B-0.10") == sr.getSoftwareList().back());
  CPPUNIT_ASSERT(sr.isResolved());
  versions.clear();
  sr.clear();
}

void SoftwareTest::ApplicationEnvironmentCastTest() {
  std::list<Arc::ApplicationEnvironment> appEnvs(1, Arc::ApplicationEnvironment("TEST", "1.0"));
  const std::list<Arc::Software>& sw = reinterpret_cast< const std::list<SV>& >(appEnvs);
  CPPUNIT_ASSERT_EQUAL(static_cast<SV>(appEnvs.front()), sw.front());
}

CPPUNIT_TEST_SUITE_REGISTRATION(SoftwareTest);
