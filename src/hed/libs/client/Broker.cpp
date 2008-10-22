#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/client/Broker.h>

namespace Arc {
  
   std::string UpToLow(std::string str) {
        for (int i=0;i<strlen(str.c_str());i++)
             if (str[i] >= 0x41 && str[i] <= 0x5A)
                   str[i] = str[i] + 0x20;
        return str;
  }

  bool Value_Matching(int value, Arc::XMLNode node){
     if ( node["LowerBoundedRange"] ) {
           return ( ( !node["LowerBoundedRange"].Attribute("exclusiveBound") && ( value >= (int)node["LowerBoundedRange"] ) ) ||
                        (  node["LowerBoundedRange"].Attribute("exclusiveBound") && ( value >    (int)node["LowerBoundedRange"] )) );
     }
     else if ( node["UpperBoundedRange"] ) {
           return ( ( !node["UpperBoundedRange"].Attribute("exclusiveBound") && ( value <= (int)node["UpperBoundedRange"] ) ) ||
                        (  node["UpperBoundedRange"].Attribute("exclusiveBound") && ( value <    (int)node["UpperBoundedRange"] )) );
     }
     else if ( node["Exact"] ) {
           return ( ( ((double)node["Exact"] - node["Exact"].Attribute("epsilon") ) <= (double)value ) &&
                        ( (double)value <= ((double)node["Exact"] - node["Exact"].Attribute("epsilon") ) ) );
     }
     else if ( node["Range"] ) {
           return ( ( !node["Range"]["LowerBound"].Attribute("exclusiveBound") && ( value >= (int)node["Range"]["LowerBound"] ) ) ||
                        (  node["Range"]["LowerBound"].Attribute("exclusiveBound") && ( value >    (int)node["Range"]["LowerBound"] )) ) &&
                      ( ( !node["Range"]["UpperBound"].Attribute("exclusiveBound") && ( value <= (int)node["Range"]["UpperBound"] ) ) ||
                        (  node["Range"]["UpperBound"].Attribute("exclusiveBound") && ( value <    (int)node["Range"]["UpperBound"] )) );
     }
     return true;
  }

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

                //JSDL Operating System Types: Unknow, MACOS, LINUX, Solaris, SunOS, Windows_XP, FreeBSD, etc
	//GLUE2 OSFamily_t: linux, macosx, windows, solaris
                //TODO: check this clever then now
	if ( (*target).OSFamily != "" ) {
	  if ( Arc::UpToLow((*target).OSFamily) != Arc::UpToLow(jobd["JobDescription"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]) )
	    continue;
	}

	//GLUE2 OSName_t: scientificlinux, scientificlinuxcern, ubuntu, debian, centos, fedora, rhes, mandrake, suse, leopard, windowsxp, windowsvista
                //TODO: check this clever then now
	if ( (*target).OSName != "" ) {
	  if ( Arc::UpToLow((*target).OSName) != Arc::UpToLow(jobd["JobDescription"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]) )	
	    continue;
	}

	if ( (*target).CPUModel!= "" ) {
	  if ( Arc::UpToLow((*target).CPUModel) != Arc::UpToLow(jobd["JobDescription"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"]) )
	    continue;
	}
	
                //TODO: check this clever then now
	if ( (*target).Reservation ) {
	  if ( (*target).Reservation != jobd["JobDescription"]["Resources"]["ExclusiveExecution"] )
	    continue;
	}


//TODO: check the Candidates element (HostName, Queue)
	if ( (*target).MaxDiskSpace != -1 ) {
	  if ( !Arc::Value_Matching ( (*target).MaxDiskSpace, jobd["JobDescription"]["Resources"]["FileSystem"]["DiskSpace"] ) || 
	       !Arc::Value_Matching ( (*target).MaxDiskSpace,  jobd["JobDescription"]["Resources"]["IndividualDiskSpace"] ) || 
	       !Arc::Value_Matching ( (*target).MaxDiskSpace,  jobd["JobDescription"]["Resources"]["TotalDiskSpace"]) )
	    continue;
	}
	
	if ( (*target).TotalPhysicalCPUs != -1 ) {
	  if ( !Arc::Value_Matching ( (*target).TotalPhysicalCPUs, jobd["JobDescription"]["Resources"]["IndividualCPUCount"] )||
	       !Arc::Value_Matching ( (*target).TotalPhysicalCPUs, jobd["JobDescription"]["Resources"]["TotalCPUCount"]) )
	    continue;
	}
	
	if ( (*target).TotalLogicalCPUs != -1 ) {
	  if ( !Arc::Value_Matching ( (*target).TotalLogicalCPUs, jobd["JobDescription"]["Resources"]["IndividualCPUCount"] )||
	       !Arc::Value_Matching ( (*target).TotalLogicalCPUs, jobd["JobDescription"]["Resources"]["TotalCPUCount"]) )
	    continue;
	}
	
	if ( (*target).CPUClockSpeed != -1 ) {
	  if ( !Arc::Value_Matching ( (*target).CPUClockSpeed, jobd["JobDescription"]["Resources"]["IndividualCPUSpeed"]) )
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
	if ( !Arc::Value_Matching( (int)(*target).MinCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"] )||
	     !Arc::Value_Matching( (int)(*target).MinCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"] )|| 
	     (int)(*target).MinCPUTime.GetPeriod() > jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (int)(*target).MaxCPUTime.GetPeriod() != -1 ) {
	if ( !Arc::Value_Matching( (int)(*target).MaxCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"] )||
	     !Arc::Value_Matching( (int)(*target).MaxCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"] )||
	     (int)(*target).MaxCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (int)(*target).MaxTotalCPUTime.GetPeriod() != -1 ) {
	if ( !Arc::Value_Matching ( (int)(*target).MaxTotalCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"] )||
	     !Arc::Value_Matching ( (int)(*target).MaxTotalCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"] )||
	     (int)(*target).MaxTotalCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (int)(*target).DefaultCPUTime.GetPeriod() != -1 ) {
	if ( !Arc::Value_Matching ( (int)(*target).DefaultCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"] )||
	     !Arc::Value_Matching ( (int)(*target).DefaultCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"] )||
	     (int)(*target).DefaultCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (*target).MaxMemory != -1 ) {
	if (!Arc::Value_Matching ( (*target).MaxMemory, jobd["JobDescription"]["Resources"]["IndividualPhysicalMemory"] )||
	    !Arc::Value_Matching ( (*target).MaxMemory, jobd["JobDescription"]["Resources"]["TotalPhysicalMemory"] )||
	    (*target).MaxMemory < jobd["JobDescription"]["Application"]["POSIXApplication"]["MemoryLimit"])
	  continue;
      }
      
      if ( (*target).NodeMemory != -1 ) {
	if (!Arc::Value_Matching ( (*target).NodeMemory, jobd["JobDescription"]["Resources"]["IndividualPhysicalMemory"] )||
	    !Arc::Value_Matching ( (*target).NodeMemory, jobd["JobDescription"]["Resources"]["TotalPhysicalMemory"] )||
	    (*target).NodeMemory < jobd["JobDescription"]["Application"]["POSIXApplication"]["MemoryLimit"])
	  continue;
      }
      
      if ( (*target).MainMemorySize != -1 ) {
	if (!Arc::Value_Matching ( (*target).MainMemorySize, jobd["JobDescription"]["Resources"]["IndividualPhysicalMemory"] )||
	    !Arc::Value_Matching ( (*target).MainMemorySize, jobd["JobDescription"]["Resources"]["TotalPhysicalMemory"] )||
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



