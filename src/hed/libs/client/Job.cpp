#include <arc/IString.h>
#include <arc/ArcConfig.h>
#include <arc/client/Job.h>

namespace Arc {
  
  Job::Job() : ExitCode(-1),
	       WaitingPosition(-1),
	       RequestedWallTime(-1),
	       RequestedTotalCPUTime(-1),
	       RequestedMainMemory(-1),
	       RequestedSlots(-1),
	       UsedWallTime(-1),
	       UsedTotalCPUTime(-1),
	       UsedMainMemory(-1),
	       UsedSlots(-1),
	       SubmissionTime(-1),
	       ComputingManagerSubmissionTime(-1),
	       StartTime(-1),
	       ComputingManagerEndTime(-1),
	       EndTime(-1),
	       WorkingAreaEraseTime(-1),
	       ProxyExpirationTime(-1),
	       CreationTime(-1),
	       Validity(-1),
	       VirtualMachine(false) {}
  
  Job::~Job() {}

  void Job::Print(bool longlist) const{
    
    std::cout<<Arc::IString("Job: %s", JobID.str())<<std::endl;
    if (!Name.empty())
      std::cout<<Arc::IString(" Name: %s", Name)<<std::endl;
    if (!State.empty())
      std::cout<<Arc::IString(" State: %s", State)<<std::endl;
    if(ExitCode != -1)
      std::cout<<Arc::IString(" Exit Code: %d", ExitCode)<<std::endl;
    if(!Error.empty()){
      std::list<std::string>::const_iterator iter;
      for(iter = Error.begin(); iter != Error.end(); iter++ ){
	std::cout<<Arc::IString(" Error: %s", *iter)<<std::endl;
      }
    }
    if (longlist) {
      if (!Owner.empty())
	std::cout<<Arc::IString(" Owner: %s", Owner)<<std::endl;	  
      if (!OtherMessages.empty())
	std::cout<<Arc::IString(" Other Messages: %s", OtherMessages)<<std::endl;	  	    
      if (!ExecutionCE.empty())
	std::cout<<Arc::IString(" ExecutionCE: %s", ExecutionCE)<<std::endl;	  	      
      if (!Queue.empty())
	std::cout<<Arc::IString(" Queue: %s", Queue)<<std::endl;	  	  
      if (UsedSlots != -1)
	std::cout<<Arc::IString(" Used Slots: %d", UsedSlots)<<std::endl;	  
      if (WaitingPosition != -1)
	std::cout<<Arc::IString(" Waiting Position: %d", WaitingPosition)<<std::endl;	                    
      if (!StdIn.empty())
	std::cout<<Arc::IString(" Stdin: %s", StdIn)<<std::endl;	                                        
      if (!StdOut.empty())
	std::cout<<Arc::IString(" Stdout: %s", StdOut)<<std::endl;	                                          
      if (!StdErr.empty())
	std::cout<<Arc::IString(" Stderr: %s", StdErr)<<std::endl;	                    			  
      if (!LogDir.empty())
	std::cout<<Arc::IString(" Grid Manager Log Directory: %s", LogDir)<<std::endl;
      if (SubmissionTime != -1)
	std::cout<<Arc::IString(" Submitted: %s", (std::string) SubmissionTime)<<std::endl;
      if (EndTime != -1)
	std::cout<<Arc::IString(" End Time: %s", (std::string) EndTime)<<std::endl;
      if (!SubmissionHost.empty())
	std::cout<<Arc::IString(" Submitted from: %s", SubmissionHost)<<std::endl;                                  
      if (!SubmissionClientName.empty())
	std::cout<<Arc::IString(" Submitting client: %s", SubmissionClientName)<<std::endl;
      if (RequestedTotalCPUTime != -1)
	std::cout<<Arc::IString(" Requested CPU Time: %s", (std::string) RequestedTotalCPUTime)<<std::endl;
      if (UsedTotalCPUTime != -1)
	std::cout<<Arc::IString(" Used CPU Time: %s", (std::string) UsedTotalCPUTime)<<std::endl;
      if (UsedWallTime != -1)
	std::cout<<Arc::IString(" Used Wall Time: %s", (std::string) UsedWallTime)<<std::endl;
      if (UsedMainMemory != -1)
	std::cout<<Arc::IString(" Used Memory: %d", UsedMainMemory)<<std::endl;
      if (WorkingAreaEraseTime != -1)
	std::cout<<Arc::IString((State == "DELETED") ? 
				" Results were deleted: %s" : 
				" Results must be retrieved before: %s", 
				  (std::string) WorkingAreaEraseTime)<<std::endl;
      if (ProxyExpirationTime != -1)
	std::cout<<Arc::IString(" Proxy valid until: %s", (std::string) ProxyExpirationTime)<<std::endl;
      if (CreationTime != -1)
	std::cout<<Arc::IString(" Entry valid from: %s", (std::string) CreationTime)<<std::endl;
      if (Validity != -1)
	std::cout<<Arc::IString(" Entry valid for: %s", (std::string) Validity)<<std::endl;
    }//end if long
    
    std::cout<<std::endl;
    
  } // end print
  
} // namespace Arc
