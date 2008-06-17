#include <arc/IString.h>
#include "JobController.h"
#include <arc/ArcConfig.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/User.h>

#include <iostream>
#include <algorithm>

namespace Arc {
  
  JobController::JobController(Arc::Config *cfg, std::string flavour)
    : ACC(), 
      GridFlavour(flavour),
      mcfg(*cfg) {}

  JobController::~JobController() {}
  
  void JobController::IdentifyJobs(std::list<std::string> jobids){

    if(jobids.empty()){ //We are looking at all jobs of this grid flavour

      Arc::XMLNodeList Jobs = mcfg.XPathLookup("//job[flavour='"+ GridFlavour+"']", Arc::NS());
      XMLNodeList::iterator iter;
      
      for(iter = Jobs.begin(); iter!= Jobs.end(); iter++){
	Job ThisJob;
	ThisJob.JobID = (std::string) (*iter)["id"];
	ThisJob.InfoEndpoint = (std::string) (*iter)["source"];
	JobStore.push_back(ThisJob);
      }

    } else {//Jobs are targeted individually. The supplied jobids list may not contain any jobs of this grid flavour

      std::list<std::string>::iterator it;

      for(it = jobids.begin(); it != jobids.end(); it++){
	Arc::XMLNode ThisXMLJob = (*(mcfg.XPathLookup("//job[id='"+ *it+"']", Arc::NS())).begin());
	if(GridFlavour == (std::string) ThisXMLJob["flavour"]){
	  Job ThisJob;
	  ThisJob.JobID = (std::string) ThisXMLJob["id"];
	  ThisJob.InfoEndpoint = (std::string) ThisXMLJob["source"];
	  JobStore.push_back(ThisJob);	  
	}
      }
    }

    std::cout<<"JobController"+GridFlavour+" has identified " << JobStore.size() << " jobs" <<std::endl;

  }

  void JobController::PrintJobInformation(bool longlist){
    
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){
      
      std::cout<<Arc::IString("Job: %s", jobiter->JobID.str())<<std::endl;
      if (!jobiter->Name.empty())
	std::cout<<Arc::IString(" Name: %s", jobiter->Name)<<std::endl;
      if (!jobiter->State.empty())
	std::cout<<Arc::IString(" State: %s", jobiter->State)<<std::endl;
      if(jobiter->ExitCode != -1)
	std::cout<<Arc::IString(" Exit Code: %d", jobiter->ExitCode)<<std::endl;
      if(!jobiter->Error.empty()){
	std::list<std::string>::iterator iter;
	for(iter = jobiter->Error.begin(); iter != jobiter->Error.end(); iter++ ){
	  std::cout<<Arc::IString(" Error: %s", *iter)<<std::endl;
	}
      }
      if (longlist) {
	if (!jobiter->Owner.empty())
	  std::cout<<Arc::IString(" Owner: %s", jobiter->Owner)<<std::endl;	  
	if (!jobiter->OtherMessages.empty())
	  std::cout<<Arc::IString(" Other Messages: %s", jobiter->OtherMessages)<<std::endl;	  	    
	if (!jobiter->ExecutionCE.empty())
	  std::cout<<Arc::IString(" ExecutionCE: %s", jobiter->ExecutionCE)<<std::endl;	  	      
	if (!jobiter->Queue.empty())
	  std::cout<<Arc::IString(" Queue: %s", jobiter->Queue)<<std::endl;	  	  
	if (jobiter->UsedSlots != -1)
	  std::cout<<Arc::IString(" Used Slots: %d", jobiter->UsedSlots)<<std::endl;	  
	if (jobiter->WaitingPosition != -1)
	  std::cout<<Arc::IString(" Waiting Position: %d", jobiter->WaitingPosition)<<std::endl;	                    
	if (!jobiter->StdIn.empty())
	  std::cout<<Arc::IString(" Stdin: %s", jobiter->StdIn)<<std::endl;	                                        
	if (!jobiter->StdOut.empty())
	  std::cout<<Arc::IString(" Stdout: %s", jobiter->StdOut)<<std::endl;	                                          
	if (!jobiter->StdErr.empty())
	  std::cout<<Arc::IString(" Stderr: %s", jobiter->StdErr)<<std::endl;	                    			  
	if (!jobiter->LogDir.empty())
	  std::cout<<Arc::IString(" Grid Manager Log Directory: %s", jobiter->LogDir)<<std::endl;
	if (jobiter->SubmissionTime != -1)
	  std::cout<<Arc::IString(" Submitted: %s", (std::string) jobiter->SubmissionTime)<<std::endl;
	if (jobiter->EndTime != -1)
	  std::cout<<Arc::IString(" End Time: %s", (std::string) jobiter->EndTime)<<std::endl;
	if (!jobiter->SubmissionHost.empty())
	  std::cout<<Arc::IString(" Submitted from: %s", jobiter->SubmissionHost)<<std::endl;                                  
	if (!jobiter->SubmissionClientName.empty())
	  std::cout<<Arc::IString(" Submitting client: %s", jobiter->SubmissionClientName)<<std::endl;
	if (jobiter->RequestedTotalCPUTime != -1)
	  std::cout<<Arc::IString(" Requested CPU Time: %s", (std::string) jobiter->RequestedTotalCPUTime)<<std::endl;
	if (jobiter->UsedTotalCPUTime != -1)
	  std::cout<<Arc::IString(" Used CPU Time: %s", (std::string) jobiter->UsedTotalCPUTime)<<std::endl;
	if (jobiter->UsedWallTime != -1)
	  std::cout<<Arc::IString(" Used Wall Time: %s", (std::string) jobiter->UsedWallTime)<<std::endl;
	if (jobiter->UsedMainMemory != -1)
	  std::cout<<Arc::IString(" Used Memory: %d", jobiter->UsedMainMemory)<<std::endl;
	if (jobiter->WorkingAreaEraseTime != -1)
	  std::cout<<Arc::IString((jobiter->State == "DELETED") ? 
				  " Results were deleted: %s" : 
				  " Results must be retrieved before: %s", 
				  (std::string) jobiter->WorkingAreaEraseTime)<<std::endl;
	if (jobiter->ProxyExpirationTime != -1)
	  std::cout<<Arc::IString(" Proxy valid until: %s", (std::string) jobiter->ProxyExpirationTime)<<std::endl;
	if (jobiter->CreationTime != -1)
	  std::cout<<Arc::IString(" Entry valid from: %s", (std::string) jobiter->CreationTime)<<std::endl;
	if (jobiter->Validity != -1)
	  std::cout<<Arc::IString(" Entry valid until: %s", (std::string) jobiter->Validity)<<std::endl;
      }//end if long
      
      std::cout<<std::endl;
      
    } //end loop over jobs
    
  }
  
  static void progress(FILE *o, const char *, unsigned int,
		       unsigned long long int all, unsigned long long int max,
		       double, double) {
    static int rs = 0;
    const char rs_[4] = {'|', '/', '-', '\\'};
    if (max) {
      fprintf(o, "\r|");
      unsigned int l = (74 * all + 37) / max;
      if (l > 74) l = 74;
      unsigned int i = 0;
      for (; i < l; i++) fprintf(o, "=");
      fprintf(o, "%c", rs_[rs++]);
      if (rs > 3) rs = 0;
      for (; i < 74; i++) fprintf(o, " ");
      fprintf(o, "|\r");
      fflush(o);
      return;
    }
    fprintf(o, "\r%llu kB                    \r", all / 1024);
  }

  void JobController::CopyFile(URL src, URL dst){

    Arc::DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(false);
    mover.verbose(true);
    mover.set_progress_indicator(&progress);
            
    std::cout<<"Now copying (from -> to):"<<std::endl;
    std::cout << src.str() << " -> " << dst.str() << std::endl;

    Arc::DataHandle source(src);
    Arc::DataHandle destination(dst);

    // Create cache
    // This should not be needed (DataMover bug)
    Arc::User cache_user;
    std::string cache_path2;
    std::string cache_data_path;
    std::string id = "<ngcp>";
    Arc::DataCache cache(cache_path2, cache_data_path, "", id, cache_user);
    
    std::string failure;
    int timeout = 300;
    if (!mover.Transfer(*source, *destination, cache, Arc::URLMap(), 0, 0, 0, timeout, failure)) {
      if (!failure.empty()) std::cerr << "File download failed: " << failure << std::endl;
      else std::cerr << "File download failed" << std::endl;
    }
    
  }

} // namespace Arc
