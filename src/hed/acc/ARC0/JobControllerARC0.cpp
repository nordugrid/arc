#include "JobControllerARC0.h"
#include "FTPControl.h"
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataHandle.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>

#include <map>
#include <iostream>
#include <algorithm>

namespace Arc {

  Logger JobControllerARC0::logger(Logger::getRootLogger(), "JobControllerARC0");

  JobControllerARC0::JobControllerARC0(Arc::Config *cfg) : JobController(cfg, "ARC0") {}
  
  JobControllerARC0::~JobControllerARC0() {}

  ACC *JobControllerARC0::Instance(Config *cfg, ChainContext *) {
    return new JobControllerARC0(cfg);
  }
  
  void JobControllerARC0::GetJobInformation(){

    //1. Sort jobs according to host and base dn (path)
    std::list<Job>::iterator iter;
    std::map< std::string, std::list<Job*> > SortedByHost; 

    for(iter=JobStore.begin();iter != JobStore.end(); iter++){
      SortedByHost[iter->InfoEndpoint.Host()+"/"+iter->InfoEndpoint.Path()].push_back(&*iter);  
    }
    
    //Actions below this point could(should) be threaded in the future

    std::map< std::string, std::list<Job*> >::iterator HostIterator;

    for(HostIterator = SortedByHost.begin(); HostIterator != SortedByHost.end(); HostIterator++){
      //2. Copy info endpoint url from one of the jobs      
      URL FinalURL = (*HostIterator->second.begin())->InfoEndpoint;
      //3a. Prepare new filter      
      std::string filter = "(|";
      std::list<Job*>::iterator it;      
      for(it = HostIterator->second.begin(); it != HostIterator->second.end(); it++){
	filter += (*it)->InfoEndpoint.LDAPFilter();
      }
      filter += ")";
      //3b. Change filter of the final url      
      FinalURL.ChangeLDAPFilter(filter);
      
      //4. Read from information source
      
      DataHandle handler(FinalURL);
      DataBufferPar buffer;

      if (!handler->StartReading(buffer))
	return;

      int handle;
      unsigned int length;
      unsigned long long int offset;
      std::string result;

      while (buffer.for_write() || !buffer.eof_read())
	if (buffer.for_write(handle, length, offset, true)) {
	  result.append(buffer[handle], length);
	  buffer.is_written(handle);
	}

      if (!handler->StopReading())
	return;

      //5. Fill jobs with information
      XMLNode XMLresult(result);

      for(it = HostIterator->second.begin(); it != HostIterator->second.end(); it++){
	XMLNode JobXMLInfo = (*XMLresult.XPathLookup("//nordugrid-job-globalid"
						     "[nordugrid-job-globalid='"+(*it)->JobID.str()+"']", NS()).begin());
	if(JobXMLInfo["nordugrid-job-status"])
	  (*it)->State = (std::string) JobXMLInfo["nordugrid-job-status"];
	if(JobXMLInfo["nordugrid-job-globalowner"])
	  (*it)->Owner = (std::string) JobXMLInfo["nordugrid-job-globalowner"];
	if(JobXMLInfo["nordugrid-job-execcluster"])
	  (*it)->ExecutionCE = (std::string) JobXMLInfo["nordugrid-job-execcluster"];
	if(JobXMLInfo["nordugrid-job-execqueue"])
	  (*it)->Queue = (std::string) JobXMLInfo["nordugrid-job-execqueue"];
	if(JobXMLInfo["nordugrid-job-submissionui"])
	  (*it)->SubmissionHost = (std::string) JobXMLInfo["nordugrid-job-submissionui"];
	if(JobXMLInfo["nordugrid-job-submissiontime"])
	  (*it)->SubmissionTime = (std::string) JobXMLInfo["nordugrid-job-submissiontime"];
	if(JobXMLInfo["nordugrid-job-sessiondirerasetime"])
	  (*it)->WorkingAreaEraseTime = (std::string) JobXMLInfo["nordugrid-job-sessiondirerasetime"];
	if(JobXMLInfo["nordugrid-job-proxyexpirationtime"])
	  (*it)->ProxyExpirationTime = (std::string) JobXMLInfo["nordugrid-job-proxyexpirationtime"];
	if(JobXMLInfo["nordugrid-job-completiontime"])
	  (*it)->EndTime = (std::string) JobXMLInfo["nordugrid-job-completiontime"];
	if(JobXMLInfo["nordugrid-job-cpucount"])
	  (*it)->UsedSlots = stringtoi(JobXMLInfo["nordugrid-job-cpucount"]);
	if(JobXMLInfo["nordugrid-job-usedcputime"])
	  (*it)->UsedTotalCPUTime = (std::string) JobXMLInfo["nordugrid-job-usedcputime"];
	if(JobXMLInfo["nordugrid-job-usedwalltime"])
	  (*it)->UsedWallTime = (std::string) JobXMLInfo["nordugrid-job-usedwalltime"];
	if(JobXMLInfo["nordugrid-job-exitcode"])
	  (*it)->ExitCode = stringtoi(JobXMLInfo["nordugrid-job-exitcode"]);
	if(JobXMLInfo["Mds-validfrom"]){
	  (*it)->CreationTime = (std::string) (JobXMLInfo["Mds-validfrom"]);
	  if(JobXMLInfo["Mds-validto"]){
	    Time Validto = (std::string) (JobXMLInfo["Mds-validto"]);
	    (*it)->Validity = Validto - (*it)->CreationTime;
	  }
	}
	if(JobXMLInfo["nordugrid-job-stdout"])
	  (*it)->StdOut = (std::string) (JobXMLInfo["nordugrid-job-stdout"]);
	if(JobXMLInfo["nordugrid-job-stderr"])
	  (*it)->StdErr = (std::string) (JobXMLInfo["nordugrid-job-stderr"]);
	if(JobXMLInfo["nordugrid-job-stdin"])
	  (*it)->StdIn = (std::string) (JobXMLInfo["nordugrid-job-stdin"]);
	if(JobXMLInfo["nordugrid-job-reqcputime"])
	  (*it)->RequestedTotalCPUTime = (std::string) (JobXMLInfo["nordugrid-job-reqcputime"]);
	if(JobXMLInfo["nordugrid-job-reqwalltime"])
	  (*it)->RequestedWallTime = (std::string) (JobXMLInfo["nordugrid-job-reqwalltime"]);
	if(JobXMLInfo["nordugrid-job-rerunable"])
	  (*it)->RestartState = (std::string) (JobXMLInfo["nordugrid-job-rerunable"]);
	if(JobXMLInfo["nordugrid-job-queuerank"])
	  (*it)->WaitingPosition = stringtoi(JobXMLInfo["nordugrid-job-queuerank"]);
	if(JobXMLInfo["nordugrid-job-comment"])
	  (*it)->OtherMessages = (std::string) (JobXMLInfo["nordugrid-job-comment"]);
	if(JobXMLInfo["nordugrid-job-usedmem"])
	  (*it)->UsedMainMemory = stringtoi(JobXMLInfo["nordugrid-job-usedmem"]);
	if(JobXMLInfo["nordugrid-job-errors"]){
	  for(XMLNode n = JobXMLInfo["nordugrid-job-errors"]; n; ++n){
	    (*it)->Error.push_back((std::string) n);
	  }	
	}
	if(JobXMLInfo["nordugrid-job-jobname"])
	  (*it)->Name = (std::string) (JobXMLInfo["nordugrid-job-jobname"]);
	if(JobXMLInfo["nordugrid-job-gmlog"])
	  (*it)->LogDir = (std::string) (JobXMLInfo["nordugrid-job-gmlog"]);
	if(JobXMLInfo["nordugrid-job-clientsofware"])
	  (*it)->SubmissionClientName = (std::string) (JobXMLInfo["nordugrid-job-clientsoftware"]);
	if(JobXMLInfo["nordugrid-job-executionnodes"]){
	  for(XMLNode n = JobXMLInfo["nordugrid-job-executionnodes"]; n; ++n){
	    (*it)->ExecutionNode.push_back((std::string) n);
	  }
	}
	if(JobXMLInfo["nordugrid-job-runtimeenvironment"]){
	  for(XMLNode n = JobXMLInfo["nordugrid-job-runtimeenvironment"]; n; ++n){
	    (*it)->UsedApplicationEnvironment.push_back((std::string) n);
	  }
	}

      }

    } //end loop over the different hosts

  } //end GetJobInformation

  bool JobControllerARC0::GetThisJob(Job ThisJob, std::string downloaddir){
    
    logger.msg(DEBUG, "Downloading job: %s", ThisJob.JobID.str());
    bool SuccesfulDownload = true;

    Arc::DataHandle source(ThisJob.JobID);
    
    if(source){
      std::list<FileInfo> outputfiles;
      source->ListFiles(outputfiles, true);
      std::cout<<"Job directory contains:"<<std::endl;
      //loop over files
      for(std::list<Arc::FileInfo>::iterator i = outputfiles.begin();i != outputfiles.end(); i++){
	std::cout << i->GetName() <<std::endl;
	std::string src = ThisJob.JobID.str() +"/"+ i->GetName();
	std::string path_temp = ThisJob.JobID.Path(); 
	size_t slash = path_temp.find_first_of("/");
	std::string dst;
	if(downloaddir.empty()){
	  dst = path_temp.substr(slash+1) + "/" + i->GetName();
	} else {
	  dst = downloaddir + "/" + i->GetName();
	}
	bool GotThisFile = CopyFile(src, dst);
	if(!GotThisFile)
	  SuccesfulDownload = false;
      }
    } else {
      logger.msg(ERROR, "Failed dowloading job: %s. Could not get data handle.", ThisJob.JobID.str());
    }

    return SuccesfulDownload;

  }//end GetThisJob

  bool JobControllerARC0::CleanThisJob(Job ThisJob, bool force){

    logger.msg(DEBUG, "Cleaning job: %s", ThisJob.JobID.str());

    //connect
    FTPControl ftpCtrl;
    bool gotConnected = ftpCtrl.connect(ThisJob.JobID, 500);
    if(!gotConnected){
      logger.msg(ERROR, "Failed to connect for job cleaning");
      return false;
    }
    
    std::string path = ThisJob.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobpath = path.substr(0, pos);
    std::string jobidnum = path.substr(pos+1);
    
    //send commands
    bool CommandSent = ftpCtrl.sendCommand("CWD " + jobpath, 500); 
    if(!CommandSent){
      logger.msg(ERROR, "Failed sending CWD command for job cleaning");
      return false;	    
    }
    CommandSent = ftpCtrl.sendCommand("RMD " + jobidnum, 500); 
    if(!CommandSent){
      logger.msg(ERROR, "Failed sending RMD command for job cleaning");
      return false;	    
    }
    
    //disconnect
    bool gotDisconnected = ftpCtrl.disconnect(500);
    if(!gotDisconnected){
      logger.msg(ERROR, "Failed to disconnect after job cleaning");
      return false;	    
    }

    logger.msg(DEBUG, "Job cleaning succesful");
    return true;


  }//end CleanThisJob    

  bool JobControllerARC0::CancelThisJob(Job ThisJob){
    
    logger.msg(DEBUG, "Cleaning job: %s", ThisJob.JobID.str());
    
    //connect
    FTPControl ftpCtrl;
    bool gotConnected = ftpCtrl.connect(ThisJob.JobID, 500);
    if(!gotConnected){
      logger.msg(ERROR, "Failed to connect for job cleaning");
      return false;
    }
    
    std::string path = ThisJob.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobpath = path.substr(0, pos);
    std::string jobidnum = path.substr(pos+1);
    
    //send commands
    bool CommandSent = ftpCtrl.sendCommand("CWD " + jobpath, 500); 
    if(!CommandSent){
      logger.msg(ERROR, "Failed sending CWD command for job cancelling");
      return false;	    
    }
    CommandSent = ftpCtrl.sendCommand("DELE " + jobidnum, 500); 
    if(!CommandSent){
      logger.msg(ERROR, "Failed sending DELE command for job cancelling");
      return false;	    
    }
    
    //disconnect
    bool gotDisconnected = ftpCtrl.disconnect(500);
    if(!gotDisconnected){
      logger.msg(ERROR, "Failed to disconnect after job cancelling");
      return false;	    
    }

    logger.msg(DEBUG, "Job cancel succesful");
    return true;

  }

  URL JobControllerARC0::GetFileUrlThisJob(Job ThisJob, std::string whichfile){

    std::string urlstring;

    if (whichfile == "stdout")
      urlstring = ThisJob.JobID.str() + '/' + ThisJob.StdOut;
    else if (whichfile == "stderr")
      urlstring = ThisJob.JobID.str() + '/' + ThisJob.StdErr;
    else if (whichfile == "gmlog"){
      urlstring = ThisJob.JobID.str();
      urlstring.insert(urlstring.rfind('/'), "/info");
      urlstring += "/errors";
    }
    
    return URL(urlstring);

  }

    
} // namespace Arc
