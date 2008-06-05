#include "JobControllerARC0.h"
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataHandle.h>
#include <arc/StringConv.h>

#include <map>
#include <iostream>
#include <algorithm>

namespace Arc {

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
      
      std::cout<<"Final URL = " << FinalURL.str() << std::endl;

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
						     "[nordugrid-job-globalid='"+(*it)->id+"']", NS()).begin());
	if(JobXMLInfo["nordugrid-job-status"])
	  (*it)->status = (std::string) JobXMLInfo["nordugrid-job-status"];
	if(JobXMLInfo["nordugrid-job-globalowner"])
	  (*it)->owner = (std::string) JobXMLInfo["nordugrid-job-globalowner"];
	if(JobXMLInfo["nordugrid-job-execcluster"])
	  (*it)->cluster = (std::string) JobXMLInfo["nordugrid-job-execcluster"];
	if(JobXMLInfo["nordugrid-job-execqueue"])
	  (*it)->queue = (std::string) JobXMLInfo["nordugrid-job-execqueue"];
	if(JobXMLInfo["nordugrid-job-submissionui"])
	  (*it)->submission_ui = (std::string) JobXMLInfo["nordugrid-job-submissionui"];
	if(JobXMLInfo["nordugrid-job-submissiontime"])
	  (*it)->submission_time = (std::string) JobXMLInfo["nordugrid-job-submissiontime"];
	if(JobXMLInfo["nordugrid-job-sessiondirerasetime"])
	  (*it)->erase_time = (std::string) JobXMLInfo["nordugrid-job-sessiondirerasetime"];
	if(JobXMLInfo["nordugrid-job-proxyexpirationtime"])
	  (*it)->proxy_expire_time = (std::string) JobXMLInfo["nordugrid-job-proxyexpirationtime"];
	if(JobXMLInfo["nordugrid-job-completiontime"])
	  (*it)->completion_time = (std::string) JobXMLInfo["nordugrid-job-completiontime"];
	if(JobXMLInfo["nordugrid-job-cpucount"])
	  (*it)->cpu_count = stringtoi(JobXMLInfo["nordugrid-job-cpucount"]);
	if(JobXMLInfo["nordugrid-job-usedcputime"])
	  (*it)->used_cpu_time = (std::string) JobXMLInfo["nordugrid-job-usedcputime"];
	if(JobXMLInfo["nordugrid-job-usedwalltime"])
	  (*it)->used_wall_time = (std::string) JobXMLInfo["nordugrid-job-usedwalltime"];
	if(JobXMLInfo["nordugrid-job-exitcode"])
	  (*it)->exitcode = stringtoi(JobXMLInfo["nordugrid-job-exitcode"]);
	if(JobXMLInfo["Mds-validfrom"])
	  (*it)->mds_validfrom = (std::string) (JobXMLInfo["Mds-validfrom"]);
	if(JobXMLInfo["Mds-validto"])
	  (*it)->mds_validto = (std::string) (JobXMLInfo["Mds-validto"]);
	if(JobXMLInfo["nordugrid-job-stdout"])
	  (*it)->sstdout = (std::string) (JobXMLInfo["nordugrid-job-stdout"]);
	if(JobXMLInfo["nordugrid-job-stderr"])
	  (*it)->sstderr = (std::string) (JobXMLInfo["nordugrid-job-stderr"]);
	if(JobXMLInfo["nordugrid-job-stdin"])
	  (*it)->sstdin = (std::string) (JobXMLInfo["nordugrid-job-stdin"]);
	if(JobXMLInfo["nordugrid-job-reqcputime"])
	  (*it)->requested_cpu_time = (std::string) (JobXMLInfo["nordugrid-job-reqcputime"]);
	if(JobXMLInfo["nordugrid-job-reqwalltime"])
	  (*it)->requested_wall_time = (std::string) (JobXMLInfo["nordugrid-job-reqwalltime"]);
	if(JobXMLInfo["nordugrid-job-rerunable"])
	  (*it)->rerunable = (std::string) (JobXMLInfo["nordugrid-job-rerunable"]);
	if(JobXMLInfo["nordugrid-job-queuerank"])
	  (*it)->queue_rank = stringtoi(JobXMLInfo["nordugrid-job-queuerank"]);
	if(JobXMLInfo["nordugrid-job-comment"])
	  (*it)->comment = (std::string) (JobXMLInfo["nordugrid-job-comment"]);
	if(JobXMLInfo["nordugrid-job-usedmem"])
	  (*it)->used_memory = stringtoi(JobXMLInfo["nordugrid-job-usedmem"]);
	if(JobXMLInfo["nordugrid-job-errors"])
	  (*it)->errors = (std::string) (JobXMLInfo["nordugrid-job-errors"]);
	if(JobXMLInfo["nordugrid-job-jobname"])
	  (*it)->job_name = (std::string) (JobXMLInfo["nordugrid-job-jobname"]);
	if(JobXMLInfo["nordugrid-job-gmlog"])
	  (*it)->gmlog = (std::string) (JobXMLInfo["nordugrid-job-gmlog"]);
	if(JobXMLInfo["nordugrid-job-clientsofware"])
	  (*it)->client_software = (std::string) (JobXMLInfo["nordugrid-job-clientsoftware"]);


	/*
	  std::list<RuntimeEnvironment> runtime_environments;
	  std::list<std::string> execution_nodes;
	*/

      }

    } //end loop over the different hosts

  } //end GetJobInformation

  void JobControllerARC0::PerformAction(std::string /* action */){

  }

} // namespace Arc
