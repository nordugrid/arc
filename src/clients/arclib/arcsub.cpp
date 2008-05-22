#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <sstream>

#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/IString.h>
#include <arc/DateTime.h>
#include <arc/ArcConfig.h>
#include <arc/StringConv.h>
#include <arc/client/Submitter.h>
#include <arc/misc/ClientInterface.h>
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
	    const int timeout,
	    const bool anonymous){
  
  if (JobDescriptionFiles.empty() && JobDescriptionStrings.empty()){
    std::cout<<Arc::IString("No job description input specified")<<std::endl;
    return;
  }
  
  std::list<std::string> JobDescriptionList;
  
  //Loop over input job description files
  for(std::list<std::string>::const_iterator it = JobDescriptionFiles.begin();it != JobDescriptionFiles.end(); it++){
    
    std::ifstream descriptionfile(it->c_str());

    if (!descriptionfile)
      std::cout<<Arc::IString("Can not open file: %s", *it)<<std::endl;
    
    descriptionfile.seekg(0, std::ios::end);
    std::streamsize length = descriptionfile.tellg();
    descriptionfile.seekg(0, std::ios::beg);
    
    char* buffer = new char[length+1];
    descriptionfile.read(buffer, length);
    descriptionfile.close();
    
    buffer[length] = '\0';
    std::string xrsl(buffer);
    delete[] buffer;

    //!!Each job description file can contain several jobs, this must be split into 
    //several entries in the JobDescription list. Need to be fixed once the JobDescription
    //class is in place
    
    JobDescriptionList.push_back(xrsl);

  }

  //Loop over job description input strings
  for (std::list<std::string>::const_iterator it = JobDescriptionStrings.begin();it != JobDescriptionStrings.end(); it++) {
    
    //!!Each job description file can contain several jobs, this must be split into 
    //several entries in the JobDescription list. Need to be fixed once the JobDescription
    //class is in place
    
    JobDescriptionList.push_back(*it);
    
  }
  
  // We have now sorted and organized the JobDescriptions, 
  // next we find available clusters
  
  Arc::ACCConfig acccfg;
  Arc::NS ns;
  Arc::Config mcfg(ns);
  acccfg.MakeConfig(mcfg);
  
  bool ClustersSpecified = false;
  bool IndexServersSpecified = false;
  int TargetURL = 1;

  //first add to config element the specified target clusters (if any)
  for (std::list<std::string>::const_iterator it = ClusterSelect.begin(); it != ClusterSelect.end(); it++){
    
    std::cout<<"Submitting to targeted cluster: " << TargetURL<<std::endl;

    size_t colon = (*it).find_first_of(":");
    std::string GridFlavour = (*it).substr(0, colon);
    std::string SomeURL = (*it).substr(colon+1);

    Arc::XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
    ThisRetriever.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
    ThisRetriever.NewAttribute("id") = "retriever" + Arc::tostring(TargetURL);
    Arc::XMLNode ThisRetriever1 = ThisRetriever.NewChild("URL") = (std::string) SomeURL;
    ThisRetriever1.NewAttribute("ServiceType") = "computing";
    
    TargetURL++;
    ClustersSpecified = true;
  }

  //if no cluster url are given next steps are index servers (giis'es in ARC0)
  if (!ClustersSpecified){ //means that -c option takes priority over -g
    for (std::list<std::string>::const_iterator it = IndexUrls.begin(); it != IndexUrls.end(); it++){      
      
      size_t colon = (*it).find_first_of(":");
      std::string GridFlavour = (*it).substr(0, colon);
      std::string SomeURL = (*it).substr(colon+1);
      
      Arc::XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
      ThisRetriever.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
      ThisRetriever.NewAttribute("id") = "retriever" + Arc::tostring(TargetURL);
      Arc::XMLNode ThisRetriever1 = ThisRetriever.NewChild("URL") = (std::string) SomeURL;
      ThisRetriever1.NewAttribute("ServiceType") = "index";
      
      TargetURL++;
      IndexServersSpecified = true;
    }
  }
  
  //if neither clusters nor index servers are specified, read from config file
  if(!ClustersSpecified && !IndexServersSpecified){
    
  }

  //remove cluster that are rejected by user
  for (std::list<std::string>::const_iterator it = ClusterReject.begin();it != ClusterReject.end(); it++){
    
  }
    
  //get cluster information end prepare execution targets to be considered by the broker
  Arc::TargetGenerator TarGen(mcfg);
  TarGen.GetTargets(0, 1);  

  //store time of information request
  Arc::Time infotime;
  
  std::map<int, std::string> notsubmitted;
  
  int jobnr = 1;
  std::list<std::string> jobids;
  
  if (ClusterSelect.size()==1 && TarGen.FoundTargets.size()==0) {
    std::cout<< Arc::IString("Job submission failed due to: \n"
                             "The specified cluster %s \n"
                             "did not return any information", *ClusterSelect.begin()) << std::endl;
    
    return;

  }
  
  Arc::XMLNode JobIdStorage;
  JobIdStorage.ReadFromFile("jobs.xml");

  for (std::list<std::string>::iterator it = JobDescriptionList.begin();it != JobDescriptionList.end(); it++, jobnr++){
    
    //if more than 5 minutes has passed, renew target information
    Arc::Time now;
    if(now.GetTime() - infotime.GetTime() > 300){
      //TarGen.CleanTargets();
      //TarGen.GetTargets(0, 1);
      infotime = now;
    }
    
    //perform brokering (not yet implemented)
    //broker needs to take JobDescription is input
    
    //get submitter from chosen target 
    //for now use the first executiontarget found
    
    Arc::Submitter *submitter = TarGen.FoundTargets.begin()->GetSubmitter();
    
    std::cout<<"Submitting jobs ..."<<std::endl;
    std::pair<Arc::URL, Arc::URL> jobid;
    jobid = submitter->Submit(*it);
    
    Arc::XMLNode ThisJob = JobIdStorage.NewChild("job");
    ThisJob.NewAttribute("id") = jobid.first.str();
    ThisJob.NewChild("name") = "test";
    ThisJob.NewChild("flavour") = TarGen.FoundTargets.begin()->GridFlavour;   
    ThisJob.NewChild("source") = jobid.second.str();
    
    std::cout<<"Job submitted with jobid: "<< jobid.first.str() <<std::endl;
    std::cout<<"Information endpoint for this job: "<< jobid.second.str() <<std::endl;
    
    /*
    std::string jobid;    
    std::string jobname;
      
      try {
      jobname = it->GetRelation("jobname").GetSingleValue();
      }
    catch (XrslError e) {}
    
    try {
      it->Eval();
      
      PerformXrslValidation(*it, unknownattr);
      
      std::list<Target> targetlist = ConstructTargets(queuelist, *it);
      
      PerformStandardBrokering(targetlist);
      
      JobSubmission submit(*it, targetlist, dryrun);
      
      if (dumpxrsl) {
	if(targetlist.begin() != targetlist.end()) {
	  notify(INFO) << _("Selected queue") << ": "
		       << targetlist.begin()->name << "@"
		       << targetlist.begin()->cluster.hostname << std::endl;
	  
	  Xrsl jobxrsl = submit.PrepareXrsl(*targetlist.begin());
	  std::cout << jobxrsl.str() << std::endl;
	} else {
	  notify(WARNING) << _("No suitable target found") << std::endl;
	};
	continue;
      }
      
      jobid = submit.Submit(timeout);
      
      submit.RegisterJobsubmission(queuelist);
      
    }
    catch (ARCLibError e) {
      notify(ERROR) << _("Job submission failed due to")
		    << ": " << e.what() << std::endl;
      notsubmitted[jobnr] = jobname;
      continue;
    }
    
    AddJobID(jobid, jobname);

    if (!joblistfile.empty()) {
      LockFile(joblistfile);
      std::ofstream jobs(joblistfile.c_str(), std::ios::app);
      jobs << jobid << std::endl;
      jobs.close();
			UnlockFile(joblistfile);
    }
    
    std::string histfilename = GetEnv("HOME");
    histfilename.append ("/.arc/history");
    LockFile(histfilename);
    std::ofstream nghist (histfilename.c_str(), std::ios::app);
    nghist << TimeStamp() << "  " << jobid << std::endl;
    nghist.close();
    UnlockFile(histfilename);
    
    std::cout << _("Job submitted with jobid")
	      << ": " << jobid << std::endl;
    
    jobids.push_back(jobid);
  }
    */  
    
  }
   
  JobIdStorage.SaveToFile("jobs.xml");  

  if (JobDescriptionList.size()>1) {
    std::cout << std::endl << Arc::IString("Job submission summary:") << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were submitted", 
			      JobDescriptionList.size() - notsubmitted.size(), JobDescriptionList.size())<<std::endl;
    if (notsubmitted.size()) {
      std::cout << Arc::IString("The following %d were not submitted", notsubmitted.size()) << std::endl;
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

#define ARCSUB
#include "arccli.cpp"
