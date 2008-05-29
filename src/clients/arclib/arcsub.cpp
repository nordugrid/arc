#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/IString.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/client/TargetGenerator.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");


void arcsub(const std::list<std::string>& JobDescriptionFiles,
	    const std::list<std::string>& JobDescriptionStrings,
	    const std::list<std::string>& ClusterSelect,
	    const std::list<std::string>& ClusterReject,
	    const std::list<std::string>& IndexUrls,
	    const std::string& JobListFile,
	    const bool dryrun,
	    const bool dumpdescription,
	    const bool unknownattr,
	    const int timeout) {

  if (JobDescriptionFiles.empty() && JobDescriptionStrings.empty()) {
    std::cout << Arc::IString("No job description input specified")
	      << std::endl;
    return;
  }

  std::list<std::string> JobDescriptionList;

  //Loop over input job description files
  for (std::list<std::string>::const_iterator it = JobDescriptionFiles.begin();
       it != JobDescriptionFiles.end(); it++) {

    std::ifstream descriptionfile(it->c_str());

    if (!descriptionfile)
      std::cout << Arc::IString("Can not open file: %s", *it) << std::endl;

    descriptionfile.seekg(0, std::ios::end);
    std::streamsize length = descriptionfile.tellg();
    descriptionfile.seekg(0, std::ios::beg);

    char *buffer = new char[length + 1];
    descriptionfile.read(buffer, length);
    descriptionfile.close();

    buffer[length] = '\0';
    std::string xrsl(buffer);
    delete[] buffer;

    // Each job description file can contain several jobs, this must be split
    // into several entries in the JobDescription list. Need to be fixed once
    // the JobDescription class is in place

    JobDescriptionList.push_back(xrsl);
  }

  //Loop over job description input strings
  for (std::list<std::string>::const_iterator it =
	 JobDescriptionStrings.begin();
       it != JobDescriptionStrings.end(); it++)

    // Each job description file can contain several jobs, this must be split
    // into several entries in the JobDescription list. Need to be fixed once
    // the JobDescription class is in place

    JobDescriptionList.push_back(*it);

  //prepare targets
  Arc::TargetGenerator TarGen(ClusterSelect, ClusterReject, IndexUrls);
  TarGen.GetTargets(0, 1);

  //store time of information request
  Arc::Time infotime;

  std::map<int, std::string> notsubmitted;

  int jobnr = 1;
  std::list<std::string> jobids;

  if (ClusterSelect.size() == 1 && TarGen.FoundTargets().size() == 0) {
    std::cout << Arc::IString("Job submission failed because "
			      "the specified cluster %s "
			      "did not return any information",
			      *ClusterSelect.begin()) << std::endl;
    return;
  }

  Arc::XMLNode JobIdStorage;
  JobIdStorage.ReadFromFile("jobs.xml");

  for (std::list<std::string>::iterator it = JobDescriptionList.begin();
       it != JobDescriptionList.end(); it++, jobnr++) {

    //if more than 5 minutes has passed, renew target information
    Arc::Time now;
    if (now.GetTime() - infotime.GetTime() > 300) {
      //TarGen.CleanTargets();
      //TarGen.GetTargets(0, 1);
      infotime = now;
    }

    //perform brokering (not yet implemented)
    //broker needs to take JobDescription is input

    //get submitter from chosen target
    //for now use the first executiontarget found

    Arc::Submitter *submitter = TarGen.FoundTargets().begin()->GetSubmitter();

    std::cout << "Submitting jobs ..." << std::endl;
    std::pair<Arc::URL, Arc::URL> jobid;
    jobid = submitter->Submit(*it);

    Arc::XMLNode ThisJob = JobIdStorage.NewChild("job");
    ThisJob.NewChild("id") = jobid.first.str();
    ThisJob.NewChild("name") = "test";
    ThisJob.NewChild("flavour") = TarGen.FoundTargets().begin()->GridFlavour;
    ThisJob.NewChild("source") = jobid.second.str();

    std::cout << "Job submitted with jobid: " << jobid.first.str() << std::endl;
    std::cout << "Information endpoint for this job: " << jobid.second.str()
	      << std::endl;
  }

  JobIdStorage.SaveToFile("jobs.xml");

  if (JobDescriptionList.size() > 1) {
    std::cout << std::endl << Arc::IString("Job submission summary:")
	      << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were submitted",
			      JobDescriptionList.size() - notsubmitted.size(),
			      JobDescriptionList.size()) << std::endl;
    if (notsubmitted.size()) {
      std::cout << Arc::IString("The following %d were not submitted",
				notsubmitted.size()) << std::endl;
      /*
      std::map<int, std::string>::iterator it;
      for (it = notsubmitted.begin(); it != notsubmitted.end(); it++) {
	std::cout << _("Job nr.") << " " << it->first;
	if (it->second.size()>0) std::cout << ": " << it->second;
	std::cout << std::endl;
      }
      */
    }

  }

  return;
}

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[jobid]"), "", istring(
			      "Argument to -g has format:\n"
			      "GRID:URL e.g.\n"
			      "ARC0:ldap://grid.tsl.uu.se:2135/mds-vo-name=sweden,O=grid\n"
			      "CREAM:ldap://cream.grid.upjs.sk:2170/o=grid\n"
			      "\n"
			      "Argument to -c has format:\n"
			      "GRID:URL e.g.\n"
			      "ARC0:ldap://grid.tsl.uu.se:2135/nordugrid-cluster-name=grid.tsl.uu.se,Mds-Vo-name=local,o=grid"));

  std::list<std::string> clustertemp;
  options.AddOption('c', "cluster",
		    istring("explicity select or reject a specific cluster"),
		    istring("[-]name"),
		    clustertemp);

  std::list<std::string> indexurls;
  options.AddOption('i', "indexurl", istring("url to a index server"),
		    istring("url"),
		    indexurls);

  std::list<std::string> jobdescriptionstrings;
  options.AddOption('e', "jobdescrstring",
		    istring("jobdescription string describing the job to be submitted"),
		    istring("string"),
		    jobdescriptionstrings);

  std::list<std::string> jobdescriptionfiles;
  options.AddOption('f', "jobdescrfile",
		    istring("jobdescription file describing the job to be submitted"),
		    istring("string"),
		    jobdescriptionstrings);

  std::string joblist;
  options.AddOption('j', "joblist",
		    istring("file where the jobids will be stored"),
		    istring("filename"),
		    joblist);

  bool dryrun = false;
  options.AddOption('D', "dryrun", istring("add dryrun option"),
		    dryrun);

  bool dumpdescription = false;
  options.AddOption('x', "dumpdescription",
		    istring("do not submit - dump job description"),
		    dumpdescription);

  int timeout = 20;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
		    istring("seconds"), timeout);

  std::string debug;
  options.AddOption('d', "debug",
		    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
		    istring("debuglevel"), debug);

  bool unknownattribute = false;
  options.AddOption('U', "unknownattr",
		    istring("allow unknown attributes in job description"),
		    unknownattribute);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
		    version);

  std::list<std::string> params = options.Parse(argc, argv);

  //sort clustertemp into clusterselect and clusterreject
  std::list<std::string> clusterselect;
  std::list<std::string> clusterreject;
  for (std::list<std::string>::iterator it = clustertemp.begin();
       it != clustertemp.end(); it++)
    if ((*it).find_first_of('-') == 0)
      clusterreject.push_back(*it);
    else
      clusterselect.push_back(*it);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsub", VERSION) << std::endl;
    return 0;
  }

  arcsub(jobdescriptionfiles, jobdescriptionstrings, clusterselect,
	 clusterreject, indexurls, joblist, dryrun, dumpdescription,
	 unknownattribute, timeout);

  return 0;
}
