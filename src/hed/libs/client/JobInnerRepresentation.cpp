#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/IString.h>
#include <arc/ArcConfig.h>
#include <arc/client/JobInnerRepresentation.h>

namespace Arc {

  JobInnerRepresentation::JobInnerRepresentation() { Reset(); }

  JobInnerRepresentation::~JobInnerRepresentation() {}

  void JobInnerRepresentation::Print(bool longlist) const {

    std::cout << Arc::IString(" Executable: %s", Executable) << std::endl;
    std::cout << Arc::IString(" Grid Manager Log Directory: %s", LogDir) << std::endl;
    if (!JobName.empty())
      std::cout << Arc::IString(" JobName: %s", JobName) << std::endl;
    if (!Description.empty())
      std::cout << Arc::IString(" Description: %s", Description) << std::endl;
    if (!JobProject.empty())
      std::cout << Arc::IString(" JobProject: %s", JobProject) << std::endl;
    if (!JobType.empty())
      std::cout << Arc::IString(" JobType: %s", JobType) << std::endl;
    if (!JobCategory.empty())
      std::cout << Arc::IString(" JobCategory: %s", JobCategory) << std::endl;

    if (longlist) {
      if (!UserTag.empty()) {
        std::list<std::string>::const_iterator iter;
        for (iter = UserTag.begin(); iter != UserTag.end(); iter++)
	  std::cout << Arc::IString(" UserTag: %s", *iter) << std::endl;
      }
      if (!OptionalElement.empty()) {
        std::list<std::string>::const_iterator iter;
        for (iter = OptionalElement.begin(); iter != OptionalElement.end(); iter++)
	  std::cout << Arc::IString(" OptionalElement: %s", *iter) << std::endl;
      }
      if (!Author.empty())
        std::cout << Arc::IString(" Author: %s", Author) << std::endl;
      if (DocumentExpiration != -1)
	std::cout << Arc::IString(" DocumentExpiration: %s",
				  (std::string)DocumentExpiration) << std::endl;
      if (!Rank.empty())
        std::cout << Arc::IString(" Rank: %s", Rank) << std::endl;
      if (FuzzyRank)
        std::cout << " FuzzyRank: true" << std::endl;
      if (!Argument.empty()) {
        std::list<std::string>::const_iterator iter;
        for (iter = Argument.begin(); iter != Argument.end(); iter++)
	  std::cout << Arc::IString(" Argument: %s", *iter) << std::endl;
      }
      if (!Input.empty())
	std::cout << Arc::IString(" Input: %s", Input) << std::endl;
      if (!Output.empty())
	std::cout << Arc::IString(" Output: %s", Output) << std::endl;
      if (!Error.empty())
	std::cout << Arc::IString(" Error: %s", Error) << std::endl;
      if ( bool(RemoteLogging) )
        std::cout << Arc::IString(" RemoteLogging: %s", RemoteLogging.fullstr()) << std::endl;
      if (!Environment.empty()) {
        std::list<Arc::EnvironmentType>::const_iterator iter;
        for (iter = Environment.begin(); iter != Environment.end(); iter++){
	  std::cout << Arc::IString(" Environment.name: %s", (*iter).name_attribute) << std::endl;
	  std::cout << Arc::IString(" Environment: %s", (*iter).value) << std::endl;
        }
      }
      if (LRMSReRun > 0)
	std::cout << Arc::IString(" LRMSReRun: %d", LRMSReRun) << std::endl;
      if (!Prologue.Name.empty())
	std::cout << Arc::IString(" Prologue: %s", Prologue.Name) << std::endl;
      if (!Prologue.Arguments.empty()){
        std::list<std::string>::const_iterator iter;
        for (iter = Prologue.Arguments.begin(); iter != Prologue.Arguments.end(); iter++){
	  std::cout << Arc::IString(" Prologue.Arguments: %s", *iter) << std::endl;
        }
      }
      if (!Epilogue.Name.empty())
	std::cout << Arc::IString(" Epilogue: %s", Epilogue.Name) << std::endl;
      if (!Epilogue.Arguments.empty()){
        std::list<std::string>::const_iterator iter;
        for (iter = Epilogue.Arguments.begin(); iter != Epilogue.Arguments.end(); iter++){
	  std::cout << Arc::IString(" Epilogue.Arguments: %s", *iter) << std::endl;
        }
      }
      if (SessionLifeTime != -1)
	std::cout << Arc::IString(" SessionLifeTime: %s",
				  (std::string)SessionLifeTime) << std::endl;
      if ( bool( AccessControl) ){
        std::string str;
        AccessControl.GetXML(str, true);
	std::cout << Arc::IString(" AccessControl: %s", str) << std::endl;
      }
      if (ProcessingStartTime != -1)
	std::cout << Arc::IString(" ProcessingStartTime: %s",
				  (std::string)ProcessingStartTime.str()) << std::endl;
      if (!Notification.empty()){
         for (std::list<Arc::NotificationType>::const_iterator it=Notification.begin(); it!=Notification.end(); it++) {
	     std::cout << Arc::IString(" Notification element: ") << std::endl;
             for (std::list<std::string>::const_iterator iter=(*it).Address.begin();
                            iter!=(*it).Address.end(); iter++) {
	         std::cout << Arc::IString("    Address: %s", (*iter)) << std::endl;
            }
             for (std::list<std::string>::const_iterator iter=(*it).State.begin();
                            iter!=(*it).State.end(); iter++) {
	         std::cout << Arc::IString("    State: %s", (*iter)) << std::endl;
           }
         }
      }
      if ( bool(CredentialService))
        std::cout << Arc::IString(" CredentialService: %s", CredentialService.str()) << std::endl;
      if (Join)
        std::cout << " Join: true" << std::endl;
      if (TotalCPUTime != -1)
	std::cout << Arc::IString(" Total CPU Time: %s",
				  (std::string)TotalCPUTime) << std::endl;
      if (IndividualCPUTime != -1)
	std::cout << Arc::IString(" Individual CPU Time: %s",
				  (std::string)IndividualCPUTime) << std::endl;
      if (TotalWallTime != -1)
	std::cout << Arc::IString(" Total Wall Time: %s",
				  (std::string)TotalWallTime) << std::endl;
      if (IndividualWallTime != -1)
	std::cout << Arc::IString(" Individual Wall Time: %s",
				  (std::string)IndividualWallTime) << std::endl;
//ReferenceTime
      if (ExclusiveExecution)
        std::cout << " ExclusiveExecution: true" << std::endl;
      if (!NetworkInfo.empty())
	std::cout << Arc::IString(" NetworkInfo: %s", NetworkInfo) << std::endl;
      if (!OSFamily.empty())
	std::cout << Arc::IString(" OSFamily: %s", OSFamily) << std::endl;
      if (!OSName.empty())
	std::cout << Arc::IString(" OSName: %s", OSName) << std::endl;
      if (!OSVersion.empty())
	std::cout << Arc::IString(" OSVersion: %s", OSVersion) << std::endl;
      if (!Platform.empty())
	std::cout << Arc::IString(" Platform: %s", Platform) << std::endl;
      if (IndividualPhysicalMemory > 0)
	std::cout << Arc::IString(" IndividualPhysicalMemory: %d", IndividualPhysicalMemory) << std::endl;
      if (IndividualVirtualMemory > 0)
	std::cout << Arc::IString(" IndividualVirtualMemory: %d", IndividualVirtualMemory) << std::endl;
      if (IndividualDiskSpace > 0)
	std::cout << Arc::IString(" IndividualDiskSpace: %d", IndividualDiskSpace) << std::endl;
      if (DiskSpace > 0)
	std::cout << Arc::IString(" DiskSpace: %d", DiskSpace) << std::endl;
      if (CacheDiskSpace > 0)
	std::cout << Arc::IString(" CacheDiskSpace: %d", CacheDiskSpace) << std::endl;
      if (SessionDiskSpace > 0)
	std::cout << Arc::IString(" SessionDiskSpace: %d", SessionDiskSpace) << std::endl;
      if (bool(EndPointURL))
        std::cout << Arc::IString(" EndPointURL: %s", EndPointURL.str()) << std::endl;
      if (!QueueName.empty())
	std::cout << Arc::IString(" QueueName: %s", QueueName) << std::endl;
      if (!CEType.empty())
	std::cout << Arc::IString(" CEType: %s", CEType) << std::endl;
      if (NumberOfProcesses > 0)
	std::cout << Arc::IString(" NumberOfProcesses: %d", NumberOfProcesses) << std::endl;
      if (ProcessPerHost > 0)
	std::cout << Arc::IString(" ProcessPerHost: %d", ProcessPerHost) << std::endl;
      if (ThreadPerProcesses > 0)
	std::cout << Arc::IString(" ThreadPerProcesses: %d", ThreadPerProcesses) << std::endl;
      if (!SPMDVariation.empty())
	std::cout << Arc::IString(" SPMDVariation: %s", SPMDVariation) << std::endl;
      if (!RunTimeEnvironment.empty()) {
        std::list<Arc::RunTimeEnvironmentType>::const_iterator iter;
        for (iter = RunTimeEnvironment.begin(); iter != RunTimeEnvironment.end(); iter++){
	    std::cout << Arc::IString(" RunTimeEnvironment.Name: %s", (*iter).Name) << std::endl;
            std::list<std::string>::const_iterator it;
            for (it = (*iter).Version.begin(); it != (*iter).Version.end(); it++){
	        std::cout << Arc::IString("    RunTimeEnvironment.Version: %s", (*it)) << std::endl;
            }
        }
      }
      if (!InBound.empty())
	std::cout << Arc::IString(" InBound: %s", InBound) << std::endl;
      if (!OutBound.empty())
	std::cout << Arc::IString(" OutBound: %s", OutBound) << std::endl;
      if (!File.empty()) {
        std::list<Arc::FileType>::const_iterator iter;
        for (iter = File.begin(); iter != File.end(); iter++){
	   std::cout << Arc::IString(" File element:") << std::endl;
	   std::cout << Arc::IString("     Name: %s", (*iter).Name) << std::endl;
           std::list<Arc::SourceType>::const_iterator it_source;
           for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
	       std::cout << Arc::IString("     Source.URI: %s", (*it_source).URI.fullstr()) << std::endl;
               if ((*it_source).Threads > 0)
	          std::cout << Arc::IString("     Source.Threads: %d", (*it_source).Threads) << std::endl;
           }
           std::list<Arc::TargetType>::const_iterator it_target;
           for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
	       std::cout << Arc::IString("     Target.URI: %s", (*it_target).URI.fullstr()) << std::endl;
               if ((*it_target).Threads > 0)
	          std::cout << Arc::IString("     Target.Threads: %d", (*it_target).Threads) << std::endl;
               if ((*it_target).Mandatory)
	          std::cout << Arc::IString("     Target.Mandatory: true") << std::endl;
               if ((*it_target).NeededReplicas > 0)
	          std::cout << Arc::IString("     Target.NeededReplicas: %d", (*it_target).NeededReplicas) << std::endl;
           }
           if ((*iter).KeepData)
 	      std::cout << Arc::IString("     KeepData: true") << std::endl;
           if ((*iter).IsExecutable)
 	      std::cout << Arc::IString("     IsExecutable: true") << std::endl;
           if (bool((*iter).DataIndexingService))
	      std::cout << Arc::IString("     DataIndexingService: %s", (*iter).DataIndexingService.fullstr()) << std::endl;
           if ((*iter).DownloadToCache)
	      std::cout << Arc::IString("     DownloadToCache: true") << std::endl;
        }
      }
      if (!Directory.empty()) {
        std::list<Arc::DirectoryType>::const_iterator iter;
        for (iter = Directory.begin(); iter != Directory.end(); iter++)
	   std::cout << Arc::IString(" Directory elemet:") << std::endl;
	   std::cout << Arc::IString("     Name: %s", (*iter).Name) << std::endl;
           std::list<Arc::SourceType>::const_iterator it_source;
           for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
	       std::cout << Arc::IString("     Source.URI: %s", (*it_source).URI.fullstr()) << std::endl;
               if ((*it_source).Threads > 0)
	          std::cout << Arc::IString("     Source.Threads: %d", (*it_source).Threads) << std::endl;
           }
           std::list<Arc::TargetType>::const_iterator it_target;
           for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
	       std::cout << Arc::IString("     Target.URI: %s", (*it_target).URI.fullstr()) << std::endl;
               if ((*it_target).Threads > 0)
	          std::cout << Arc::IString("     Target.Threads: %d", (*it_target).Threads) << std::endl;
               if ((*it_target).Mandatory)
	          std::cout << Arc::IString("     Target.Mandatory: true") << std::endl;
               if ((*it_target).NeededReplicas > 0)
	          std::cout << Arc::IString("     Target.NeededReplicas: %d", (*it_target).NeededReplicas) << std::endl;
           }
           if ((*iter).KeepData)
 	      std::cout << Arc::IString("     KeepData: true") << std::endl;
           if ((*iter).IsExecutable)
 	      std::cout << Arc::IString("     IsExecutable: true") << std::endl;
           if (bool((*iter).DataIndexingService))
	      std::cout << Arc::IString("     DataIndexingService: %s", (*iter).DataIndexingService.fullstr()) << std::endl;
           if ((*iter).DownloadToCache)
	      std::cout << Arc::IString("     DownloadToCache: true") << std::endl;
      }
      if (bool(DataIndexingService))
        std::cout << Arc::IString(" DataIndexingService: %s", DataIndexingService.fullstr()) << std::endl;
      if (bool(StagingInBaseURI))
        std::cout << Arc::IString(" StagingInBaseURI: %s", StagingInBaseURI.fullstr()) << std::endl;
      if (bool(StagingOutBaseURI))
        std::cout << Arc::IString(" StagingOutBaseURI: %s", StagingOutBaseURI.fullstr()) << std::endl;
    }

    std::cout << std::endl;
  } // end Print

  void JobInnerRepresentation::Reset(){
    Executable.clear();
    LogDir.clear();
    if (!JobName.empty())
       JobName.clear();
    if (!Description.empty())
       Description.clear();
    if (!JobProject.empty())
       JobProject.clear();
    if (!JobType.empty())
       JobType.clear();
    if (!JobCategory.empty())
       JobCategory.clear();
    if (!UserTag.empty())
       UserTag.clear();
    
    if (!OptionalElement.empty())
       OptionalElement.clear();
    if (!Author.empty())
       Author.clear();
    if (DocumentExpiration != -1)
       DocumentExpiration = -1;
    if (!Rank.empty())
       Rank.clear();
    if (FuzzyRank)
       FuzzyRank = false;

    if (!Argument.empty())
       Argument.clear();
    if (!Input.empty())
       Input.clear();
    if (!Output.empty())
       Output.clear();
    if (!Error.empty())
       Error.clear();

/*    if (!bool(RemoteLogging)){
       RemoteLogging.ChangePort(-1);		//TODO: reset the URL is better
       RemoteLogging.ChangeLDAPScope(URL::base);
//       RemoteLogging.~URL();
//       RemoteLogging.URL();
    }*/
    if (!Environment.empty())
       Environment.clear();
    if (LRMSReRun > 0)
       LRMSReRun = 0;
    if (!Prologue.Name.empty())
       Prologue.Name.clear();
     if (!Prologue.Arguments.empty())
       Prologue.Arguments.clear();
    if (!Epilogue.Name.empty())
       Epilogue.Name.clear();
    if (!Epilogue.Arguments.empty())
       Epilogue.Arguments.clear();
    if (SessionLifeTime != -1)
       SessionLifeTime = -1;
    if (!bool(AccessControl))
       AccessControl.Destroy();
    if (ProcessingStartTime != -1)
       ProcessingStartTime = -1;
    if (!Notification.empty())
       Notification.clear();
/*    if (!bool(CredentialService)){
       CredentialService.ChangePort(-1);		//TODO: reset the URL is better
       CredentialService.ChangeLDAPScope(URL::base);
    }*/
    if (Join)
       Join = false;

    if (TotalCPUTime != -1)
       TotalCPUTime = -1;
    if (IndividualCPUTime != -1)
       IndividualCPUTime = -1;
    if (TotalWallTime != -1)
       TotalWallTime = -1;
    if (IndividualWallTime != -1)
       IndividualWallTime = -1;
//ReferenceTime
    if (ExclusiveExecution)
       ExclusiveExecution = false;
    if (!NetworkInfo.empty())
       NetworkInfo.clear();
    if (!OSFamily.empty())
       OSFamily.clear();
    if (!OSName.empty())
       OSName.clear();
    if (!OSVersion.empty())
       OSVersion.clear();
    if (!Platform.empty())
       Platform.clear();
    if (IndividualPhysicalMemory > 0)
       IndividualPhysicalMemory = 0;
    if (IndividualVirtualMemory > 0)
       IndividualVirtualMemory = 0;
    if (IndividualDiskSpace > 0)
       IndividualDiskSpace = 0;
    if (DiskSpace > 0)
       DiskSpace = 0;
    if (CacheDiskSpace > 0)
       CacheDiskSpace = 0;
    if (SessionDiskSpace > 0)
       SessionDiskSpace = 0;
/*    if (!bool(EndPointURL)){
       EndPointURL.ChangePort(-1);		//TODO: reset the URL is better
       EndPointURL.ChangeLDAPScope(URL::base);
    }*/
    if (!QueueName.empty())
       QueueName.clear();
    if (!CEType.empty())
       CEType.clear();
    if (NumberOfProcesses > 0)
       NumberOfProcesses = 0;
    if (ProcessPerHost > 0)
       ProcessPerHost = 0;
    if (ThreadPerProcesses > 0)
       ThreadPerProcesses = 0;
    if (!SPMDVariation.empty())
       SPMDVariation.clear();
    if (!RunTimeEnvironment.empty())
       RunTimeEnvironment.clear();
    if (!InBound.empty())
       InBound.clear();
    if (!OutBound.empty())
       OutBound.clear();

    if (!File.empty())
       File.clear();
    if (!Directory.empty())
       Directory.clear();
/*    if (!bool(DataIndexingService)){
       DataIndexingService.ChangePort(-1);		//TODO: reset the URL is better
       DataIndexingService.ChangeLDAPScope(URL::base);
    }*/
/*    if (!bool(StagingInBaseURI)){
       StagingInBaseURI.ChangePort(-1);		//TODO: reset the URL is better
       StagingInBaseURI.ChangeLDAPScope(URL::base);
    }*/
/*    if (!bool(StagingOutBaseURI)){
       StagingOutBaseURI.ChangePort(-1);		//TODO: reset the URL is better
       StagingOutBaseURI.ChangeLDAPScope(URL::base);
    }*/
  } // end Reset

} // namespace Arc
