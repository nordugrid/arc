#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/client/Broker.h>

namespace Arc {
  
  Arc::Logger logger(Arc::Logger::getRootLogger(), "broker");
  Arc::LogStream logcerr(std::cerr);
  
  Broker::Broker(Config *cfg) : ACC(cfg),
				PreFilteringDone(false),
				TargetSortingDone(false){}

  Broker::~Broker() {}
  
  void Broker::PreFilterTargets(Arc::TargetGenerator& targen,  Arc::JobDescription jd) {
    
    logger.msg(VERBOSE, "Prefiltering targets according to input jobdescription");
    
    //for testing purposes
    /*
    ExecutionTarget eTarget;
    eTarget.MaxCPUTime = 1; 
    eTarget.RunningJobs = 1; 
    eTarget.MaxRunningJobs = 1; 
    eTarget.WaitingJobs = 3;
    eTarget.MaxDiskSpace = 10;
    PossibleTargets.push_back(eTarget);
    
    eTarget.MinCPUTime = 1; 
    eTarget.MaxRunningJobs = 22;
    eTarget.WaitingJobs = 1; 
    PossibleTargets.push_back(eTarget);
    
    ExecutionTarget eTarget2;
    eTarget2.DefaultCPUTime = 1; 
    eTarget2.RunningJobs = 3; 
    eTarget2.MaxRunningJobs = 31;
    eTarget2.WaitingJobs = 20; 
    PossibleTargets.push_back(eTarget2);
    */

    //Perform pre filtering of targets found according to the input jobdescription

    Arc::XMLNode jobd = jd.getXML();

    for (std::list<Arc::ExecutionTarget>::const_iterator target =	\
	   targen.FoundTargets().begin(); target != targen.FoundTargets().end(); \
	 target++) {  
      
      //This argoments are dependence from the Resource item.
      if ( jobd["JobDescription"]["Resources"] ) {
	
	if ( (*target).MaxDiskSpace != -1 ) {
	  if ( (*target).MaxDiskSpace < jobd["JobDescription"]["Resources"]["DiskSpace"] || 
	       (*target).MaxDiskSpace < jobd["JobDescription"]["Resources"]["IndividualDiskSpace"] || 
	       (*target).MaxDiskSpace < jobd["JobDescription"]["Resources"]["TotalDiskSpace"])
	    continue;
	}
	
	if ( (*target).TotalPhysicalCPUs != -1 ) {
	  if ( (*target).TotalPhysicalCPUs < jobd["JobDescription"]["Resources"]["IndividualCPUCount"] ||
	       (*target).TotalPhysicalCPUs < jobd["JobDescription"]["Resources"]["TotalCPUCount"])
	    continue;
	}
	
	if ( (*target).TotalLogicalCPUs != -1 ) {
	  if ( (*target).TotalLogicalCPUs < jobd["JobDescription"]["Resources"]["IndividualCPUCount"] ||
	       (*target).TotalLogicalCPUs < jobd["JobDescription"]["Resources"]["TotalCPUCount"])
	    continue;
	}
	
	if ( (*target).CPUClockSpeed != -1 ) {
	  if ( (*target).CPUClockSpeed < jobd["JobDescription"]["Resources"]["IndividualCPUSpeed"])
	    continue;
	}
      }
      
      //This argoments are dependence from the Application item.
      if ( jobd["JobDescription"]["Application"] ) {
	
	if ( (int)(*target).MinWallTime.GetPeriod() != -1 ) {
	  if ( (int)(*target).MinWallTime.GetPeriod() > jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
	if ( (int)(*target).MaxWallTime.GetPeriod() != -1 ) {
	  if ( (int)(*target).MaxWallTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
	if ( (int)(*target).MaxTotalWallTime.GetPeriod() != -1 ) {
	  if ( (int)(*target).MaxTotalWallTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
	if ( (int)(*target).DefaultWallTime.GetPeriod() != -1 ) {
	  if ( (int)(*target).DefaultWallTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
      }
      
      //This argoments are dependence from the Application and the Resource items.
      if ( (int)(*target).MinCPUTime.GetPeriod() != -1 ) {
	if ( (int)(*target).MinCPUTime.GetPeriod() > jobd["JobDescription"]["Resources"]["IndividualCPUTime"] ||
	     (int)(*target).MinCPUTime.GetPeriod() > jobd["JobDescription"]["Resources"]["TotalCPUTime"] || 
	     (int)(*target).MinCPUTime.GetPeriod() > jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (int)(*target).MaxCPUTime.GetPeriod() != -1 ) {
	if ( (int)(*target).MaxCPUTime.GetPeriod() < jobd["JobDescription"]["Resources"]["IndividualCPUTime"] ||
	     (int)(*target).MaxCPUTime.GetPeriod() < jobd["JobDescription"]["Resources"]["TotalCPUTime"] ||
	     (int)(*target).MaxCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (int)(*target).MaxTotalCPUTime.GetPeriod() != -1 ) {
	if ( (int)(*target).MaxTotalCPUTime.GetPeriod() < jobd["JobDescription"]["Resources"]["IndividualCPUTime"] ||
	     (int)(*target).MaxTotalCPUTime.GetPeriod() < jobd["JobDescription"]["Resources"]["TotalCPUTime"] ||
	     (int)(*target).MaxTotalCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (int)(*target).DefaultCPUTime.GetPeriod() != -1 ) {
	if ( (int)(*target).DefaultCPUTime.GetPeriod() < jobd["JobDescription"]["Resources"]["IndividualCPUTime"] ||
	     (int)(*target).DefaultCPUTime.GetPeriod() < jobd["JobDescription"]["Resources"]["TotalCPUTime"] ||
	     (int)(*target).DefaultCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (*target).MaxMemory != -1 ) {
	if ((*target).MaxMemory < jobd["JobDescription"]["Resources"]["IndividualPhysicalMemory"] ||
	    (*target).MaxMemory < jobd["JobDescription"]["Resources"]["TotalPhysicalMemory"] ||
	    (*target).MaxMemory < jobd["JobDescription"]["Application"]["POSIXApplication"]["MemoryLimit"])
	  continue;
      }
      
      if ( (*target).NodeMemory != -1 ) {
	if ((*target).NodeMemory < jobd["JobDescription"]["Resources"]["IndividualPhysicalMemory"] ||
	    (*target).NodeMemory < jobd["JobDescription"]["Resources"]["TotalPhysicalMemory"] ||
	    (*target).NodeMemory < jobd["JobDescription"]["Application"]["POSIXApplication"]["MemoryLimit"])
	  continue;
      }
      
      if ( (*target).MainMemorySize != -1 ) {
	if ((*target).MainMemorySize < jobd["JobDescription"]["Resources"]["IndividualPhysicalMemory"] ||
	    (*target).MainMemorySize < jobd["JobDescription"]["Resources"]["TotalPhysicalMemory"] ||
	    (*target).MainMemorySize < jobd["JobDescription"]["Application"]["POSIXApplication"]["MemoryLimit"])
	  continue;
      }

      //if the target made it down here, then insert into list of possible targets
      PossibleTargets.push_back(*target);	     
    
    } //end loop over all found targets
    
    logger.msg(VERBOSE, "Number of possible targets : %d",PossibleTargets.size());
    current = PossibleTargets.begin();    
    PreFilteringDone = true;
    TargetSortingDone = false;

  }
    
  ExecutionTarget& Broker::GetBestTarget(bool &EndOfList) {

    if (!TargetSortingDone){
      logger.msg(VERBOSE, "Target sorting not done, sorting them now");
      SortTargets();
    }
    
    std::vector<Arc::ExecutionTarget>::iterator ret_pointer;
    ret_pointer = current;
    current++;  
    
    if (ret_pointer == PossibleTargets.end()) {
      EndOfList = true;  
    }
    
    return *ret_pointer;
  }
  
} // namespace Arc



