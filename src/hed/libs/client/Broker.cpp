#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/client/Broker.h>

namespace Arc {

  Arc::Logger Broker::logger(Arc::Logger::getRootLogger(), "broker");
  
  Broker::Broker(Config *cfg) : ACC(cfg),
				PreFilteringDone(false),
				TargetSortingDone(false){}

  Broker::~Broker() {}

  void Broker::PreFilterTargets(Arc::TargetGenerator& targen,  Arc::JobDescription jd) {
 
	jd.getInnerRepresentation(jir);

    for (std::list<Arc::ExecutionTarget>::const_iterator target =	\
	   targen.FoundTargets().begin(); target != targen.FoundTargets().end(); \
	   target++) {  

	  // TODO: if the type is a number than we need to check the measure

       if (!((std::string)(jir.EndPointURL.Host())).empty()) {
           if (!((std::string)((*target).url.Host())).empty()) {
	        if (((std::string)(*target).url.Host()) != jir.EndPointURL.Host()) { // Example: knowarc1.grid.niif.hu 
			   continue;
            }
          }
       }
    
      // CEType is not decided yet, this will be modify to other

       if ((int)jir.ProcessingStartTime.GetTime() != -1) {
           if ((int)(*target).DowntimeStarts.GetTime() != -1 && (int)(*target).DowntimeEnds.GetTime() != -1) {
	        if ((int)(*target).DowntimeStarts.GetTime() > (int)jir.ProcessingStartTime.GetTime() || 
               (int)(*target).DowntimeEnds.GetTime() < (int)jir.ProcessingStartTime.GetTime()) { // 3423434 
			   continue;
            }
           }
       }

       if (!(*target).HealthState.empty()) {

          // Enumeration for healthstate: ok, critical, other, unknown, warning

           if ((*target).HealthState != "ok") {
			   continue;
           }
       }

       if (!jir.CEType.empty()) {
           if (!(*target).ImplementationName.empty()) {
	        if ((*target).ImplementationName != jir.CEType) { // Example: A-REX, ARC0 
			   continue;
            }
           }
       }

       if (!jir.QueueName.empty()) {
           if (!(*target).MappingQueue.empty()) {
	        if ((*target).MappingQueue != jir.QueueName) { // Example: gridlong 
			   continue;
            }
           }
       }

       if ((int)jir.TotalWallTime.GetPeriod() != -1) {
	        if ((int)(*target).MaxWallTime.GetPeriod() != -1) { // Example: 123
               if (!((int)(*target).MaxWallTime.GetPeriod()) >= (int)jir.TotalWallTime.GetPeriod()) { 
			       continue;
               }
            }
	        if ((int)(*target).MinWallTime.GetPeriod() != -1) { // Example: 123
               if (!((int)(*target).MinWallTime.GetPeriod()) > (int)jir.TotalWallTime.GetPeriod()) { 
			       continue;
               }
            }
       }

       if ((int)jir.TotalCPUTime.GetPeriod() != -1) {
	        if ((int)(*target).MaxCPUTime.GetPeriod() != -1) { // Example: 456
               if (!((int)(*target).MaxCPUTime.GetPeriod()) >= (int)jir.TotalCPUTime.GetPeriod()) {
			       continue;
               }
            }
	        if ((int)(*target).MinCPUTime.GetPeriod() != -1) { // Example: 456
               if (!((int)(*target).MinCPUTime.GetPeriod()) > (int)jir.TotalCPUTime.GetPeriod()) {
			       continue;
               }
            }
       }

       if (jir.IndividualPhysicalMemory != -1) {
	        if ((*target).NodeMemory != -1) { // Example: 678
               if (!((*target).NodeMemory >= jir.IndividualPhysicalMemory)) {
			       continue;
               }
            }
	        else if ((*target).MaxMainMemory != -1) { // Example: 678
               if (!((*target).MaxMainMemory >= jir.IndividualPhysicalMemory)) {
			       continue;
               }
            }
       }

       if (jir.IndividualVirtualMemory != -1) {
	        if ((*target).MaxVirtualMemory != -1) { // Example: 678
               if (!((*target).MaxVirtualMemory >= jir.IndividualVirtualMemory)) {
			       continue;
               }
            }
       }

       if (!jir.Platform.empty()) {
           if (!(*target).Platform.empty()) { // Example: i386
	        if ((*target).Platform != jir.Platform) {  
			   continue;
            }
           }
       }

       if (!jir.OSFamily.empty()) {
           if (!(*target).OSFamily.empty()) { // Example: linux
	        if ((*target).OSFamily != jir.OSFamily) {  
			   continue;
            }
           }
       }

       if (!jir.OSName.empty()) {
           if (!(*target).OSName.empty()) { // Example: ubuntu
	        if ((*target).OSName != jir.OSName) {  
			   continue;
            }
           }
       }

       if (!jir.OSVersion.empty()) {
           if (!(*target).OSVersion.empty()) { // Example: ?
			// TODO finish the version check
	        if ((*target).OSVersion != jir.OSVersion) {  
			   continue;
            }
           }
       }

      if (!jir.RunTimeEnvironment.empty()) {
          if (!(*target).ApplicationEnvironments.empty()) {
  
          // TODO: finish this runtimeenvironment prefiltering part
/*
          std::list<Arc::RunTimeEnvironmentType>::const_iterator iter;
        for (iter = RunTimeEnvironment.begin(); iter != RunTimeEnvironment.end(); iter++){
           
        std::coddut << Arc::IString(" RunTimeEnvironment.Name: %s", (*iter).Name) << std::endl;
            std::list<std::string>::const_iterator it;
            for (it = (*iter).Version.begin(); it != (*iter).Version.end(); it++){
            std::cout << Arc::IString("    RunTimeEnvironment.Version: %s", (*it)) << std::endl;
           }
    
  */      }    
      }

       if (!jir.NetworkInfo.empty()) {
           if (!(*target).NetworkInfo.empty()) { // Example: infiniband
	        if ((*target).NetworkInfo != jir.NetworkInfo) {  
			   continue;
            }
           }
       }

       if (jir.SessionDiskSpace != -1) {
	        if ((*target).MaxDiskSpace != -1) { // Example: 5656
               if (!((*target).MaxDiskSpace >= jir.SessionDiskSpace)) {
			       continue;
               }
            }
	        else if ((*target).WorkingAreaTotal != -1) { // Example: 5656
               if (!((*target).WorkingAreaTotal >= jir.SessionDiskSpace)) {
			       continue;
               }
            }
           
       }

       if (jir.DiskSpace != -1 && jir.CacheDiskSpace != -1) {
	        if ((*target).MaxDiskSpace != -1) { // Example: 5656
               if (!((*target).MaxDiskSpace >= (jir.DiskSpace - jir.CacheDiskSpace))) {
			       continue;
               }
            }
	        else if ((*target).WorkingAreaTotal != -1) { // Example: 5656
               if (!((*target).WorkingAreaTotal >= (jir.DiskSpace - jir.CacheDiskSpace))) {
			       continue;
               }
            }
       }

       if (jir.DiskSpace != -1) {
	        if ((*target).MaxDiskSpace != -1) { // Example: 5656
               if (!((*target).MaxDiskSpace >= jir.DiskSpace)) {
			       continue;
               }
            }
	        else if ((*target).WorkingAreaTotal != -1) { // Example: 5656
               if (!((*target).WorkingAreaTotal >= jir.DiskSpace)) {
			       continue;
               }
            }
       }

       if (jir.CacheDiskSpace != -1) {
	        if ((*target).CacheTotal != -1) { // Example: 5656
               if (!((*target).CacheTotal >= jir.CacheDiskSpace)) {
			       continue;
               }
            }
       }

       if (jir.Slots != -1) {
	        if ((*target).TotalSlots != -1) { // Example: 5656
               if (!((*target).TotalSlots >= jir.Slots)) {
			       continue;
               }
            }
	        else if ((*target).MaxSlotsPerJob != -1) { // Example: 5656
               if (!((*target).MaxSlotsPerJob >= jir.Slots)) {
			       continue;
               }
            }
       }

       if (jir.NumberOfProcesses != -1) {
	        if ((*target).TotalSlots != -1) { // Example: 5656
               if (!((*target).TotalSlots >= jir.NumberOfProcesses)) {
			       continue;
               }
            }
	        else if ((*target).MaxSlotsPerJob != -1) { // Example: 5656
               if (!((*target).MaxSlotsPerJob >= jir.NumberOfProcesses)) {
			       continue;
               }
            }
       }

       if ((int)jir.SessionLifeTime.GetPeriod() != -1) {
	        if ((int)(*target).MaxWallTime.GetPeriod() != -1) { // Example: 123
               if (!((int)(*target).MaxWallTime.GetPeriod()) >= (int)jir.SessionLifeTime.GetPeriod()) { 
			       continue;
               }
            }
       }

       if (jir.InBound) {
	        if (!(*target).ConnectivityIn) { // Example: false (boolean)
			       continue;
            }
       }

       if (jir.OutBound) {
	        if (!(*target).ConnectivityOut) { // Example: false (boolean)
			       continue;
            }
       }
    

       if (!jir.ReferenceTime.value.empty()) {
           if (!(*target).Benchmarks.empty()) { // Example: benchmark="frequency" value="2.8Ghz"
			// TODO finish the benchmark test
			 continue;
           }
       }

      PossibleTargets.push_back(*target);	     

    } //end loop over all found targets
    
    logger.msg(DEBUG, "Possible targets after prefiltering: %d",PossibleTargets.size());
    
    for(std::vector<ExecutionTarget>::iterator iter = PossibleTargets.begin();
	iter != PossibleTargets.end(); iter++){
      logger.msg(DEBUG, "Cluster: %s", iter->DomainName);
      logger.msg(DEBUG, "Health State: %s", iter->HealthState);
    }
    
    PreFilteringDone = true;
    TargetSortingDone = false;

  }

  ExecutionTarget& Broker::GetBestTarget(bool &EndOfList) {

    if(PossibleTargets.size() <= 0) {
      EndOfList = true;
      return *PossibleTargets.end();
    }

    if (!TargetSortingDone){
      logger.msg(VERBOSE, "Target sorting not done, sorting them now");
      SortTargets();
      current = PossibleTargets.begin();    
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



