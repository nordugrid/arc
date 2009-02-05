#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include <arc/IString.h>
#include <arc/ArcConfig.h>
#include <arc/client/JobInnerRepresentation.h>

namespace Arc {

  JobInnerRepresentation::JobInnerRepresentation() { Reset(); }

  //JobInnerRepresentation::~JobInnerRepresentation() {}

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

      if (!(Migration.MigrationID.Address()).empty()) {
        std::list<Arc::WSAEndpointReference>::const_iterator m_iter;
        std::string str;
        int i = 1;
        Arc::WSAEndpointReference epr_tmp;
        for (m_iter = Migration.OldJobIDs.begin(); m_iter != Migration.OldJobIDs.end(); m_iter++, i++) {
           epr_tmp = (*m_iter);
           ((Arc::XMLNode)epr_tmp).GetXML(str, true);
           std::cout << Arc::IString("%d. Old Job EPR: %s", i, str) << std::endl;
        }
        epr_tmp = Migration.MigrationID;
        ((Arc::XMLNode)epr_tmp).GetXML(str, true);
        std::cout << Arc::IString("Migration EPR: %s", str) << std::endl;
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
      if (LRMSReRun > -1)
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
      if (!ReferenceTime.benchmark_attribute.empty())
	std::cout << Arc::IString(" ReferenceTime.benchmark: %s", ReferenceTime.benchmark_attribute) << std::endl;
      if (!ReferenceTime.value_attribute.empty())
	std::cout << Arc::IString(" ReferenceTime.value: %s", ReferenceTime.value_attribute) << std::endl;
      if (!ReferenceTime.value.empty())
	std::cout << Arc::IString(" ReferenceTime: %s", ReferenceTime.value) << std::endl;
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
      if (IndividualPhysicalMemory > -1)
	std::cout << Arc::IString(" IndividualPhysicalMemory: %d", IndividualPhysicalMemory) << std::endl;
      if (IndividualVirtualMemory > -1)
	std::cout << Arc::IString(" IndividualVirtualMemory: %d", IndividualVirtualMemory) << std::endl;
      if (DiskSpace > -1)
	std::cout << Arc::IString(" DiskSpace: %d", DiskSpace) << std::endl;
      if (CacheDiskSpace > -1)
	std::cout << Arc::IString(" CacheDiskSpace: %d", CacheDiskSpace) << std::endl;
      if (SessionDiskSpace > -1)
	std::cout << Arc::IString(" SessionDiskSpace: %d", SessionDiskSpace) << std::endl;
      if (IndividualDiskSpace > -1)
	std::cout << Arc::IString(" IndividualDiskSpace: %d", IndividualDiskSpace) << std::endl;
      if (bool(EndPointURL))
        std::cout << Arc::IString(" EndPointURL: %s", EndPointURL.str()) << std::endl;
      if (!QueueName.empty())
	std::cout << Arc::IString(" QueueName: %s", QueueName) << std::endl;
      if (!CEType.empty())
	std::cout << Arc::IString(" CEType: %s", CEType) << std::endl;
      if (Slots > -1)
	std::cout << Arc::IString(" Slots: %d", Slots) << std::endl;
      if (NumberOfProcesses > -1)
	std::cout << Arc::IString(" NumberOfProcesses: %d", NumberOfProcesses) << std::endl;
      if (ProcessPerHost > -1)
	std::cout << Arc::IString(" ProcessPerHost: %d", ProcessPerHost) << std::endl;
      if (ThreadPerProcesses > -1)
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
      if (Homogeneous)
	std::cout << Arc::IString(" Homogeneous: true") << std::endl;
      if (InBound)
	std::cout << Arc::IString(" InBound: true") << std::endl;
      if (OutBound)
	std::cout << Arc::IString(" OutBound: true") << std::endl;
      if (!File.empty()) {
        std::list<Arc::FileType>::const_iterator iter;
        for (iter = File.begin(); iter != File.end(); iter++){
	   std::cout << Arc::IString(" File element:") << std::endl;
	   std::cout << Arc::IString("     Name: %s", (*iter).Name) << std::endl;
           std::list<Arc::SourceType>::const_iterator it_source;
           for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
	       std::cout << Arc::IString("     Source.URI: %s", (*it_source).URI.fullstr()) << std::endl;
               if ((*it_source).Threads > -1)
	          std::cout << Arc::IString("     Source.Threads: %d", (*it_source).Threads) << std::endl;
           }
           std::list<Arc::TargetType>::const_iterator it_target;
           for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
	       std::cout << Arc::IString("     Target.URI: %s", (*it_target).URI.fullstr()) << std::endl;
               if ((*it_target).Threads > -1)
	          std::cout << Arc::IString("     Target.Threads: %d", (*it_target).Threads) << std::endl;
               if ((*it_target).Mandatory)
	          std::cout << Arc::IString("     Target.Mandatory: true") << std::endl;
               if ((*it_target).NeededReplicas > -1)
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
               if ((*it_source).Threads > -1)
	          std::cout << Arc::IString("     Source.Threads: %d", (*it_source).Threads) << std::endl;
           }
           std::list<Arc::TargetType>::const_iterator it_target;
           for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
	       std::cout << Arc::IString("     Target.URI: %s", (*it_target).URI.fullstr()) << std::endl;
               if ((*it_target).Threads > -1)
	          std::cout << Arc::IString("     Target.Threads: %d", (*it_target).Threads) << std::endl;
               if ((*it_target).Mandatory)
	          std::cout << Arc::IString("     Target.Mandatory: true") << std::endl;
               if ((*it_target).NeededReplicas > -1)
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

      if ( !XRSL_elements.empty() ) {
        std::map<std::string, std::string>::const_iterator it;
        for ( it = XRSL_elements.begin(); it != XRSL_elements.end(); it++ ) {
            std::cout << Arc::IString(" XRSL_elements: [%s], %s", it->first, it->second) << std::endl;
        }
      }

      if ( !JDL_elements.empty() ) {
        std::map<std::string, std::string>::const_iterator it;
        for ( it = JDL_elements.begin(); it != JDL_elements.end(); it++ ) {
            std::cout << Arc::IString(" JDL_elements: [%s], %s", it->first, it->second) << std::endl;
        }
      }

    std::cout << std::endl;
  } // end of Print

  void JobInnerRepresentation::Reset(){
       Executable.clear();
       LogDir.clear();
       JobName.clear();
       Description.clear();
       JobProject.clear();
       JobType.clear();
       JobCategory.clear();
       UserTag.clear();
       OptionalElement.clear();
       Author.clear();
       DocumentExpiration = -1;
       Rank.clear();
       FuzzyRank = false;

       Argument.clear();
       Input.clear();
       Output.clear();
       Error.clear();

    if (!bool(RemoteLogging)){
       RemoteLogging.ChangePort(-1);		//TODO: reset the URL better
       RemoteLogging.ChangeLDAPScope(URL::base);
       RemoteLogging.ChangeProtocol("");
    }
       Environment.clear();
       LRMSReRun = -1;
       Prologue.Name.clear();
       Prologue.Arguments.clear();
       Epilogue.Name.clear();
       Epilogue.Arguments.clear();
       SessionLifeTime = -1;
    if (!bool(AccessControl))
       AccessControl.Destroy();
       ProcessingStartTime = -1;
       Notification.clear();
    if (!bool(CredentialService)){
       CredentialService.ChangePort(-1);		//TODO: reset the URL better
       CredentialService.ChangeLDAPScope(URL::base);
       CredentialService.ChangeProtocol("");
    }
    if (Join)
       Join = false;

       TotalCPUTime = -1;
       IndividualCPUTime = -1;
       TotalWallTime = -1;
       IndividualWallTime = -1;
       ReferenceTime.benchmark_attribute = "frequency";
       ReferenceTime.value_attribute = "2.8Ghz" ;
       ReferenceTime.value.clear();
       ExclusiveExecution = false;
       NetworkInfo.clear();
       OSFamily.clear();
       OSName.clear();
       OSVersion.clear();
       Platform.clear();
       IndividualPhysicalMemory = -1;
       IndividualVirtualMemory = -1;
       DiskSpace = -1;
       CacheDiskSpace = -1;
       SessionDiskSpace = -1;
       IndividualDiskSpace = -1;
    if (!bool(EndPointURL)){
       EndPointURL.ChangePort(-1);		//TODO: reset the URL better
       EndPointURL.ChangeLDAPScope(URL::base);
       EndPointURL.ChangeProtocol("");
    }
       QueueName.clear();
       CEType.clear();
       Slots = -1;
       NumberOfProcesses = -1;
       ProcessPerHost = -1;
       ThreadPerProcesses = -1;
       SPMDVariation.clear();
       RunTimeEnvironment.clear();
       Homogeneous = false;
       InBound = false;
       OutBound = false;

       File.clear();
       Directory.clear();
    if (!bool(DataIndexingService)){
       DataIndexingService.ChangePort(-1);		//TODO: reset the URL better
       DataIndexingService.ChangeLDAPScope(URL::base);
       DataIndexingService.ChangeProtocol("");
    }
    if (!bool(StagingInBaseURI)){
       StagingInBaseURI.ChangePort(-1);		//TODO: reset the URL better
       StagingInBaseURI.ChangeLDAPScope(URL::base);
       StagingInBaseURI.ChangeProtocol("");
    }
    if (!bool(StagingOutBaseURI)){
       StagingOutBaseURI.ChangePort(-1);		//TODO: reset the URL better
       StagingOutBaseURI.ChangeLDAPScope(URL::base);
       StagingOutBaseURI.ChangeProtocol("");
    }

    if ( !XRSL_elements.empty() )
       XRSL_elements.clear();

    if ( !JDL_elements.empty() )
       JDL_elements.clear();

    cached = false;
  } // end of Reset

    //Inner representation export to JSDL-GIN XML
    bool JobInnerRepresentation::getXML( Arc::XMLNode& jobTree) const {
        //TODO: wath is the new GIN namespace (it is now temporary namespace)
        Arc::NS gin_namespaces;
        gin_namespaces["jsdl-gin"] = "http://schemas.ogf.org/gin-profile/2008/11/jsdl";
        gin_namespaces["targetNamespace"] = "http://schemas.ogf.org/gin-profile/2008/11/jsdl";
        gin_namespaces["elementFormDefault"] = "qualified";
        Arc::XMLNode outputTree( "<JobDefinition />" );
        outputTree.Namespaces( gin_namespaces );
        outputTree.New(jobTree);

        //obligatory elements: Executable, LogDir
        if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
        if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
        if ( !bool( jobTree["JobDescription"]["Application"]["Executable"] ) ) 
           jobTree["JobDescription"]["Application"].NewChild("Executable") = Executable;
        if ( !bool( jobTree["JobDescription"]["Application"]["LogDir"] ) ) 
           jobTree["JobDescription"]["Application"].NewChild("LogDir") = LogDir;

        //optional elements
        //JobIdentification
        if (!JobName.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"] ) ) 
              jobTree["JobDescription"].NewChild("JobIdentification");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"]["JobName"] ) ) 
              jobTree["JobDescription"]["JobIdentification"].NewChild("JobName") = JobName;
        }
        if (!Description.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"] ) ) 
              jobTree["JobDescription"].NewChild("JobIdentification");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"]["Description"] ) ) 
              jobTree["JobDescription"]["JobIdentification"].NewChild("Description") = Description;
        }
        if (!JobProject.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"] ) ) 
              jobTree["JobDescription"].NewChild("JobIdentification");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"]["JobProject"] ) ) 
              jobTree["JobDescription"]["JobIdentification"].NewChild("JobProject") = JobProject;
        }
        if (!JobType.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"] ) ) 
              jobTree["JobDescription"].NewChild("JobIdentification");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"]["JobType"] ) ) 
              jobTree["JobDescription"]["JobIdentification"].NewChild("JobType") = JobType;
        }
        if (!JobCategory.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"] ) ) 
              jobTree["JobDescription"].NewChild("JobIdentification");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"]["JobCategory"] ) ) 
             jobTree["JobDescription"]["JobIdentification"].NewChild("JobCategory") = JobCategory;
        }
        if (!UserTag.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["JobIdentification"] ) ) 
              jobTree["JobDescription"].NewChild("JobIdentification");
           for (std::list<std::string>::const_iterator it=UserTag.begin();
                            it!=UserTag.end(); it++) {
              Arc::XMLNode attribute = jobTree["JobDescription"]["JobIdentification"].NewChild("UserTag");
              attribute = (*it);
           }
        }

        //Meta
        if (!OptionalElement.empty()){
           if ( !bool( jobTree["Meta"] ) ) jobTree.NewChild("Meta");
           for (std::list<std::string>::const_iterator it=OptionalElement.begin();
                            it!=OptionalElement.end(); it++) {
              Arc::XMLNode attribute = jobTree["Meta"].NewChild("OptionalElement");
              attribute = (*it);
           }
        }
        if (!Author.empty()){
           if ( !bool( jobTree["Meta"] ) ) jobTree.NewChild("Meta");
           if ( !bool( jobTree["Meta"]["Author"] ) ) 
              jobTree["Meta"].NewChild("Author") = Author;
        }
        if (DocumentExpiration != -1){
           if ( !bool( jobTree["Meta"] ) ) jobTree.NewChild("Meta");
           if ( !bool( jobTree["Meta"]["DocumentExpiration"] ) ) 
              jobTree["Meta"].NewChild("DocumentExpiration") = (std::string)DocumentExpiration;
        }
        if (!Rank.empty()){
           if ( !bool( jobTree["Meta"] ) ) jobTree.NewChild("Meta");
           if ( !bool( jobTree["JobDescription"]["Meta"]["Rank"] ) ) 
              jobTree["Meta"].NewChild("Rank") = Rank;
        }
        if (FuzzyRank){
           if ( !bool( jobTree["Meta"] ) ) jobTree.NewChild("Meta");
           if ( !bool( jobTree["Meta"]["FuzzyRank"] ) ) 
              jobTree["Meta"].NewChild("FuzzyRank") = "true";
        }

        //Application
        if (!Argument.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           for (std::list<std::string>::const_iterator it=Argument.begin();
                            it!=Argument.end(); it++) {
              Arc::XMLNode attribute = jobTree["JobDescription"]["Application"].NewChild("Argument");
              attribute = (*it);
           }
        }
        if (!Input.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["Input"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("Input") = Input;
        }
        if (!Output.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["Output"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("Output") = Output;
        }
        if (!Error.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["Error"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("Error") = Error;
        }
        if ( bool(RemoteLogging) ){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["RemoteLogging"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("RemoteLogging") =
                                       RemoteLogging.fullstr();
        }
        if (!Environment.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           for (std::list<Arc::EnvironmentType>::const_iterator it=Environment.begin();
                            it!=Environment.end(); it++) {
              Arc::XMLNode attribute = jobTree["JobDescription"]["Application"].NewChild("Environment");
              attribute = (*it).value;
              attribute.NewAttribute("name") = (*it).name_attribute;
           }
        }
        if (LRMSReRun > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["LRMSReRun"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("LRMSReRun");
           std::ostringstream oss;
           oss << LRMSReRun;
           jobTree["JobDescription"]["Application"]["LRMSReRun"] = oss.str();
        }
        if (!Prologue.Name.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["Prologue"] ) ){
              std::string prologue;
              prologue += Prologue.Name;
              std::list<std::string>::const_iterator iter;
              for (iter = Prologue.Arguments.begin();
                   iter != Prologue.Arguments.end(); iter++){
                prologue += " ";
                prologue += *iter;
              }
              jobTree["JobDescription"]["Application"].NewChild("Prologue") = prologue;
           }
        }
        if (!Epilogue.Name.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["Epilogue"] ) ){
              std::string epilogue;
              epilogue += Epilogue.Name;
              std::list<std::string>::const_iterator iter;
              for (iter = Epilogue.Arguments.begin();
                   iter != Epilogue.Arguments.end(); iter++){
                epilogue += " ";
                epilogue += *iter;
              }
              jobTree["JobDescription"]["Application"].NewChild("Epilogue") = epilogue;
           }
        }
        if (SessionLifeTime != -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["SessionLifeTime"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("SessionLifeTime") = SessionLifeTime;
        }
        if ( bool(AccessControl) ){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["AccessControl"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("AccessControl");
           jobTree["JobDescription"]["Application"]["AccessControl"].NewChild(AccessControl);
        }
        if (ProcessingStartTime != -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["ProcessingStartTime"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("ProcessingStartTime") =
                                               ProcessingStartTime;
        }
        if (!Notification.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           for (std::list<Arc::NotificationType>::const_iterator it=Notification.begin();
                            it!=Notification.end(); it++) {
              Arc::XMLNode attribute = jobTree["JobDescription"]["Application"].NewChild("Notification");
              for (std::list<std::string>::const_iterator iter=(*it).Address.begin();
                            iter!=(*it).Address.end(); iter++) {
                  attribute.NewChild("Address") = (*iter);
              }
              for (std::list<std::string>::const_iterator iter=(*it).State.begin();
                            iter!=(*it).State.end(); iter++) {
                  attribute.NewChild("State") = (*iter);
              }
           }
        }
        if ( bool(CredentialService) ){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["CredentialService"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("CredentialService") =
                                               CredentialService.fullstr();
        }
        if (Join){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Application"] ) ) 
              jobTree["JobDescription"].NewChild("Application");
           if ( !bool( jobTree["JobDescription"]["Application"]["Join"] ) ) 
              jobTree["JobDescription"]["Application"].NewChild("Join") = "true";
        }

        // Resource
        if (TotalCPUTime != -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["TotalCPUTime"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("TotalCPUTime") = (std::string)TotalCPUTime;
        }
        if (IndividualCPUTime != -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["IndividualCPUTime"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("IndividualCPUTime") = (std::string)IndividualCPUTime;
        }
        if (TotalWallTime != -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["TotalWallTime"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("TotalWallTime") = (std::string)TotalWallTime;
        }
        if (IndividualWallTime != -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["IndividualWallTime"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("IndividualWallTime") = (std::string)IndividualWallTime;
        }
        if (!ReferenceTime.value.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           Arc::XMLNode attribute;
           if ( !bool( jobTree["JobDescription"]["Resource"]["ReferenceTime"] ) ) 
              attribute = jobTree["JobDescription"]["Resource"].NewChild("ReferenceTime");
           attribute = ReferenceTime.value;
           if (!ReferenceTime.benchmark_attribute.empty()){
              attribute.NewAttribute("benchmark") = ReferenceTime.benchmark_attribute;
           }
           if (!ReferenceTime.value_attribute.empty()){
              attribute.NewAttribute("value") = ReferenceTime.value_attribute;
           }
        }
        if (ExclusiveExecution){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["ExclusiveExecution"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("ExclusiveExecution") = "true";
        }
        if (!NetworkInfo.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["NetworkInfo"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("NetworkInfo") = NetworkInfo;
        }
        if (!OSFamily.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["OSFamily"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("OSFamily") = OSFamily;
        }
        if (!OSName.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["OSName"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("OSName") = OSName;
        }
        if (!OSVersion.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["OSVersion"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("OSVersion") = OSVersion;
        }
        if (!Platform.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Platform"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("Platform") = Platform;
        }
        if (IndividualPhysicalMemory > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["IndividualPhysicalMemory"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("IndividualPhysicalMemory");
           std::ostringstream oss;
           oss << IndividualPhysicalMemory;
           jobTree["JobDescription"]["Resource"]["IndividualPhysicalMemory"] = oss.str();
        }
        if (IndividualVirtualMemory > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["IndividualVirtualMemory"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("IndividualVirtualMemory");
           std::ostringstream oss;
           oss << IndividualVirtualMemory;
           jobTree["JobDescription"]["Resource"]["IndividualVirtualMemory"] = oss.str();
        }
        if (DiskSpace > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["DiskSpace"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
           std::ostringstream oss;
           oss << DiskSpace;
           jobTree["JobDescription"]["Resource"]["DiskSpace"] = oss.str();
        }
        if (CacheDiskSpace > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["DiskSpace"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
           if ( !bool( jobTree["JobDescription"]["Resource"]["DiskSpace"]["CacheDiskSpace"] ) ) 
              jobTree["JobDescription"]["Resource"]["DiskSpace"].NewChild("CacheDiskSpace");
           std::ostringstream oss;
           oss << CacheDiskSpace;
           jobTree["JobDescription"]["Resource"]["DiskSpace"]["CacheDiskSpace"] = oss.str();
        }
        if (SessionDiskSpace > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["DiskSpace"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
           if ( !bool( jobTree["JobDescription"]["Resource"]["DiskSpace"]["SessionDiskSpace"] ) ) 
              jobTree["JobDescription"]["Resource"]["DiskSpace"].NewChild("SessionDiskSpace");
           std::ostringstream oss;
           oss << SessionDiskSpace;
           jobTree["JobDescription"]["Resource"]["DiskSpace"]["SessionDiskSpace"] = oss.str();
        }
        if (IndividualDiskSpace > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["DiskSpace"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
           if ( !bool( jobTree["JobDescription"]["Resource"]["DiskSpace"]["IndividualDiskSpace"] ) ) 
              jobTree["JobDescription"]["Resource"]["DiskSpace"].NewChild("IndividualDiskSpace");
           std::ostringstream oss;
           oss << IndividualDiskSpace;
           jobTree["JobDescription"]["Resource"]["DiskSpace"]["IndividualDiskSpace"] = oss.str();
        }
        if (bool(EndPointURL)){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["CandidateTarget"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("CandidateTarget");
           if ( !bool( jobTree["JobDescription"]["Resource"]["CandidateTarget"]["EndPointURL"] ) ) 
              jobTree["JobDescription"]["Resource"]["CandidateTarget"].NewChild("EndPointURL") =
                                                     EndPointURL.str();
        }
        if (!QueueName.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["CandidateTarget"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("CandidateTarget");
           if ( !bool( jobTree["JobDescription"]["Resource"]["CandidateTarget"]["QueueName"] ) ) 
              jobTree["JobDescription"]["Resource"]["CandidateTarget"].NewChild("QueueName") =
                                                    QueueName;
        }
        if (!CEType.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["CEType"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("CEType") = CEType;
        }
        if (Slots > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("Slots");
           std::ostringstream oss;
           oss << NumberOfProcesses;
           jobTree["JobDescription"]["Resource"]["Slots"] = oss.str();
        }
        if (NumberOfProcesses > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("Slots");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"]["NumberOfProcesses"] ) ) 
              jobTree["JobDescription"]["Resource"]["Slots"].NewChild("NumberOfProcesses");
           std::ostringstream oss;
           oss << NumberOfProcesses;
           jobTree["JobDescription"]["Resource"]["Slots"]["NumberOfProcesses"] = oss.str();
        }
        if (ProcessPerHost > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("Slots");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"]["ProcessPerHost"] ) ) 
              jobTree["JobDescription"]["Resource"]["Slots"].NewChild("ProcessPerHost");
           std::ostringstream oss;
           oss << ProcessPerHost;
           jobTree["JobDescription"]["Resource"]["Slots"]["ProcessPerHost"] = oss.str();
        }
        if (ThreadPerProcesses > -1){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("Slots");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"]["ThreadPerProcesses"] ) ) 
              jobTree["JobDescription"]["Resource"]["Slots"].NewChild("ThreadPerProcesses");
           std::ostringstream oss;
           oss << ThreadPerProcesses;
           jobTree["JobDescription"]["Resource"]["Slots"]["ThreadPerProcesses"] = oss.str();
        }
        if (!SPMDVariation.empty()){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("Slots");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Slots"]["SPMDVariation"] ) ) 
              jobTree["JobDescription"]["Resource"]["Slots"].NewChild("SPMDVariation") =
                                                    SPMDVariation;
        }
        if (!RunTimeEnvironment.empty()) {
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           std::list<Arc::RunTimeEnvironmentType>::const_iterator iter;
           for (iter = RunTimeEnvironment.begin(); 
                        iter != RunTimeEnvironment.end(); iter++){
                Arc::XMLNode attribute = jobTree["JobDescription"]["Resource"].NewChild("RunTimeEnvironment");
                attribute.NewChild("Name") = (*iter).Name;
                std::list<std::string>::const_iterator it;
                for (it = (*iter).Version.begin(); it != (*iter).Version.end(); it++){
                    attribute.NewChild("Version") = (*it);
                }
           }
        }
        if (Homogeneous){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["Homogeneous"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("Homogeneous") = "true";
        }
        if (InBound){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["NodeAccess"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("NodeAccess");
           if ( !bool( jobTree["JobDescription"]["Resource"]["NodeAccess"]["InBound"] ) ) 
              jobTree["JobDescription"]["Resource"]["NodeAccess"].NewChild("InBound") = "true";
        }
        if (OutBound){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["Resource"] ) ) 
              jobTree["JobDescription"].NewChild("Resource");
           if ( !bool( jobTree["JobDescription"]["Resource"]["NodeAccess"] ) ) 
              jobTree["JobDescription"]["Resource"].NewChild("NodeAccess");
           if ( !bool( jobTree["JobDescription"]["Resource"]["NodeAccess"]["OutBound"] ) ) 
              jobTree["JobDescription"]["Resource"]["NodeAccess"].NewChild("OutBound") = "true";
        }
     
        //DataStaging
        if (!File.empty()) {
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["DataStaging"] ) ) 
              jobTree["JobDescription"].NewChild("DataStaging");
           std::list<Arc::FileType>::const_iterator iter;
           for (iter = File.begin(); iter != File.end(); iter++){
              Arc::XMLNode attribute = jobTree["JobDescription"]["DataStaging"].NewChild("File");
	      attribute.NewChild("Name") = (*iter).Name;
              std::list<Arc::SourceType>::const_iterator it_source;
              for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
	          XMLNode attribute_source = attribute.NewChild("Source");
                  attribute_source.NewChild("URI") = (*it_source).URI.fullstr();
                  if ((*it_source).Threads > -1){
                     std::ostringstream oss;
                     oss << (*it_source).Threads;
                     attribute_source.NewChild("Threads") = oss.str();
                  }
              }
              std::list<Arc::TargetType>::const_iterator it_target;
              for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
	          XMLNode attribute_target = attribute.NewChild("Target");
                  attribute_target.NewChild("URI") = (*it_target).URI.fullstr();
                  if ((*it_target).Threads > -1){
                     std::ostringstream oss;
                     oss << (*it_target).Threads;
                     attribute_target.NewChild("Threads") = oss.str();
                  }
                  if ((*it_target).Mandatory)
                     attribute_target.NewChild("Mandatory") = "true";
                  if ((*it_target).NeededReplicas > -1){
                     std::ostringstream oss;
                     oss << (*it_target).NeededReplicas;
                     attribute_target.NewChild("NeededReplicas") = oss.str();
                  }
              }
              if ((*iter).KeepData)
 	         attribute.NewChild("KeepData") = "true";
              if ((*iter).IsExecutable)
 	         attribute.NewChild("IsExecutable") = "true";
              if (bool((*iter).DataIndexingService))
 	         attribute.NewChild("DataIndexingService") = (*iter).DataIndexingService.fullstr();
              if ((*iter).DownloadToCache)
 	         attribute.NewChild("DownloadToCache") = "true";
           }
        }
        if (!Directory.empty()) {
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["DataStaging"] ) ) 
              jobTree["JobDescription"].NewChild("DataStaging");
           std::list<Arc::DirectoryType>::const_iterator iter;
           for (iter = Directory.begin(); iter != Directory.end(); iter++){
              Arc::XMLNode attribute = jobTree["JobDescription"]["DataStaging"].NewChild("Directory");
	      attribute.NewChild("Name") = (*iter).Name;
              std::list<Arc::SourceType>::const_iterator it_source;
              for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
	          XMLNode attribute_source = attribute.NewChild("Source");
                  attribute_source.NewChild("URI") = (*it_source).URI.fullstr();
                  if ((*it_source).Threads > -1){
                     std::ostringstream oss;
                     oss << (*it_source).Threads;
                     attribute_source.NewChild("Threads") = oss.str();
                  }
              }
              std::list<Arc::TargetType>::const_iterator it_target;
              for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
	          XMLNode attribute_target = attribute.NewChild("Target");
                  attribute_target.NewChild("URI") = (*it_target).URI.fullstr();
                  if ((*it_target).Threads > -1){
                     std::ostringstream oss;
                     oss << (*it_target).Threads;
                     attribute_target.NewChild("Threads") = oss.str();
                  }
                  if ((*it_target).Mandatory)
                     attribute_target.NewChild("Mandatory") = "true";
                  if ((*it_target).NeededReplicas > -1){
                     std::ostringstream oss;
                     oss << (*it_target).NeededReplicas;
                     attribute_target.NewChild("NeededReplicas") = oss.str();
                  }
              }
              if ((*iter).KeepData)
 	         attribute.NewChild("KeepData") = "true";
              if ((*iter).IsExecutable)
 	         attribute.NewChild("IsExecutable") = "true";
              if (bool((*iter).DataIndexingService))
 	         attribute.NewChild("DataIndexingService") = (*iter).DataIndexingService.fullstr();
              if ((*iter).DownloadToCache)
 	         attribute.NewChild("DownloadToCache") = "true";
           }
        }
        if (bool(DataIndexingService)){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["DataStaging"] ) ) 
              jobTree["JobDescription"].NewChild("DataStaging");
           if ( !bool( jobTree["JobDescription"]["DataStaging"]["Defaults"] ) ) 
              jobTree["JobDescription"]["DataStaging"].NewChild("Defaults");
           if ( !bool( jobTree["JobDescription"]["DataStaging"]["Defaults"]["DataIndexingService"] ) ) 
              jobTree["JobDescription"]["DataStaging"]["Defaults"].NewChild("DataIndexingService") =
                                         DataIndexingService.fullstr();
        }
        if (bool(StagingInBaseURI)){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["DataStaging"] ) ) 
              jobTree["JobDescription"].NewChild("DataStaging");
           if ( !bool( jobTree["JobDescription"]["DataStaging"]["Defaults"] ) ) 
              jobTree["JobDescription"]["DataStaging"].NewChild("Defaults");
           if ( !bool( jobTree["JobDescription"]["DataStaging"]["Defaults"]["StagingInBaseURI"] ) ) 
              jobTree["JobDescription"]["DataStaging"]["Defaults"].NewChild("StagingInBaseURI") =
                                         StagingInBaseURI.fullstr();
        }
        if (bool(StagingOutBaseURI)){
           if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
           if ( !bool( jobTree["JobDescription"]["DataStaging"] ) ) 
              jobTree["JobDescription"].NewChild("DataStaging");
           if ( !bool( jobTree["JobDescription"]["DataStaging"]["Defaults"] ) ) 
              jobTree["JobDescription"]["DataStaging"].NewChild("Defaults");
           if ( !bool( jobTree["JobDescription"]["DataStaging"]["Defaults"]["StagingOutBaseURI"] ) ) 
              jobTree["JobDescription"]["DataStaging"]["Defaults"].NewChild("StagingOutBaseURI") =
                                         StagingOutBaseURI.fullstr();
        }
        return true;
    } //end of getXML

  // Copy constructor
  JobInnerRepresentation::JobInnerRepresentation(const JobInnerRepresentation& job){
    JobName             = job.JobName;
    Description         = job.Description;
    JobProject          = job.JobProject;
    JobType             = job.JobType;
    JobCategory         = job.JobCategory;
    UserTag             = job.UserTag;
    
    OptionalElement     = job.OptionalElement;
    Author              = job.Author;
    DocumentExpiration  = job.DocumentExpiration;
    Rank                = job.Rank;
    FuzzyRank           = job.FuzzyRank;

    Executable          = job.Executable;
    LogDir              = job.LogDir;
    Argument            = job.Argument;
    Input               = job.Input;
    Output              = job.Output;
    Error               = job.Error;
    RemoteLogging       = job.RemoteLogging;
    Environment         = job.Environment;
    LRMSReRun           = job.LRMSReRun;
    Prologue.Name       = job.Prologue.Name;
    Prologue.Arguments  = job.Prologue.Arguments;
    Epilogue.Name       = job.Epilogue.Name;
    Epilogue.Arguments  = job.Epilogue.Arguments;
    SessionLifeTime     = job.SessionLifeTime;
    AccessControl       = job.AccessControl;
    ProcessingStartTime = job.ProcessingStartTime;
    Notification        = job.Notification;
    CredentialService   = job.CredentialService;
    Join                = job.Join;

    TotalCPUTime        = job.TotalCPUTime;
    IndividualCPUTime   = job.IndividualCPUTime;
    TotalWallTime       = job.TotalWallTime;
    IndividualWallTime  = job.IndividualWallTime;
    ReferenceTime.benchmark_attribute = job.ReferenceTime.benchmark_attribute;
    ReferenceTime.value_attribute     = job.ReferenceTime.value_attribute;
    ReferenceTime.value               = job.ReferenceTime.value;
    ExclusiveExecution  = job.ExclusiveExecution;
    NetworkInfo         = job.NetworkInfo;
    OSFamily            = job.OSFamily;
    OSName              = job.OSName;
    OSVersion           = job.OSVersion;
    Platform            = job.Platform;
    IndividualPhysicalMemory = job.IndividualPhysicalMemory;
    IndividualVirtualMemory  = job.IndividualVirtualMemory;
    IndividualDiskSpace      = job.IndividualDiskSpace;
    DiskSpace           = job.DiskSpace;
    CacheDiskSpace      = job.CacheDiskSpace;
    SessionDiskSpace    = job.SessionDiskSpace;
    EndPointURL         = job.EndPointURL;
    QueueName           = job.QueueName;
    CEType              = job.CEType;
    Slots               = job.Slots;
    NumberOfProcesses   = job.NumberOfProcesses;
    ProcessPerHost      = job.ProcessPerHost;
    ThreadPerProcesses  = job.ThreadPerProcesses;
    SPMDVariation       = job.SPMDVariation;
    RunTimeEnvironment  = job.RunTimeEnvironment;
    Homogeneous         = job.Homogeneous;
    InBound             = job.InBound;
    OutBound            = job.OutBound;

    File                = job.File;
    Directory           = job.Directory;
    DataIndexingService = job.DataIndexingService;
    StagingInBaseURI    = job.StagingInBaseURI;
    StagingOutBaseURI   = job.StagingOutBaseURI;

    XRSL_elements       = job.XRSL_elements;
    JDL_elements        = job.JDL_elements;
  } // end of copy constructor

  JobInnerRepresentation::JobInnerRepresentation(const long int addrptr) {
    *this = *((JobInnerRepresentation*) addrptr);
  }

} // namespace Arc
