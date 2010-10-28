#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/client/JobState.h>

class JobStateTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobStateTest);
  CPPUNIT_TEST(GetStateTypeTest);
  CPPUNIT_TEST(GetGeneralStateTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobStateTest() {}
  void setUp() {}
  void tearDown() {}
  void GetStateTypeTest();
  void GetGeneralStateTest();
};

void JobStateTest::GetStateTypeTest() {
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::ACCEPTED,   Arc::JobState::GetStateType("Accepted"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::PREPARING,  Arc::JobState::GetStateType("Preparing"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::SUBMITTING, Arc::JobState::GetStateType("Submitting"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::HOLD,       Arc::JobState::GetStateType("Hold"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::QUEUING,    Arc::JobState::GetStateType("Queuing"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::RUNNING,    Arc::JobState::GetStateType("Running"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::FINISHING,  Arc::JobState::GetStateType("Finishing"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::FINISHED,   Arc::JobState::GetStateType("Finished"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::KILLED,     Arc::JobState::GetStateType("Killed"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::DELETED,    Arc::JobState::GetStateType("Deleted"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::OTHER,      Arc::JobState::GetStateType("Other"));
  CPPUNIT_ASSERT_EQUAL(Arc::JobState::UNDEFINED,  Arc::JobState::GetStateType("UnknownState"));
}

void JobStateTest::GetGeneralStateTest() {
  CPPUNIT_ASSERT_EQUAL((std::string)"Accepted",   Arc::JobState::StateTypeString[Arc::JobState::ACCEPTED]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Preparing",  Arc::JobState::StateTypeString[Arc::JobState::PREPARING]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Submitting", Arc::JobState::StateTypeString[Arc::JobState::SUBMITTING]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Hold",       Arc::JobState::StateTypeString[Arc::JobState::HOLD]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Queuing",    Arc::JobState::StateTypeString[Arc::JobState::QUEUING]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Running",    Arc::JobState::StateTypeString[Arc::JobState::RUNNING]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Finishing",  Arc::JobState::StateTypeString[Arc::JobState::FINISHING]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Finished",   Arc::JobState::StateTypeString[Arc::JobState::FINISHED]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Killed",     Arc::JobState::StateTypeString[Arc::JobState::KILLED]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Deleted",    Arc::JobState::StateTypeString[Arc::JobState::DELETED]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Other",      Arc::JobState::StateTypeString[Arc::JobState::OTHER]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Undefined",  Arc::JobState::StateTypeString[Arc::JobState::UNDEFINED]);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobStateTest);
