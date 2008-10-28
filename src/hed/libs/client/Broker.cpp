#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/client/Broker.h>

namespace Arc {
  
  bool Value_Matching(double value, Arc::XMLNode node) {
     if (node["LowerBoundedRange"]) {
           return ((!node["LowerBoundedRange"].Attribute("exclusiveBound") && (value >= (double)node["LowerBoundedRange"])) || (node["LowerBoundedRange"].Attribute("exclusiveBound") && (value > (double)node["LowerBoundedRange"])));
     }
     else if (node["UpperBoundedRange"]) {
           return ((!node["UpperBoundedRange"].Attribute("exclusiveBound") && (value <= (double)node["UpperBoundedRange"])) || (node["UpperBoundedRange"].Attribute("exclusiveBound") && (value < (double)node["UpperBoundedRange"])));
     }
     else if (node["Exact"]) {
           return ((((double)node["Exact"] - node["Exact"].Attribute("epsilon")) <= value) && (value <= ((double)node["Exact"] + node["Exact"].Attribute("epsilon"))));
     }
     else if (node["Range"]) {
           return ((!node["Range"]["LowerBound"].Attribute("exclusiveBound") && (value >= (double)node["Range"]["LowerBound"])) || (node["Range"]["LowerBound"].Attribute("exclusiveBound") && (value > (double)node["Range"]["LowerBound"]))) && ((!node["Range"]["UpperBound"].Attribute("exclusiveBound") && (value <= (double)node["Range"]["UpperBound"])) || (node["Range"]["UpperBound"].Attribute("exclusiveBound") && (value < (double)node["Range"]["UpperBound"])));
     }
     return true;
  }

  bool OSCheck(std::string ostype, Arc::XMLNode node) {

	// INFO: if you would like to add a new operating system type than you must add it to these two arrays and increase the OS_ARRAY_SIZE constant value
   // You must use lower case characters in the arrays for correct working

	 const int OS_ARRAY_SIZE = 5;

	 std::string JSDLOSTypes[] = {"macos", "linux", "solaris", "sunos", "windows_xp"};  

     std::string GLUE2OSTypes[] = {"macosx", "linux", "solaris", "solaris", "windows"};

	 StringManipulator sm;

	 std::string job_jsdl_os_type_tmp;
	 std::string exec_tar_glue2_os_type_tmp;

	 for (int i = 0; OS_ARRAY_SIZE - 1; i++) {
		job_jsdl_os_type_tmp = sm.trim((std::string)node);
		job_jsdl_os_type_tmp = sm.toLowerCase(job_jsdl_os_type_tmp);
		exec_tar_glue2_os_type_tmp = sm.trim(ostype);
		exec_tar_glue2_os_type_tmp = sm.toLowerCase(exec_tar_glue2_os_type_tmp);

		if (JSDLOSTypes[i] == job_jsdl_os_type_tmp && GLUE2OSTypes[i] == exec_tar_glue2_os_type_tmp)
			return true;
		else if ( i  == OS_ARRAY_SIZE - 1)
			return false;
     }

     /*

       GLUE2 OSFamily_t open enumeration:

               Value                        Description

              linux          Family of operating systems based on Linux kernel
              macosx         Family of operating systems based on MacOS X
              windows        Family of operating systems based on Windows
              solaris        Family of operating systems based on Solaris

     */

  }

  bool ArchCheck(std::string archtype, Arc::XMLNode node) {

	// INFO: if you would like to add a new architecture than you must add it to these two arrays and increase the ARCH_ARRAY_SIZE constant value
   // You must use lower case characters in the arrays for correct working

	 const int ARCH_ARRAY_SIZE = 9;
	
     std::string JSDLArchTypes[] = {"sparc", "powerpc", "x86", "x86_32", "x86_64", "parisc", "mips", "ia64", "arm"};

     std::string GLUE2ArchTypes[] = {"sparc", "powerpc", "i386", "i386", "itanium", "other", "other", "itanium", "other"};

	 StringManipulator sm;

	 std::string job_jsdl_arch_type_tmp;
	 std::string exec_tar_glue2_arch_type_tmp;

	 for (int i = 0; ARCH_ARRAY_SIZE - 1; i++) {
		job_jsdl_arch_type_tmp = sm.trim((std::string)node);
		job_jsdl_arch_type_tmp = sm.toLowerCase(job_jsdl_arch_type_tmp);
		exec_tar_glue2_arch_type_tmp = sm.trim(archtype);
		exec_tar_glue2_arch_type_tmp = sm.toLowerCase(exec_tar_glue2_arch_type_tmp);

		if (JSDLArchTypes[i] == job_jsdl_arch_type_tmp && GLUE2ArchTypes[i] == exec_tar_glue2_arch_type_tmp)
			return true;
		else if ( i  == ARCH_ARRAY_SIZE - 1)
			return false;
     }

   /*

			JSDL Architecture types:

             Normative JSDL             Name Definition

              sparc           A SPARC architecture processor
              powerpc         A PowerPC architecture processor
              x86             An Intel Architecture processor derived from the 8086 chip set
              x86_32 		  An x86 processor capable of 32-bit processing mode
              x86_64          An x86 processor capable of 64-bit processing mode
              parisc          A PARISC architecture processor
			  mips 			  A MIPS architecture processor
			  ia64 			  An Intel Architecture 64-bit processor
			  arm             An ARM processor
              other           A value not defined by this enumeration


              GLUE2 Platform_t open enumeration:

			  Value 			Description

              i386 		Intel 386 architecture
			  amd64 	AMD 64bit architecture
              itanium   Intel 64-bit architecture
              powerpc   PowerPC architecture
              sparc     SPARC architecture

              GLUE2 extension (Created by Gabor Roczei):
              other     A value not defined by this enumeration

   */

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
      
      // These arguments are dependence of the Resource item.

      if ( jobd["JobDescription"]["Resources"] ) {

		// TODO: we need a better ExclusiveExecution filtering

	    if (jobd["JobDescription"]["Resources"]["ExclusiveExecution"]) {
	        StringManipulator sm;
	        std::string reservation_tmp;

	        reservation_tmp = sm.trim((std::string)jobd["JobDescription"]["Resources"]["ExclusiveExecution"]);
	        reservation_tmp = sm.toLowerCase(reservation_tmp);
	  
	        if ((*target).Reservation && reservation_tmp == "true") {
		       // this target is good for us
            }
	        else
	           continue;
	     }

      if (jobd["JobDescription"]["Resources"]["CandidateHosts"]) {

	        StringManipulator sm;
   			bool good = false;

	        for( int i = 0; (bool)(jobd["JobDescription"]["Resources"]["CandidateHosts"]["HostName"][i]) ; i++ ) {
              std::string value = (std::string)jobd["JobDescription"]["Resources"]["CandidateHosts"]["HostName"][i];
			  value = sm.trim(value);
	        if ((*target).DomainName == value) {
			   good = true;
            }
          } 
		  if (!good) continue;
	  } 
      else if (jobd["JobDescription"]["Resources"]["CandidateTarget"]) {

		 // TODO: jsdl-arc namespace check

	        StringManipulator sm;
   			bool good = false;

	        for (int i=0; (bool)(jobd["JobDescription"]["Resources"]["CandidateTarget"]["HostName"][i]) ; i++ ) {
              std::string value = (std::string)jobd["JobDescription"]["Resources"]["CandidateTarget"]["HostName"][i];
			  value = sm.trim(value);
	        if ((*target).DomainName == value) {
			   good = true;
            }
          }
 
		  if (!good) continue;

		  good = false;
 
		 // TODO: jsdl-arc namespace check

	      for (int i = 0; (bool)(jobd["JobDescription"]["Resources"]["CandidateTarget"]["QueueName"][i]) ; i++ ) {
              std::string value = (std::string)jobd["JobDescription"]["Resources"]["CandidateTarget"]["QueueName"][i];
			  value = sm.trim(value);
	         if ((*target).MappingQueue == value) {
			   good = true;
            }
          } 
		  if (!good) continue;
	  }

	  if ( (*target).OSFamily != "" && jobd["JobDescription"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]) {
		      if (!OSCheck((*target).OSFamily, jobd["JobDescription"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"])) {
				  continue;
              }    	
        }    	

	  if ( (*target).CPUModel != "" && jobd["JobDescription"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"]) {
		      if (!ArchCheck((*target).CPUModel, jobd["JobDescription"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"])) {
				  continue;
              }    	
	  }
	
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
      
   } // End of the Resource elements check
      
      //These arguments are dependence of the Application item.

      if (jobd["JobDescription"]["Application"]) {
	
	if ((int)(*target).MinWallTime.GetPeriod() != -1) {
	  if ((int)(*target).MinWallTime.GetPeriod() > jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
	if ((int)(*target).MaxWallTime.GetPeriod() != -1) {
	  if ((int)(*target).MaxWallTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
	if ((int)(*target).MaxTotalWallTime.GetPeriod() != -1) {
	  if ((int)(*target).MaxTotalWallTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
	if ((int)(*target).DefaultWallTime.GetPeriod() != -1) {
	  if ((int)(*target).DefaultWallTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"])
	    continue;
	}
	
      }
      
      //These arguments are dependence of the Application and the Resource items.

      if ((int)(*target).MinCPUTime.GetPeriod() != -1 ) {
	if ( !Arc::Value_Matching( (int)(*target).MinCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"] )||
	     !Arc::Value_Matching( (int)(*target).MinCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"] )|| 
	     (int)(*target).MinCPUTime.GetPeriod() > jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ((int)(*target).MaxCPUTime.GetPeriod() != -1) {
	if (!Arc::Value_Matching((int)(*target).MaxCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"]) ||
	     !Arc::Value_Matching((int)(*target).MaxCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"] ) ||
	     (int)(*target).MaxCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ((int)(*target).MaxTotalCPUTime.GetPeriod() != -1 ) {
	if (!Arc::Value_Matching ((int)(*target).MaxTotalCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"] ) ||
	     !Arc::Value_Matching ((int)(*target).MaxTotalCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"]) ||
	     (int)(*target).MaxTotalCPUTime.GetPeriod() < jobd["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"])
	  continue;
      }
      
      if ( (int)(*target).DefaultCPUTime.GetPeriod() != -1 ) {
	if ( !Arc::Value_Matching ((int)(*target).DefaultCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["IndividualCPUTime"] )||
	     !Arc::Value_Matching ((int)(*target).DefaultCPUTime.GetPeriod(), jobd["JobDescription"]["Resources"]["TotalCPUTime"] )||
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



