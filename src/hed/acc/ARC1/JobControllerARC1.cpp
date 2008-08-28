#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataHandle.h>
#include <arc/StringConv.h>
#include <arc/client/UserConfig.h>

#include "JobControllerARC1.h"
#include "arex_client.h"

#include <map>
#include <iostream>
#include <algorithm>

namespace Arc {

  Logger JobControllerARC1::logger(JobController::logger, "ARC1");

  JobControllerARC1::JobControllerARC1(Arc::Config *cfg)
    : JobController(cfg, "ARC1") {}
  
  JobControllerARC1::~JobControllerARC1() {}

  ACC *JobControllerARC1::Instance(Config *cfg, ChainContext *) {
    return new JobControllerARC1(cfg);
  }
  
  void JobControllerARC1::GetJobInformation() {
    for (std::list<Job>::iterator iter = JobStore.begin();
	 iter != JobStore.end(); iter++) {
      MCCConfig cfg;
      UserConfig uc;
      const XMLNode cfgtree = uc.ConfTree();
      cfg.AddProxy(cfgtree["ProxyPath"]);
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      AREXClient ac(url, cfg);
      NS ns;
      ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
      ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
      ns["wsa"]="http://www.w3.org/2005/08/addressing";
      ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
      ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
      ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
      ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
      XMLNode id(ns, "ActivityIdentifier");
      id.NewChild("wsa:Address") = url.str();
      id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
      std::string idstr;
      id.GetXML(idstr);
      iter->State = ac.stat(idstr);
    }
  }

  bool JobControllerARC1::GetThisJob(Job ThisJob, std::string downloaddir) {

    logger.msg(DEBUG, "Downloading job: %s", ThisJob.JobID.str());
    bool SuccesfulDownload = true;

    Arc::DataHandle source(ThisJob.JobID);    
    if(source){

      std::list<std::string> downloadthese = GetDownloadFiles(source);

      std::cout<<"List of downloadable files:"<<std::endl;      
      std::cout<<"Number of files: "<< downloadthese.size()<<std::endl;      

      //loop over files
      for(std::list<std::string>::iterator i = downloadthese.begin();
	  i != downloadthese.end(); i++) {
	std::cout << *i <<std::endl;	
	std::string src = ThisJob.JobID.str() + "/"+ *i;
	std::string path_temp = ThisJob.JobID.Path(); 
	size_t slash = path_temp.find_last_of("/");
	std::string dst;
	if(downloaddir.empty())
	  dst = path_temp.substr(slash+1) + "/" + *i;
	else
	  dst = downloaddir + "/" + *i;
	bool GotThisFile = CopyFile(src, dst);
	if(!GotThisFile)
	  SuccesfulDownload = false;
      }
    }
    else
      logger.msg(ERROR, "Failed dowloading job: %s. "
		 "Could not get data handle.", ThisJob.JobID.str());
  }

  bool JobControllerARC1::CleanThisJob(Job ThisJob, bool force) {
    MCCConfig cfg;
    UserConfig uc;
    const XMLNode cfgtree = uc.ConfTree();
    cfg.AddProxy(cfgtree["ProxyPath"]);
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    AREXClient ac(url, cfg);
    NS ns;
    ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"]="http://www.w3.org/2005/08/addressing";
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
    ac.clean(idstr);
    return true;
  }

  bool JobControllerARC1::CancelThisJob(Job ThisJob) {
    MCCConfig cfg;
    UserConfig uc;
    const XMLNode cfgtree = uc.ConfTree();
    cfg.AddProxy(cfgtree["ProxyPath"]);
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    AREXClient ac(url, cfg);
    NS ns;
    ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"]="http://www.w3.org/2005/08/addressing";
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
    ac.kill(idstr);
    return true;
  }

  URL JobControllerARC1::GetFileUrlThisJob(Job ThisJob,
					   std::string whichfile) {}

  std::list<std::string>
  JobControllerARC1::GetDownloadFiles(Arc::DataHandle& dir,
				      std::string dirname) {

    std::list<std::string> files;

    std::list<FileInfo> outputfiles;
    dir->ListFiles(outputfiles, true);

    for(std::list<Arc::FileInfo>::iterator i = outputfiles.begin();
	i != outputfiles.end(); i++) {
      if(i->GetType() == 0 || i->GetType() == 1) {
	std::cout<<"adding file "<< i->GetName() << std::endl;
	if(!dirname.empty())
	  if (dirname[dirname.size() - 1] != '/')
	    files.push_back(dirname + "/" + i->GetName());
	  else
	    files.push_back(dirname + i->GetName());
	else
	  files.push_back(i->GetName());
      }
      else if(i->GetType() == 2) {
	std::cout<<"Found directory " << i->GetName() << std::endl;
	std::string dirurl(dir->str());
	if (dirurl[dirurl.size() - 1] != '/')
	  dirurl += "/";
	dirurl += i->GetName();
	Arc::DataHandle tmpdir(dirurl);
	std::list<std::string> morefiles = GetDownloadFiles(tmpdir,
							    i->GetName());
	for(std::list<std::string>::iterator j = morefiles.begin();
	    j != morefiles.end(); j++)
	  if(!dirname.empty())
	    if (dirname[dirname.size() - 1] != '/')
	      files.push_back(dirname + "/"+ *j);
	    else
	      files.push_back(dirname + *j);
	  else
	    files.push_back(*j);
      }
    }
    return files;
  }

} // namespace Arc
