#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/Job.h>

class JobTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobTest);
  CPPUNIT_TEST(XMLToJobTest);
  CPPUNIT_TEST(JobToXMLTest);
  CPPUNIT_TEST(XMLToJobStateTest);
  CPPUNIT_TEST(FromOldFormatTest);
  CPPUNIT_TEST(FileTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobTest();
  void setUp() {}
  void tearDown() {}
  void XMLToJobTest();
  void JobToXMLTest();
  void XMLToJobStateTest();
  void FromOldFormatTest();
  void FileTest();

private:
  Arc::XMLNode xmlJob;
};

JobTest::JobTest() : xmlJob(Arc::XMLNode("<ComputingActivity>"
    "<Name>mc08.1050.J7</Name>"
    "<Flavour>ARC1</Flavour>"
    "<Cluster>https://ce01.niif.hu:60000/arex/</Cluster>"
    "<InfoEndpoint>https://ce01.niif.hu:60000/arex/</InfoEndpoint>"
    "<ISB>https://isb.niif.hu:60000/arex/</ISB>"
    "<OSB>https://osb.niif.hu:60000/arex/</OSB>"
    "<AuxInfo>UNICORE magic</AuxInfo>"
    "<Type>single</Type>"
    "<IDFromEndpoint>https://ce01.niif.hu:60000/arex/123456</IDFromEndpoint>"
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

void JobTest::XMLToJobTest() {
  Arc::Job job;
  job = xmlJob;

  CPPUNIT_ASSERT_EQUAL((std::string)"mc08.1050.J7", job.Name);
  CPPUNIT_ASSERT_EQUAL((std::string)"ARC1", job.Flavour);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://ce01.niif.hu:60000/arex/"), job.Cluster);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://ce01.niif.hu:60000/arex/"), job.InfoEndpoint);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://isb.niif.hu:60000/arex/"), job.ISB);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://osb.niif.hu:60000/arex/"), job.OSB);
  CPPUNIT_ASSERT_EQUAL((std::string)"UNICORE magic", job.AuxInfo);
  CPPUNIT_ASSERT_EQUAL((std::string)"single", job.Type);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://ce01.niif.hu:60000/arex/123456"), job.IDFromEndpoint);
  CPPUNIT_ASSERT_EQUAL((std::string)"345.ce01", job.LocalIDFromManager);
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid:xrsl", job.JobDescription);
  CPPUNIT_ASSERT_EQUAL((std::string)"&(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")", job.JobDescriptionDocument);
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
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2008-04-21T10:04:36Z"), job.LocalSubmissionTime);
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
  CPPUNIT_ASSERT_EQUAL(2, (int)job.ActivityOldID.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://example-ce.com:443/arex/765234", job.ActivityOldID.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://helloworld-ce.com:12345/arex/543678", job.ActivityOldID.back());
  CPPUNIT_ASSERT_EQUAL(2, (int)job.LocalInputFiles.size());
  std::map<std::string, std::string>::const_iterator itFiles = job.LocalInputFiles.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)"helloworld.sh", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"c0489bec6f7f4454d6cfe1b0a07ad5b8", itFiles->second);
  itFiles++;
  CPPUNIT_ASSERT_EQUAL((std::string)"random.dat", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"e52b14b10b967d9135c198fd11b9b8bc", itFiles->second);
}

void JobTest::JobToXMLTest() {
  Arc::Job job;
  job = xmlJob;
  Arc::XMLNode xmlOut("<ComputingActivity/>");

  job.ToXML(xmlOut);

  CPPUNIT_ASSERT_EQUAL((std::string)"mc08.1050.J7", (std::string)xmlOut["Name"]); xmlOut["Name"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"ARC1", (std::string)xmlOut["Flavour"]); xmlOut["Flavour"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://ce01.niif.hu:60000/arex/", (std::string)xmlOut["Cluster"]); xmlOut["Cluster"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://ce01.niif.hu:60000/arex/", (std::string)xmlOut["InfoEndpoint"]); xmlOut["InfoEndpoint"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://isb.niif.hu:60000/arex/", (std::string)xmlOut["ISB"]); xmlOut["ISB"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://osb.niif.hu:60000/arex/", (std::string)xmlOut["OSB"]); xmlOut["OSB"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"UNICORE magic", (std::string)xmlOut["AuxInfo"]); xmlOut["AuxInfo"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"single", (std::string)xmlOut["Type"]); xmlOut["Type"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://ce01.niif.hu:60000/arex/123456", (std::string)xmlOut["IDFromEndpoint"]); xmlOut["IDFromEndpoint"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"345.ce01", (std::string)xmlOut["LocalIDFromManager"]); xmlOut["LocalIDFromManager"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid:xrsl", (std::string)xmlOut["JobDescription"]); xmlOut["JobDescription"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"&(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")", (std::string)xmlOut["JobDescriptionDocument"]); xmlOut["JobDescriptionDocument"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Other", (std::string)xmlOut["State"]["General"]); xmlOut["State"]["General"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:failed", (std::string)xmlOut["State"]["Specific"]); xmlOut["State"]["Specific"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["State"].Size()); xmlOut["State"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Other", (std::string)xmlOut["RestartState"]["General"]); xmlOut["RestartState"]["General"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"bes:running", (std::string)xmlOut["RestartState"]["Specific"]); xmlOut["RestartState"]["Specific"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["RestartState"].Size()); xmlOut["RestartState"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["ExitCode"]); xmlOut["ExitCode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["ComputingManagerExitCode"]); xmlOut["ComputingManagerExitCode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Uploading timed out", (std::string)xmlOut["Error"]); xmlOut["Error"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Failed stage-out", (std::string)xmlOut["Error"]); xmlOut["Error"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"0", (std::string)xmlOut["WaitingPosition"]); xmlOut["WaitingPosition"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"vo:atlas", (std::string)xmlOut["UserDomain"]); xmlOut["UserDomain"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"CONFIDENTIAL", (std::string)xmlOut["Owner"]); xmlOut["Owner"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"grid02", (std::string)xmlOut["LocalOwner"]); xmlOut["LocalOwner"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT1H23M20S", (std::string)xmlOut["RequestedTotalWallTime"]); xmlOut["RequestedTotalWallTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT5H33M20S", (std::string)xmlOut["RequestedTotalCPUTime"]); xmlOut["RequestedTotalCPUTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"4", (std::string)xmlOut["RequestedSlots"]); xmlOut["RequestedSlots"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"ENV/JAVA/JRE-1.6.0", (std::string)xmlOut["RequestedApplicationEnvironment"]); xmlOut["RequestedApplicationEnvironment"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"APPS/HEP/ATLAS-14.2.23.4", (std::string)xmlOut["RequestedApplicationEnvironment"]); xmlOut["RequestedApplicationEnvironment"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"input.dat", (std::string)xmlOut["StdIn"]); xmlOut["StdIn"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"job.out", (std::string)xmlOut["StdOut"]); xmlOut["StdOut"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"err.out", (std::string)xmlOut["StdErr"]); xmlOut["StdErr"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"celog", (std::string)xmlOut["LogDir"]); xmlOut["LogDir"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"wn043", (std::string)xmlOut["ExecutionNode"]); xmlOut["ExecutionNode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"wn056", (std::string)xmlOut["ExecutionNode"]); xmlOut["ExecutionNode"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"pbs-short", (std::string)xmlOut["Queue"]); xmlOut["Queue"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT48M13S", (std::string)xmlOut["UsedTotalWallTime"]); xmlOut["UsedTotalWallTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"PT3H25M40S", (std::string)xmlOut["UsedTotalCPUTime"]); xmlOut["UsedTotalCPUTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"4453", (std::string)xmlOut["UsedMainMemory"]); xmlOut["UsedMainMemory"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-21T10:04:36Z", (std::string)xmlOut["LocalSubmissionTime"]); xmlOut["LocalSubmissionTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-21T10:05:12Z", (std::string)xmlOut["SubmissionTime"]); xmlOut["SubmissionTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T06:05:12Z", (std::string)xmlOut["ComputingManagerSubmissionTime"]); xmlOut["ComputingManagerSubmissionTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T06:45:12Z", (std::string)xmlOut["StartTime"]); xmlOut["StartTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T10:05:12Z", (std::string)xmlOut["ComputingManagerEndTime"]); xmlOut["ComputingManagerEndTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-20T10:15:12Z", (std::string)xmlOut["EndTime"]); xmlOut["EndTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-24T10:05:12Z", (std::string)xmlOut["WorkingAreaEraseTime"]); xmlOut["WorkingAreaEraseTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"2008-04-30T10:05:12Z", (std::string)xmlOut["ProxyExpirationTime"]); xmlOut["ProxyExpirationTime"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"pc4.niif.hu:3432", (std::string)xmlOut["SubmissionHost"]); xmlOut["SubmissionHost"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"nordugrid-arc-0.94", (std::string)xmlOut["SubmissionClientName"]); xmlOut["SubmissionClientName"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"Cached input file is outdated; downloading again", (std::string)xmlOut["OtherMessages"]); xmlOut["OtherMessages"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"User proxy has expired", (std::string)xmlOut["OtherMessages"]); xmlOut["OtherMessages"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://example-ce.com:443/arex/765234", (std::string)xmlOut["Associations"]["ActivityOldID"]); xmlOut["Associations"]["ActivityOldID"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"https://helloworld-ce.com:12345/arex/543678", (std::string)xmlOut["Associations"]["ActivityOldID"]); xmlOut["Associations"]["ActivityOldID"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"helloworld.sh", (std::string)xmlOut["Associations"]["LocalInputFile"]["Source"]); xmlOut["Associations"]["LocalInputFile"]["Source"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"c0489bec6f7f4454d6cfe1b0a07ad5b8", (std::string)xmlOut["Associations"]["LocalInputFile"]["CheckSum"]); xmlOut["Associations"]["LocalInputFile"]["CheckSum"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["Associations"]["LocalInputFile"].Size()); xmlOut["Associations"]["LocalInputFile"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"random.dat", (std::string)xmlOut["Associations"]["LocalInputFile"]["Source"]); xmlOut["Associations"]["LocalInputFile"]["Source"].Destroy();
  CPPUNIT_ASSERT_EQUAL((std::string)"e52b14b10b967d9135c198fd11b9b8bc", (std::string)xmlOut["Associations"]["LocalInputFile"]["CheckSum"]); xmlOut["Associations"]["LocalInputFile"]["CheckSum"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["Associations"]["LocalInputFile"].Size()); xmlOut["Associations"]["LocalInputFile"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut["Associations"].Size()); xmlOut["Associations"].Destroy();
  CPPUNIT_ASSERT_EQUAL(0, xmlOut.Size());

  Arc::Job emptyJob;
  emptyJob.ToXML(xmlOut);
  CPPUNIT_ASSERT_EQUAL(0, xmlOut.Size());
}

void JobTest::XMLToJobStateTest() {
  Arc::XMLNode xml(
  "<ComputingActivity>"
    "<State>"
      "<General>Preparing</General>"
      "<Specific>PREPARING</Specific>"
    "</State>"
  "</ComputingActivity>"
  );
  Arc::Job job;
  job = xml;
  CPPUNIT_ASSERT_EQUAL((std::string)"PREPARING", job.State());
  CPPUNIT_ASSERT_EQUAL((std::string)"Preparing", job.State.GetGeneralState());
}

void JobTest::FromOldFormatTest() {
  Arc::XMLNode xml(
  "<ComputingActivity>"
    "<JobID>https://example-ce.com:443/arex/3456789101112</JobID>"
    "<Flavour>ARC1</Flavour>"
    "<Cluster>https://example-ce.com:443/arex</Cluster>"
    "<InfoEndpoint>https://example-ce.com:443/arex/3456789101112</InfoEndpoint>"
    "<LocalSubmissionTime>2010-09-24 16:17:46</LocalSubmissionTime>"
    "<JobDescription>&amp;(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")</JobDescription>"
    "<OldJobID>https://example-ce.com:443/arex/765234</OldJobID>"
    "<OldJobID>https://helloworld-ce.com:12345/arex/543678</OldJobID>"
    "<LocalInputFiles>"
      "<File>"
        "<Source>helloworld.sh</Source>"
        "<CheckSum>c0489bec6f7f4454d6cfe1b0a07ad5b8</CheckSum>"
      "</File>"
      "<File>"
        "<Source>random.dat</Source>"
        "<CheckSum>e52b14b10b967d9135c198fd11b9b8bc</CheckSum>"
      "</File>"
    "</LocalInputFiles>"
  "</ComputingActivity>"
  );
  Arc::Job job;
  job = xml;

  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://example-ce.com:443/arex/3456789101112"), job.JobID);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://example-ce.com:443/arex/3456789101112"), job.IDFromEndpoint);
  CPPUNIT_ASSERT_EQUAL((std::string)"ARC1", job.Flavour);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://example-ce.com:443/arex"), job.Cluster);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://example-ce.com:443/arex/3456789101112"), job.InfoEndpoint);
  CPPUNIT_ASSERT_EQUAL(Arc::Time("2010-09-24 16:17:46"), job.LocalSubmissionTime);
  CPPUNIT_ASSERT_EQUAL((std::string)"&(executable=\"helloworld.sh\")(arguments=\"random.dat\")(inputfiles=(\"helloworld.sh\")(\"random.dat\"))(stdout=\"helloworld.out\")(join=\"yes\")", job.JobDescriptionDocument);

  CPPUNIT_ASSERT_EQUAL(2, (int)job.ActivityOldID.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://example-ce.com:443/arex/765234", job.ActivityOldID.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://helloworld-ce.com:12345/arex/543678", job.ActivityOldID.back());

  CPPUNIT_ASSERT_EQUAL(2, (int)job.LocalInputFiles.size());
  std::map<std::string, std::string>::const_iterator itFiles = job.LocalInputFiles.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)"helloworld.sh", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"c0489bec6f7f4454d6cfe1b0a07ad5b8", itFiles->second);
  itFiles++;
  CPPUNIT_ASSERT_EQUAL((std::string)"random.dat", itFiles->first);
  CPPUNIT_ASSERT_EQUAL((std::string)"e52b14b10b967d9135c198fd11b9b8bc", itFiles->second);
}

void JobTest::FileTest() {
  const std::string jobfile = "jobs.xml";
  std::list<Arc::Job> inJobs, outJobs;

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job1";
  inJobs.back().IDFromEndpoint.ChangePath("/arex/job1");
  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job2";
  inJobs.back().IDFromEndpoint.ChangePath("/arex/job2");

  // Write and read jobs.
  CPPUNIT_ASSERT(Arc::Job::WriteJobsToTruncatedFile(jobfile, inJobs));
  CPPUNIT_ASSERT(Arc::Job::ReadAllJobsFromFile(jobfile, outJobs));
  CPPUNIT_ASSERT_EQUAL(inJobs.size(), outJobs.size());
  CPPUNIT_ASSERT_EQUAL(inJobs.front().Name, outJobs.front().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().Name, outJobs.back().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.front().IDFromEndpoint, outJobs.front().IDFromEndpoint);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().IDFromEndpoint, outJobs.back().IDFromEndpoint);

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job3";
  inJobs.back().IDFromEndpoint.ChangePath("/arex/job3");

  // Check that pointers to new jobs are added to the list
  std::list<const Arc::Job*> newJobs;
  CPPUNIT_ASSERT(Arc::Job::WriteJobsToFile(jobfile, inJobs, newJobs));
  CPPUNIT_ASSERT_EQUAL(1, (int)newJobs.size());
  CPPUNIT_ASSERT_EQUAL(inJobs.back().Name, newJobs.front()->Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().IDFromEndpoint, newJobs.front()->IDFromEndpoint);
  CPPUNIT_ASSERT(Arc::Job::ReadAllJobsFromFile(jobfile, outJobs));
  CPPUNIT_ASSERT_EQUAL(inJobs.size(), outJobs.size());
  CPPUNIT_ASSERT_EQUAL(inJobs.front().Name, outJobs.front().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().Name, outJobs.back().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.front().IDFromEndpoint, outJobs.front().IDFromEndpoint);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().IDFromEndpoint, outJobs.back().IDFromEndpoint);

  // Check whether file is truncated.
  inJobs.pop_front();
  CPPUNIT_ASSERT(Arc::Job::WriteJobsToTruncatedFile(jobfile, inJobs));
  CPPUNIT_ASSERT(Arc::Job::ReadAllJobsFromFile(jobfile, outJobs));
  CPPUNIT_ASSERT_EQUAL(inJobs.size(), outJobs.size());
  CPPUNIT_ASSERT_EQUAL(inJobs.front().Name, outJobs.front().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().Name, outJobs.back().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.front().IDFromEndpoint, outJobs.front().IDFromEndpoint);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().IDFromEndpoint, outJobs.back().IDFromEndpoint);

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job4";
  inJobs.back().IDFromEndpoint.ChangePath("/arex/job4");

  inJobs.push_back(xmlJob);
  inJobs.back().Name = "Job5";
  inJobs.back().IDFromEndpoint.ChangePath("/arex/job5");

  inJobs.push_back(inJobs.back());

  // Identical jobs in job list is not allowed.
  CPPUNIT_ASSERT(!Arc::Job::WriteJobsToTruncatedFile(jobfile, inJobs));
  CPPUNIT_ASSERT(!Arc::Job::WriteJobsToFile(jobfile, inJobs, newJobs));
  inJobs.pop_back();

  // Adding more jobs to file.
  CPPUNIT_ASSERT(Arc::Job::WriteJobsToTruncatedFile(jobfile, inJobs));
  CPPUNIT_ASSERT(Arc::Job::ReadAllJobsFromFile(jobfile, outJobs));
  CPPUNIT_ASSERT_EQUAL(inJobs.size(), outJobs.size());
  CPPUNIT_ASSERT_EQUAL(inJobs.front().Name, outJobs.front().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().Name, outJobs.back().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.front().IDFromEndpoint, outJobs.front().IDFromEndpoint);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().IDFromEndpoint, outJobs.back().IDFromEndpoint);

  std::list<Arc::URL> toberemoved;
  toberemoved.push_back(inJobs.back().IDFromEndpoint);
  toberemoved.back().ChangePath("/arex/job3");
  toberemoved.push_back(inJobs.back().IDFromEndpoint);
  toberemoved.back().ChangePath("/arex/job4");

  // Check whether jobs are removed correctly.
  CPPUNIT_ASSERT(Arc::Job::RemoveJobsFromFile(jobfile, toberemoved));
  CPPUNIT_ASSERT(Arc::Job::ReadAllJobsFromFile(jobfile, outJobs));
  CPPUNIT_ASSERT_EQUAL(2, (int)outJobs.size());
  CPPUNIT_ASSERT_EQUAL(inJobs.front().Name, outJobs.front().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().Name, outJobs.back().Name);
  CPPUNIT_ASSERT_EQUAL(inJobs.front().IDFromEndpoint, outJobs.front().IDFromEndpoint);
  CPPUNIT_ASSERT_EQUAL(inJobs.back().IDFromEndpoint, outJobs.back().IDFromEndpoint);

  // Check whether lock is respected.
  Arc::FileLock lock(jobfile);
  CPPUNIT_ASSERT(lock.acquire());
  CPPUNIT_ASSERT(!Arc::Job::WriteJobsToTruncatedFile(jobfile, inJobs));
  CPPUNIT_ASSERT(!Arc::Job::ReadAllJobsFromFile(jobfile, outJobs));
  CPPUNIT_ASSERT(!Arc::Job::RemoveJobsFromFile(jobfile, toberemoved));
  CPPUNIT_ASSERT(lock.release());

  remove(jobfile.c_str());
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobTest);
