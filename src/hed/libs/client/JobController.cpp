#include <arc/IString.h>
#include "JobController.h"
#include <arc/ArcConfig.h>

#include <iostream>
#include <algorithm>

namespace Arc {
  
  JobController::JobController(Arc::Config *cfg, std::string flavour) : ACC(), 
									mcfg(*cfg),
									GridFlavour(flavour){}
  JobController::~JobController() {}
  
  void JobController::IdentifyJobs(std::list<std::string> jobids){

    if(jobids.empty()){ //We are looking at all jobs of this grid flavour

      Arc::XMLNodeList Jobs = mcfg.XPathLookup("//job[flavour='"+ GridFlavour+"']", Arc::NS());
      XMLNodeList::iterator iter;
      
      for(iter = Jobs.begin(); iter!= Jobs.end(); iter++){
	Job ThisJob;
	ThisJob.id = (std::string) (*iter)["id"];
	JobStore.push_back(ThisJob);
      }

    } else {//Jobs are targeted individually. The supplied jobids list may not contain any jobs of this grid flavour

      std::list<std::string>::iterator it;

      for(it = jobids.begin(); it != jobids.end(); it++){
	Arc::XMLNode ThisXMLJob = (*(mcfg.XPathLookup("//job[id='"+ *it+"']", Arc::NS())).begin());
	if(GridFlavour == (std::string) ThisXMLJob["flavour"]){
	  Job ThisJob;
	  ThisJob.id = (std::string) ThisXMLJob["id"];
	  ThisJob.InfoEndpoint = (std::string) ThisXMLJob["source"];
	  JobStore.push_back(ThisJob);	  
	}
      }
    }

    std::cout<<"JobController"+GridFlavour+" has identified " << JobStore.size() << " jobs" <<std::endl;

  }

  void JobController::PrintJobInformation(bool longlist){
    
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){
      
      std::cout<<Arc::IString("Job: %s", jobiter->id)<<std::endl;
      if (!jobiter->job_name.empty())
	std::cout<<Arc::IString(" Name: %s", jobiter->job_name)<<std::endl;
      if (!jobiter->status.empty())
	std::cout<<Arc::IString(" Status: %s", jobiter->status)<<std::endl;
      if(jobiter->exitcode != -1)
	std::cout<<Arc::IString(" Exit Code: %d", jobiter->exitcode)<<std::endl;
      if(!jobiter->errors.empty())
	std::cout<<Arc::IString(" Errors: %s", jobiter->errors)<<std::endl;
      
      if (longlist) {
	if (!jobiter->owner.empty())
	  std::cout<<Arc::IString(" Owner: %s", jobiter->owner)<<std::endl;	  
	if (!jobiter->comment.empty())
	  std::cout<<Arc::IString(" Comment: %s", jobiter->comment)<<std::endl;	  	    
	if (!jobiter->cluster.empty())
	  std::cout<<Arc::IString(" Cluster: %s", jobiter->cluster)<<std::endl;	  	      
	if (!jobiter->queue.empty())
	  std::cout<<Arc::IString(" Queue: %s", jobiter->queue)<<std::endl;	  	  
	if (jobiter->cpu_count != -1)
	  std::cout<<Arc::IString(" Requested Number of CPUs: %d", jobiter->cpu_count)<<std::endl;	  
	if (jobiter->queue_rank != -1)
	  std::cout<<Arc::IString(" Rank: %d", jobiter->queue_rank)<<std::endl;	                    
	if (!jobiter->sstdin.empty())
	  std::cout<<Arc::IString(" stdin: %s", jobiter->sstdin)<<std::endl;	                                        
	if (!jobiter->sstdout.empty())
	  std::cout<<Arc::IString(" stdout: %s", jobiter->sstdout)<<std::endl;	                                          
	if (!jobiter->sstderr.empty())
	  std::cout<<Arc::IString(" stderr: %s", jobiter->sstderr)<<std::endl;	                    			  
	if (!jobiter->gmlog.empty())
	  std::cout<<Arc::IString(" Grid Manager Log Directory: %s", jobiter->gmlog)<<std::endl;
	if (jobiter->submission_time != -1)
	  std::cout<<Arc::IString(" Submitted: %s", (std::string) jobiter->submission_time)<<std::endl;
	if (jobiter->completion_time != -1)
	  std::cout<<Arc::IString(" Completed: %s", (std::string) jobiter->completion_time)<<std::endl;
	if (!jobiter->submission_ui.empty())
	  std::cout<<Arc::IString(" Submitted from: %s", jobiter->submission_ui)<<std::endl;                                  
	if (!jobiter->client_software.empty())
	  std::cout<<Arc::IString(" Submitting client: %s", jobiter->client_software)<<std::endl;
	if (jobiter->requested_cpu_time != -1)
	  std::cout<<Arc::IString(" Reguested CPU Time: %s", (std::string) jobiter->requested_cpu_time)<<std::endl;
	if (jobiter->used_cpu_time != -1)
	  std::cout<<Arc::IString(" Used CPU Time: %s", (std::string) jobiter->used_cpu_time)<<std::endl;
	if (jobiter->used_wall_time != -1)
	  std::cout<<Arc::IString(" Used Wall Time: %s", (std::string) jobiter->used_wall_time)<<std::endl;
	if (jobiter->used_memory != -1)
	  std::cout<<Arc::IString(" Used Memory: %d", jobiter->used_memory)<<std::endl;
	if (jobiter->erase_time != -1)
	  std::cout<<Arc::IString((jobiter->status == "DELETED") ? 
				  " Results were deleted: %s" : 
				  " Results must be retrieved before: %s", (std::string) jobiter->erase_time)<<std::endl;
	if (jobiter->proxy_expire_time != -1)
	  std::cout<<Arc::IString(" Proxy valid until: %s", (std::string) jobiter->proxy_expire_time)<<std::endl;
	if (jobiter->mds_validfrom != -1)
	  std::cout<<Arc::IString(" Entry valid from: %s", (std::string) jobiter->mds_validfrom)<<std::endl;
	if (jobiter->mds_validto != -1)
	  std::cout<<Arc::IString(" Entry valid until: %s", (std::string) jobiter->mds_validto)<<std::endl;
      }//end if long

      std::cout<<std::endl;

    } //end loop over jobs

  }

} // namespace Arc
