// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>

#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include <arc/OptionParser.h>
#include <arc/Logger.h>
#include <arc/compute/Job.h>
#include "JobInformationStorageXML.h"


int main(int argc, char **argv) {

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::OptionParser options("", "", "");

  int nJobs = -1;
  options.AddOption('N', "NJobs", "number of jobs to write/read to/from storage", "n", nJobs);
  
  int bunchSize = 50000;
  options.AddOption('B', "bunchSize", "size of bunches of job objects to pass to JobInformationStorage object methods", "n", bunchSize);

  std::string action = "write";
  options.AddOption('a', "action", "Action to perform: write, append, appendreturnnew, read, readall, remove", "action", action);
  
  std::string filename = "";
  options.AddOption('f', "filename", "", "", filename);
  
  std::string typeS = "";
  options.AddOption('t', "type", "Type of storage back-end to use (XML)", "type", typeS);
  
  std::string hostname = "test.nordugrid.org";
  options.AddOption(0, "hostname", "", "", hostname);

  std::list<std::string> endpoints;
  options.AddOption(0, "endpoint", "", "", endpoints);

  std::list<std::string> rejectEndpoints;
  options.AddOption(0, "rejectEndpoint", "Reject jobs with JobManagementURL matching specified endpoints (matching algorithm: URL::StringMatches)", "reject", rejectEndpoints);

  std::string jobidinfile;
  options.AddOption('i', "jobids-from-file", "a file containing a list of job IDs", "filename", jobidinfile);

  std::string jobidoutfile;
  options.AddOption('o', "jobids-to-file", "the IDs of jobs will be appended to this file", "filename", jobidoutfile);

  std::string debug;
  options.AddOption('d', "debug",
                    "FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG",
                    "debuglevel", debug);

  options.Parse(argc, argv);

  if (!debug.empty()) Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(debug));

  if (filename.empty()) {
    std::cerr << "ERROR: No filename specified." << std::endl;
    return 1;
  }

  Arc::JobInformationStorage** jisPointer = NULL;
  if      (typeS == "XML") {
    Arc::JobInformationStorageXML *jisXML = new Arc::JobInformationStorageXML(filename);
    jisPointer = (Arc::JobInformationStorage**)&jisXML;
  }
  else {
    std::cerr << "ERROR: Unable to determine storage back-end to use." << std::endl;
    return 1;
  }

  Arc::JobInformationStorage& jis = **jisPointer;
  
  Arc::Period timing;
  if      (action == "write" || action == "append") {
    if (nJobs <= 0) {
      std::cerr << "ERROR: Invalid number of jobs specified (nJobs = " << nJobs << ")" << std::endl;
      goto error;
    }
    if (bunchSize <= 0) {
      std::cerr << "ERROR: Invalid bunch size (bunchSize = " << bunchSize << ")" << std::endl;
      goto error;
    }
    
    if (action == "write") {
      remove(filename.c_str());
    }
    else {
      Arc::FileCopy(filename, filename + ".orig");
    }
    Arc::Job j;
    j.ServiceInformationInterfaceName = "org.nordugrid.test";
    j.JobStatusInterfaceName = "org.nordugrid.test";
    j.JobManagementInterfaceName = "org.nordugrid.test";
    j.JobDescriptionDocument = "&( executable = \"/bin/echo\" )( arguments = \"Hello World\" )( stdout = \"std.out\" )( stderr = \"std.out\" )( cputime = \"PT1M\" )( outputfiles = ( \"std.out\" \"\" ) ( \"std.out\" \"\" ) )( queue = \"gridlong\" )( jobname = \"Hello World\" )( clientsoftware = \"libarccompute-trunk\" )( clientxrsl = \"&( executable = \"\"/bin/echo\"\" )( arguments = \"\"Hello World\"\" )( stdout = \"\"std.out\"\" )( join = \"\"yes\"\" )( cputime = \"\"1\"\" )( jobname = \"\"Hello World\"\" )\" )( hostname = \"x220-skou\" )( savestate = \"yes\" )";
  
    srand(Arc::Time().GetTimeNanoseconds());
  
    for (int m = 0; m <= nJobs/bunchSize; ++m) {
      std::list<Arc::Job> jobs;
      const int bunchEnd = (m != nJobs/bunchSize)*(m+1)*bunchSize + (m == nJobs/bunchSize)*(nJobs%bunchSize);
      for (int n = m*bunchSize; n < bunchEnd; ++n) {
        j.Name = "Job " + Arc::tostring(n);
        j.IDFromEndpoint = Arc::tostring(rand())+Arc::tostring(n)+Arc::tostring(rand());
        j.JobID = "http://" + hostname + "/" + j.IDFromEndpoint;
        j.ServiceInformationURL = Arc::URL("http://" + hostname + "/serviceinfo");
        j.JobStatusURL = Arc::URL("http://" + hostname + "/jobstatus");
        j.JobManagementURL = Arc::URL("http://" + hostname + "/jobmanagement");
        j.StageInDir = Arc::URL("http://" + hostname + "/stagein/" + j.IDFromEndpoint);
        j.StageOutDir = Arc::URL("http://" + hostname + "/stageout/" + j.IDFromEndpoint);
        j.SessionDir = Arc::URL("http://" + hostname + "/session/" + j.IDFromEndpoint);
        j.LocalSubmissionTime = Arc::Time();
        jobs.push_back(j);
      }
      
      if (!jobidoutfile.empty()) {
        Arc::Job::WriteJobIDsToFile(jobs, jobidoutfile);
      }
      
      Arc::Time tBefore;
      jis.Write(jobs);
      timing += Arc::Time()-tBefore;
    }
  }
  else if (action == "appendreturnnew") {
    if (nJobs <= 0) {
      std::cerr << "ERROR: Invalid number of jobs specified (nJobs = " << nJobs << ")" << std::endl;
      goto error;
    }
    if (bunchSize <= 0) {
      std::cerr << "ERROR: Invalid bunch size (bunchSize = " << bunchSize << ")" << std::endl;
      goto error;
    }
    Arc::FileCopy(filename, "append-" + filename);
    filename = "append-" + filename;
    
    Arc::Job j;
    j.ServiceInformationInterfaceName = "org.nordugrid.test";
    j.JobStatusInterfaceName = "org.nordugrid.test";
    j.JobManagementInterfaceName = "org.nordugrid.test";
    j.JobDescriptionDocument = "&( executable = \"/bin/echo\" )( arguments = \"Hello World\" )( stdout = \"std.out\" )( stderr = \"std.out\" )( cputime = \"PT1M\" )( outputfiles = ( \"std.out\" \"\" ) ( \"std.out\" \"\" ) )( queue = \"gridlong\" )( jobname = \"Hello World\" )( clientsoftware = \"libarccompute-trunk\" )( clientxrsl = \"&( executable = \"\"/bin/echo\"\" )( arguments = \"\"Hello World\"\" )( stdout = \"\"std.out\"\" )( join = \"\"yes\"\" )( cputime = \"\"1\"\" )( jobname = \"\"Hello World\"\" )\" )( hostname = \"x220-skou\" )( savestate = \"yes\" )";

    std::list<std::string> identifiers;
    Arc::Job::ReadJobIDsFromFile(jobidinfile, identifiers);
    for (int m = 0; m <= nJobs/bunchSize; ++m) {
      std::list<Arc::Job> jobs;
      std::set<std::string> prunedServices;
      const int bunchEnd = (m != nJobs/bunchSize)*(m+1)*bunchSize + (m == nJobs/bunchSize)*(nJobs%bunchSize);
      for (int n = m*bunchSize; n < bunchEnd; ++n) {
        std::string jobHostName = hostname;
        if (!identifiers.empty()) {
          Arc::URL temp(identifiers.front());
          identifiers.pop_front();
          jobHostName = temp.Host();
          j.IDFromEndpoint = temp.Path();
        }
        else {
          j.IDFromEndpoint = Arc::tostring(rand())+Arc::tostring(rand());
        }
        prunedServices.insert(jobHostName);
        j.Name = "Job " + Arc::tostring(n);
        j.JobID = "http://" +  jobHostName + "/" + j.IDFromEndpoint;
        j.ServiceInformationURL = Arc::URL("http://" + jobHostName + "/serviceinfo");
        j.JobStatusURL = Arc::URL("http://" + jobHostName + "/jobstatus");
        j.JobManagementURL = Arc::URL("http://" + jobHostName + "/jobmanagement");
        j.StageInDir = Arc::URL("http://" + jobHostName + "/stagein/" + j.IDFromEndpoint);
        j.StageOutDir = Arc::URL("http://" + jobHostName + "/stageout/" + j.IDFromEndpoint);
        j.SessionDir = Arc::URL("http://" + jobHostName + "/session/" + j.IDFromEndpoint);
        j.LocalSubmissionTime = Arc::Time();
        jobs.push_back(j);
      }
      
      std::list<const Arc::Job*> newJobs;
      Arc::Time tBefore;
      jis.Write(jobs, prunedServices, newJobs);
      timing += Arc::Time()-tBefore;
    }
  }
  else if (action == "readall") {
    std::list<Arc::Job> jobs;
    
    Arc::Time tBefore;
    jis.ReadAll(jobs, rejectEndpoints);
    timing += Arc::Time()-tBefore;
  }
  else if (action == "read") {
    std::list<Arc::Job> jobs;
    std::list<std::string> identifiers;
    Arc::Job::ReadJobIDsFromFile(jobidinfile, identifiers);
    Arc::Time tBefore;
    jis.Read(jobs, identifiers, endpoints, rejectEndpoints);
    timing += Arc::Time()-tBefore;
  }
  else if (action == "remove") {
    std::list<std::string> identifiers;
    Arc::Job::ReadJobIDsFromFile(jobidinfile, identifiers);
    Arc::Time tBefore;
    jis.Remove(identifiers);
    timing += Arc::Time()-tBefore;
  }
  else {
    std::cerr << "ERROR: Invalid action specified (action = \"" << action << "\")" << std::endl;
    delete *jisPointer;
    return 1;
  }
  delete *jisPointer;
  
  {
  int nanosecs = timing.GetPeriodNanoseconds();
  std::string zerosToPrefix = "";
  for (int i = 100000000; i > 1; i /= 10) {
    if (nanosecs / i == 0) zerosToPrefix += "0";
    else break;
  }
  std::cout << timing.GetPeriod() << "." << zerosToPrefix << nanosecs/1000 << std::endl;
  }
 return 0;
 
error:
  delete *jisPointer;
  return 1;
}
