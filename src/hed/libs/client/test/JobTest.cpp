#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/Job.h>

class JobTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobTest);
  CPPUNIT_TEST(XMLToJobTest);
  CPPUNIT_TEST(JobToXMLTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobTest();
  void setUp() {}
  void tearDown() {}
  void XMLToJobTest();
  void JobToXMLTest();

private:
  Arc::XMLNode xmlJob;
};

JobTest::JobTest() : xmlJob(Arc::XMLNode("<ComputingActivity>"
    "<Name>mc08.1050.J7</Name>"
    "<Type>single</Type>"
    "<IDFromEndpoint>https://ce01.niif.hu:60000/arex/123456</IDFromEndpoint>"
    "<LocalIDFromManager>345.ce01</LocalIDFromManager>"
    "<JobDescription>nordugrid:xrsl</JobDescription>"
    "<State>bes:failed</State>"
    "<State>nordugrid:FAILED</State>"
    "<RestartState>bes:running</RestartState>"
    "<RestartState>nordugrid:FINISHING</RestartState>"
    "<ExitCode>0</ExitCode>"
    "<ComputingManagerExitCode>0</ComputingManagerExitCode>"
    "<Error>Uploading timed out</Error>"
    "<Error>Failed stage-out</Error>"
    "<WaitingPosition>0</WaitingPosition>"
    "<UserDomain>vo:atlas</UserDomain>"
    "<Owner>CONFIDENTIAL</Owner>"
    "<LocalOwner>grid02</LocalOwner>"
    "<RequestedTotalWallTime>5000</RequestedTotalWallTime>"
    "<RequestedTotalCPUTime>20000</RequestedTotalCPUTime>"
    "<RequestedSlots>4</RequestedSlots>"
    "<RequestedApplicationEnvironment>ENV/JAVA/JRE-1.6.0</RequestedApplicationEnvironment>"
    "<RequestedApplicationEnvironment>APPS/HEP/ATLAS-14.2.23.4</RequestedApplicationEnvironment>"
    "<StdIn>input.dat</StdIn>"
    "<StdOut>job.out</StdOut>"
    "<StdErr>err.out</StdErr>"
    "<LogDir>celog</LogDir>"
    "<ExecutionNode>wn043</ExecutionNode>"
    "<ExecutionNode>wn056</ExecutionNode>"
    "<Queue>pbs-short</Queue>"
    "<UsedTotalWallTime>2893</UsedTotalWallTime>"
    "<UsedTotalCPUTime>12340</UsedTotalCPUTime>"
    "<UsedMainMemory>4453</UsedMainMemory>"
    "<SubmissionTime>2008-04-21T10:05:12Z</SubmissionTime>"
    "<ComputingManagerSubmissionTime>2008-04-20T06:05:12Z</ComputingManagerSubmissionTime>"
    "<StartTime>2008-04-20T06:45:12Z</StartTime>"
    "<ComputingManagerEndTime>2008-04-20T10:05:12Z</ComputingManagerEndTime>"
    "<EndTime>2008-04-20T10:15:12Z</EndTime>"
    "<WorkingAreaEraseTime>2008-04-24T10:05:12Z</WorkingAreaEraseTime>"
    "<ProxyExpirationTime>2008-04-30T10:05:12Z</ProxyExpirationTime>"
    "<SubmissionHost>pc4.niif.hu:3432</SubmissionHost>"
    "<SubmissionClientName>nordugrid-arc-0.94</SubmissionClientName>"
    "<OtherMessages>Cached input file is outdated; downloading again</OtherMessages>"
    "<OtherMessages>User proxy has expired</OtherMessages>"
  "</ComputingActivity>")) {}

void JobTest::XMLToJobTest() {
  Arc::Job job;
  job = xmlJob;

  CPPUNIT_ASSERT_EQUAL((std::string)"mc08.1050.J7", job.Name);
  CPPUNIT_ASSERT_EQUAL((std::string)"single", job.Type);
  CPPUNIT_ASSERT_EQUAL((std::string)"345.ce01", job.LocalIDFromManager);
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid:xrsl", job.JobDescription);
  CPPUNIT_ASSERT(job.State == Arc::JobState::OTHER);
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:failed", job.State());
  CPPUNIT_ASSERT(job.RestartState == Arc::JobState::OTHER);
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:running", job.RestartState());
  CPPUNIT_ASSERT_EQUAL(0, job.ExitCode);
  CPPUNIT_ASSERT_EQUAL((std::string)"0", job.ComputingManagerExitCode);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.Error.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Uploading timed out", job.Error.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"Failed stage-out", job.Error.back());
  CPPUNIT_ASSERT_EQUAL(0, job.WaitingPosition);
  CPPUNIT_ASSERT_EQUAL((std::string)"vo:atlas", job.UserDomain);
  CPPUNIT_ASSERT_EQUAL((std::string)"CONFIDENTIAL", job.Owner);
  CPPUNIT_ASSERT_EQUAL((std::string)"grid02", job.LocalOwner);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(5000), job.RequestedTotalWallTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(20000), job.RequestedTotalCPUTime);
  CPPUNIT_ASSERT_EQUAL(4, job.RequestedSlots);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.RequestedApplicationEnvironment.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ENV/JAVA/JRE-1.6.0", job.RequestedApplicationEnvironment.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"APPS/HEP/ATLAS-14.2.23.4", job.RequestedApplicationEnvironment.back());
  CPPUNIT_ASSERT_EQUAL((std::string)"input.dat", job.StdIn);
  CPPUNIT_ASSERT_EQUAL((std::string)"job.out", job.StdOut);
  CPPUNIT_ASSERT_EQUAL((std::string)"err.out", job.StdErr);
  CPPUNIT_ASSERT_EQUAL((std::string)"celog", job.LogDir);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.ExecutionNode.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"wn043", job.ExecutionNode.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"wn056", job.ExecutionNode.back());
  CPPUNIT_ASSERT_EQUAL((std::string)"pbs-short", job.Queue);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(2893), job.UsedTotalWallTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Period(12340), job.UsedTotalCPUTime);
  CPPUNIT_ASSERT_EQUAL(4453, job.UsedMainMemory);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-21T10:05:12Z"), job.SubmissionTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T06:05:12Z"), job.ComputingManagerSubmissionTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T06:45:12Z"), job.StartTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T10:05:12Z"), job.ComputingManagerEndTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-20T10:15:12Z"), job.EndTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-24T10:05:12Z"), job.WorkingAreaEraseTime);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-30T10:05:12Z"), job.ProxyExpirationTime);
  CPPUNIT_ASSERT_EQUAL((std::string)"pc4.niif.hu:3432", job.SubmissionHost);
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid-arc-0.94", job.SubmissionClientName);
  CPPUNIT_ASSERT_EQUAL(2, (int)job.OtherMessages.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Cached input file is outdated; downloading again", job.OtherMessages.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"User proxy has expired", job.OtherMessages.back());
}

void JobTest::JobToXMLTest() {
  Arc::Job job;
  job = xmlJob;
  Arc::XMLNode xmlOut("<ComputingActivity/>");

  job.ToXML(xmlOut);

  CPPUNIT_ASSERT_EQUAL((std::string)"mc08.1050.J7", (std::string)xmlOut["Name"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"single", (std::string)xmlOut["Type"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"345.ce01", (std::string)xmlOut["LocalIDFromManager"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid:xrsl", (std::string)xmlOut["JobDescription"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Other", (std::string)xmlOut["State"]["General"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:failed", (std::string)xmlOut["State"]["Specific"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Other", (std::string)xmlOut["RestartState"]["General"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:running", (std::string)xmlOut["RestartState"]["Specific"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["ExitCode"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["ComputingManagerExitCode"]);
  CPPUNIT_ASSERT(xmlOut["Error"][0] && xmlOut["Error"][1] && !xmlOut["Error"][2]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Uploading timed out", (std::string)xmlOut["Error"][0]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Failed stage-out", (std::string)xmlOut["Error"][1]);
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["WaitingPosition"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"vo:atlas", (std::string)xmlOut["UserDomain"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"CONFIDENTIAL", (std::string)xmlOut["Owner"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"grid02", (std::string)xmlOut["LocalOwner"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"PT1H23M20S", (std::string)xmlOut["RequestedTotalWallTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"PT5H33M20S", (std::string)xmlOut["RequestedTotalCPUTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"4", (std::string)xmlOut["RequestedSlots"]);
  CPPUNIT_ASSERT(xmlOut["RequestedApplicationEnvironment"][0] && xmlOut["RequestedApplicationEnvironment"][1] && !xmlOut["RequestedApplicationEnvironment"][2]);
  CPPUNIT_ASSERT_EQUAL((std::string)"ENV/JAVA/JRE-1.6.0", (std::string)xmlOut["RequestedApplicationEnvironment"][0]);
  CPPUNIT_ASSERT_EQUAL((std::string)"APPS/HEP/ATLAS-14.2.23.4", (std::string)xmlOut["RequestedApplicationEnvironment"][1]);
  CPPUNIT_ASSERT_EQUAL((std::string)"input.dat", (std::string)xmlOut["StdIn"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"job.out", (std::string)xmlOut["StdOut"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"err.out", (std::string)xmlOut["StdErr"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"celog", (std::string)xmlOut["LogDir"]);
  CPPUNIT_ASSERT(xmlOut["ExecutionNode"][0] && xmlOut["ExecutionNode"][1] && !xmlOut["ExecutionNode"][2]);
  CPPUNIT_ASSERT_EQUAL((std::string)"wn043", (std::string)xmlOut["ExecutionNode"][0]);
  CPPUNIT_ASSERT_EQUAL((std::string)"wn056", (std::string)xmlOut["ExecutionNode"][1]);
  CPPUNIT_ASSERT_EQUAL((std::string)"pbs-short", (std::string)xmlOut["Queue"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"PT48M13S", (std::string)xmlOut["UsedTotalWallTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"PT3H25M40S", (std::string)xmlOut["UsedTotalCPUTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"4453", (std::string)xmlOut["UsedMainMemory"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-21 12:05:12", (std::string)xmlOut["SubmissionTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20 08:05:12", (std::string)xmlOut["ComputingManagerSubmissionTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20 08:45:12", (std::string)xmlOut["StartTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20 12:05:12", (std::string)xmlOut["ComputingManagerEndTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20 12:15:12", (std::string)xmlOut["EndTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-24 12:05:12", (std::string)xmlOut["WorkingAreaEraseTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-30 12:05:12", (std::string)xmlOut["ProxyExpirationTime"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"pc4.niif.hu:3432", (std::string)xmlOut["SubmissionHost"]);
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid-arc-0.94", (std::string)xmlOut["SubmissionClientName"]);
  CPPUNIT_ASSERT(xmlOut["OtherMessages"][0] && xmlOut["OtherMessages"][1] && !xmlOut["OtherMessages"][2]);
  CPPUNIT_ASSERT_EQUAL((std::string)"Cached input file is outdated; downloading again", (std::string)xmlOut["OtherMessages"][0]);
  CPPUNIT_ASSERT_EQUAL((std::string)"User proxy has expired", (std::string)xmlOut["OtherMessages"][1]);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobTest);
