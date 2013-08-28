#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobInformationStorage.h>
#ifdef DBJSTORE_ENABLED
#include "../JobInformationStorageBDB.h"
#endif

class JobInformationStorageTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobInformationStorageTest);
  CPPUNIT_TEST(XMLGeneralTest);
  CPPUNIT_TEST(XMLReadJobsTest);
#ifdef DBJSTORE_ENABLED
  CPPUNIT_TEST(BDBGeneralTest);
  CPPUNIT_TEST(BDBReadJobsTest);
#endif
  CPPUNIT_TEST_SUITE_END();

public:
  JobInformationStorageTest();
  void setUp() {}
  void tearDown() { remove("jobs.xml"); remove("jobs.bdb"); }
  void GeneralTest(Arc::JobInformationStorage& joblist);
  void ReadJobsTest(Arc::JobInformationStorage& joblist);
  void XMLGeneralTest()  { Arc::JobInformationStorageXML joblist("jobs.xml"); GeneralTest(joblist); }
  void XMLReadJobsTest() { Arc::JobInformationStorageXML joblist("jobs.xml"); ReadJobsTest(joblist); }
#ifdef DBJSTORE_ENABLED
  void BDBGeneralTest()  { Arc::JobInformationStorageBDB joblist("jobs.bdb"); GeneralTest(joblist); }
  void BDBReadJobsTest() { Arc::JobInformationStorageBDB joblist("jobs.bdb"); ReadJobsTest(joblist); }
#endif
  void XMLLockTest();
  
private:
  Arc::XMLNode xmlJob;
};

JobInformationStorageTest::JobInformationStorageTest() : xmlJob(Arc::XMLNode("<ComputingActivity>"
    "<ServiceInformationURL>https://testbed-emi4.grid.upjs.sk:60000/arex</ServiceInformationURL>"
    "<ServiceInformationInterfaceName>org.ogf.glue.emies.resourceinfo</ServiceInformationInterfaceName>"
    "<JobStatusURL>https://testbed-emi4.grid.upjs.sk:60000/arex</JobStatusURL>"
    "<JobStatusInterfaceName>org.ogf.glue.emies.activitymanagement</JobStatusInterfaceName>"
    "<JobManagementURL>https://testbed-emi4.grid.upjs.sk:60000/arex</JobManagementURL>"
    "<JobManagementInterfaceName>org.ogf.glue.emies.activitymanagement</JobManagementInterfaceName>"
    "<JobID>https://testbed-emi4.grid.upjs.sk:60000/arex/HiqNDmAiivgnIfnhppWRvMapABFKDmABFKDmQhJKDmCBFKDmmdhHxm</JobID>"
    "<IDFromEndpoint>HiqNDmAiivgnIfnhppWRvMapABFKDmABFKDmQhJKDmCBFKDmmdhHxm</IDFromEndpoint>"
    "<StageInDir>https://testbed-emi4.grid.upjs.sk:60000/arex</StageInDir>"
    "<StageOutDir>https://testbed-emi4.grid.upjs.sk:60000/arex</StageOutDir>"
    "<SessionDir>https://testbed-emi4.grid.upjs.sk:60000/arex</SessionDir>"
    "<Name>mc08.1050.J7</Name>"
    "<Type>single</Type>"
    "<LocalIDFromManager>345.ce01</LocalIDFromManager>"
    "<JobDescription>nordugrid:xrsl</JobDescription>"
    "<JobDescriptionDocument>&amp;(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")</JobDescriptionDocument>"
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
    "<LocalSubmissionTime>2008-04-21T10:04:36Z</LocalSubmissionTime>"
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
    "<Associations>"
      "<ActivityOldID>https://example-ce.com:443/arex/765234</ActivityOldID>"
      "<ActivityOldID>https://helloworld-ce.com:12345/arex/543678</ActivityOldID>"
      "<LocalInputFile>"
        "<Source>helloworld.sh</Source>"
        "<CheckSum>c0489bec6f7f4454d6cfe1b0a07ad5b8</CheckSum>"
      "</LocalInputFile>"
      "<LocalInputFile>"
        "<Source>random.dat</Source>"
        "<CheckSum>e52b14b10b967d9135c198fd11b9b8bc</CheckSum>"
      "</LocalInputFile>"
    "</Associations>"
  "</ComputingActivity>")) {}

void JobInformationStorageTest::GeneralTest(Arc::JobInformationStorage& jobList) {
  std::list<Arc::Job> inJobs, outJobs;

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job0";
  inJobs.back().JobID = "https://ce00.niif.hu:60000/arex/job0";
  inJobs.back().ServiceInformationURL = Arc::URL("https://info00.niif.hu:2135/aris");
  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job1";
  inJobs.back().JobID = "https://ce01.niif.hu:60000/arex/job1";
  inJobs.back().ServiceInformationURL = Arc::URL("https://info01.niif.hu:2135/aris");
  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job2";
  inJobs.back().JobID = "https://ce01.niif.hu:60000/arex/job2";
  inJobs.back().ServiceInformationURL = Arc::URL("https://info01.niif.hu:2135/aris");
  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job3";
  inJobs.back().JobID = "https://ce01.niif.hu:60000/arex/job3";
  inJobs.back().ServiceInformationURL = Arc::URL("https://info01.niif.hu:2135/aris");
  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Other Job";
  inJobs.back().JobID = "https://ce-other.niif.hu:60000/arex/other-job";
  inJobs.back().ServiceInformationURL = Arc::URL("https://info-other.niif.hu:2135/aris");

  // Write and read jobs.
  CPPUNIT_ASSERT(jobList.Clean());
  CPPUNIT_ASSERT(jobList.Write(inJobs));
  CPPUNIT_ASSERT(jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT_EQUAL(5, (int)outJobs.size());
  {
  std::set<std::string> jobNames;
  jobNames.insert("Job0"); jobNames.insert("Job1"); jobNames.insert("Job2"); jobNames.insert("Job3"); jobNames.insert("Other Job");
  for (std::list<Arc::Job>::const_iterator itJ = outJobs.begin();
       itJ != outJobs.end(); ++itJ) {
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Job with name \"" + itJ->Name + "\" was unexpected" , 1, (int)jobNames.erase(itJ->Name));
  }
  }

  inJobs.clear();
  std::set<std::string> prunedServices;
  prunedServices.insert("info01.niif.hu");
  prunedServices.insert("info02.niif.hu");
  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job4";
  inJobs.back().JobID = "https://ce02.niif.hu:60000/arex/job4";
  inJobs.back().ServiceInformationURL = Arc::URL("https://info02.niif.hu:2135/aris");

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job2";
  inJobs.back().JobID = "https://ce01.niif.hu:60000/arex/job2";
  inJobs.back().ServiceInformationURL = Arc::URL("https://info01.niif.hu:2135/aris");

  // Check that pointers to new jobs are added to the list, and that jobs on services specified to be pruned are removed.
  std::list<const Arc::Job*> newJobs;
  CPPUNIT_ASSERT(jobList.Write(inJobs, prunedServices, newJobs));
  CPPUNIT_ASSERT_EQUAL(1, (int)newJobs.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Job4", newJobs.front()->Name);
  CPPUNIT_ASSERT_EQUAL((std::string)"https://ce02.niif.hu:60000/arex/job4", newJobs.front()->JobID);
  CPPUNIT_ASSERT(jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT_EQUAL(4, (int)outJobs.size());
  {
  std::set<std::string> jobNames;
  jobNames.insert("Job0"); jobNames.insert("Job2"); jobNames.insert("Job4"); jobNames.insert("Other Job");
  for (std::list<Arc::Job>::const_iterator itJ = outJobs.begin();
       itJ != outJobs.end(); ++itJ) {
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Job with name \"" + itJ->Name + "\" was unexpected" , 1, (int)jobNames.erase(itJ->Name));
  }
  }

  // Check whether file is truncated.
  CPPUNIT_ASSERT(jobList.Clean());
  CPPUNIT_ASSERT(jobList.Write(inJobs));
  CPPUNIT_ASSERT(jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT_EQUAL(2, (int)outJobs.size());
  if      ("https://ce02.niif.hu:60000/arex/job4" == outJobs.front().JobID) {
    CPPUNIT_ASSERT_EQUAL((std::string)"Job4", outJobs.front().Name);

    CPPUNIT_ASSERT_EQUAL((std::string)"Job2", outJobs.back().Name);
    CPPUNIT_ASSERT_EQUAL((std::string)"https://ce01.niif.hu:60000/arex/job2", outJobs.back().JobID);
  }
  else if ("https://ce01.niif.hu:60000/arex/job2" == outJobs.front().JobID) {
    CPPUNIT_ASSERT_EQUAL((std::string)"Job2", outJobs.front().Name);

    CPPUNIT_ASSERT_EQUAL((std::string)"Job4", outJobs.back().Name);
    CPPUNIT_ASSERT_EQUAL((std::string)"https://ce02.niif.hu:60000/arex/job4", outJobs.back().JobID);
  }
  else {
    CPPUNIT_FAIL((  "Expected: \"https://ce01.niif.hu:60000/arex/job2\" or \"https://ce02.niif.hu:60000/arex/job4\"\n"
                  "- Actual:   \"" + outJobs.front().JobID + "\"").c_str());
  } 

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job5";
  inJobs.back().JobID = "https://ce01.niif.hu:60000/arex/job5";

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job6";
  inJobs.back().JobID = "https://ce01.niif.hu:60000/arex/job6";

  inJobs.push_back(inJobs.back());
  inJobs.back().Name = "Job6New";

  // Duplicate jobs will be overwritten.
  CPPUNIT_ASSERT(jobList.Clean());
  CPPUNIT_ASSERT(jobList.Write(inJobs));
  CPPUNIT_ASSERT(jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT_EQUAL(4, (int)outJobs.size());
  bool job6NewExists = false;
  for (std::list<Arc::Job>::const_iterator itJob = outJobs.begin();
       itJob != outJobs.end(); ++itJob) {
    CPPUNIT_ASSERT(itJob->Name != "Job6");
    if (itJob->Name == "Job6New") job6NewExists = true;
  }
  CPPUNIT_ASSERT(job6NewExists);

  // Truncate file.
  CPPUNIT_ASSERT(jobList.Clean());
  newJobs.clear();
  CPPUNIT_ASSERT(jobList.Write(inJobs, std::set<std::string>(), newJobs));
  CPPUNIT_ASSERT_EQUAL(4, (int)newJobs.size());
  CPPUNIT_ASSERT(jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT_EQUAL(4, (int)outJobs.size());
  job6NewExists = false;
  for (std::list<Arc::Job>::const_iterator itJob = outJobs.begin();
       itJob != outJobs.end(); ++itJob) {
    CPPUNIT_ASSERT(itJob->Name != "Job6");
    if (itJob->Name == "Job6New") job6NewExists = true;
  }
  CPPUNIT_ASSERT(job6NewExists);

  inJobs.pop_back();

  // Adding more jobs to file.
  CPPUNIT_ASSERT(jobList.Clean());
  CPPUNIT_ASSERT(jobList.Write(inJobs));
  CPPUNIT_ASSERT(jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT_EQUAL(4, (int)outJobs.size());

  std::list<std::string> toberemoved;
  toberemoved.push_back("https://ce02.niif.hu:60000/arex/job4");
  toberemoved.push_back("https://ce01.niif.hu:60000/arex/job5");

  // Check whether jobs are removed correctly.
  CPPUNIT_ASSERT(jobList.Remove(toberemoved));
  CPPUNIT_ASSERT(jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT_EQUAL(2, (int)outJobs.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Job2", outJobs.front().Name);
  CPPUNIT_ASSERT_EQUAL((std::string)"https://ce01.niif.hu:60000/arex/job2", outJobs.front().JobID);
  CPPUNIT_ASSERT_EQUAL((std::string)"Job6", outJobs.back().Name);
  CPPUNIT_ASSERT_EQUAL((std::string)"https://ce01.niif.hu:60000/arex/job6", outJobs.back().JobID);
}

void JobInformationStorageTest::ReadJobsTest(Arc::JobInformationStorage& jobList) {

  std::list<Arc::Job> inJobs, outJobs;

  // Check if jobs are read when specified by the jobIdentifiers argument.
  // Also check that the jobIdentifiers list is modified according to found jobs.
  {
    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-1";
    inJobs.back().JobID = "https://ce.grid.org/1234567890-foo-job-1";
    inJobs.back().IDFromEndpoint = "1234567890-foo-job-1";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-2";
    inJobs.back().JobID = "https://ce.grid.org/1234567890-foo-job-2";
    inJobs.back().IDFromEndpoint = "1234567890-foo-job-2";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-2";
    inJobs.back().JobID = "https://ce.grid.org/0987654321-foo-job-2";
    inJobs.back().IDFromEndpoint = "0987654321-foo-job-2";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-3";
    inJobs.back().JobID = "https://ce.grid.org/1234567890-foo-job-3";
    inJobs.back().IDFromEndpoint = "1234567890-foo-job-3";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    CPPUNIT_ASSERT(jobList.Clean());
    CPPUNIT_ASSERT(jobList.Write(inJobs));

    std::list<std::string> jobIdentifiers;
    jobIdentifiers.push_back("https://ce.grid.org/1234567890-foo-job-1");
    // Having the same identifier twice should only result in one Job object being added to the list.
    jobIdentifiers.push_back("https://ce.grid.org/1234567890-foo-job-1");
    jobIdentifiers.push_back("foo-job-2");
    jobIdentifiers.push_back("nonexistent-job");
    
    CPPUNIT_ASSERT(jobList.Read(outJobs, jobIdentifiers));
    CPPUNIT_ASSERT_EQUAL(3, (int)outJobs.size());
    std::list<Arc::Job>::const_iterator itJ = outJobs.begin();
    CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-1", itJ->Name);
    CPPUNIT_ASSERT_EQUAL((std::string)"https://ce.grid.org/1234567890-foo-job-1", itJ->JobID);
    ++itJ;
    CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-2", itJ->Name);
    if      ("https://ce.grid.org/1234567890-foo-job-2" == itJ->JobID) {
      ++itJ;
      CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-2", itJ->Name);
      CPPUNIT_ASSERT_EQUAL((std::string)"https://ce.grid.org/0987654321-foo-job-2", itJ->JobID);
    }
    else if ("https://ce.grid.org/0987654321-foo-job-2" == itJ->JobID) {
      ++itJ;
      CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-2", itJ->Name);
      CPPUNIT_ASSERT_EQUAL((std::string)"https://ce.grid.org/1234567890-foo-job-2", itJ->JobID);
    }
    else {
      CPPUNIT_FAIL((  "Expected: \"https://ce.grid.org/1234567890-foo-job-2\" or \"https://ce.grid.org/0987654321-foo-job-2\"\n"
                    "- Actual:   \"" + itJ->JobID + "\"").c_str());
    }

    CPPUNIT_ASSERT_EQUAL(1, (int)jobIdentifiers.size());
    CPPUNIT_ASSERT_EQUAL((std::string)"nonexistent-job", jobIdentifiers.front());
  }

  // Check if jobs are read when specified by the endpoints argument.
  // Also check if jobs are read when specified by the rejectEndpoints argument.
  {
    inJobs.clear();

    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-1";
    inJobs.back().JobID = "https://ce1.grid.org/1234567890-foo-job-1";
    inJobs.back().IDFromEndpoint = "1234567890-foo-job-1";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce1.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce1.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce1.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-2";
    inJobs.back().JobID = "https://ce2.grid.org/1234567890-foo-job-2";
    inJobs.back().IDFromEndpoint = "1234567890-foo-job-2";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce2.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce2.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce2.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-3";
    inJobs.back().JobID = "https://ce2.grid.org/1234567890-foo-job-3";
    inJobs.back().IDFromEndpoint = "1234567890-foo-job-3";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce2.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce2.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce2.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    inJobs.push_back(Arc::Job());
    inJobs.back().Name = "foo-job-4";
    inJobs.back().JobID = "https://ce3.grid.org/1234567890-foo-job-4";
    inJobs.back().IDFromEndpoint = "1234567890-foo-job-4";
    inJobs.back().ServiceInformationURL = Arc::URL("https://ce3.grid.org/");
    inJobs.back().ServiceInformationInterfaceName = "org.nordugrid.test";
    inJobs.back().JobStatusURL = Arc::URL("https://ce3.grid.org/");
    inJobs.back().JobStatusInterfaceName = "org.nordugrid.test";
    inJobs.back().JobManagementURL = Arc::URL("https://ce3.grid.org/");
    inJobs.back().JobManagementInterfaceName = "org.nordugrid.test";

    CPPUNIT_ASSERT(jobList.Clean());
    CPPUNIT_ASSERT(jobList.Write(inJobs));

    std::list<std::string> jobIdentifiers, endpoints, rejectEndpoints;
    endpoints.push_back("ce2.grid.org");

    CPPUNIT_ASSERT(jobList.Read(outJobs, jobIdentifiers, endpoints));
    CPPUNIT_ASSERT_EQUAL(2, (int)outJobs.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-2", outJobs.front().Name);
    CPPUNIT_ASSERT_EQUAL((std::string)"https://ce2.grid.org/1234567890-foo-job-2", outJobs.front().JobID);
    CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-3", outJobs.back().Name);
    CPPUNIT_ASSERT_EQUAL((std::string)"https://ce2.grid.org/1234567890-foo-job-3", outJobs.back().JobID);

    outJobs.clear();
    rejectEndpoints.push_back("ce2.grid.org");

    CPPUNIT_ASSERT(jobList.ReadAll(outJobs, rejectEndpoints));
    CPPUNIT_ASSERT_EQUAL(2, (int)outJobs.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-1", outJobs.front().Name);
    CPPUNIT_ASSERT_EQUAL((std::string)"https://ce1.grid.org/1234567890-foo-job-1", outJobs.front().JobID);
    CPPUNIT_ASSERT_EQUAL((std::string)"foo-job-4", outJobs.back().Name);
    CPPUNIT_ASSERT_EQUAL((std::string)"https://ce3.grid.org/1234567890-foo-job-4", outJobs.back().JobID);
  }
}

void JobInformationStorageTest::XMLLockTest() {
  // Check whether lock is respected.
  Arc::JobInformationStorageXML jobList("jobs.xml");
  std::list<Arc::Job> inJobs, outJobs;
  std::list<std::string> toberemoved;
  Arc::FileLock lock(jobList.GetName());
  CPPUNIT_ASSERT(lock.acquire());
  CPPUNIT_ASSERT(!jobList.Write(inJobs));
  CPPUNIT_ASSERT(!jobList.ReadAll(outJobs));
  CPPUNIT_ASSERT(!jobList.Remove(toberemoved));
  CPPUNIT_ASSERT(!jobList.Clean());
  CPPUNIT_ASSERT(lock.release());
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobInformationStorageTest);
