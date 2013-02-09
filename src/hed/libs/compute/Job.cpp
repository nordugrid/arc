// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <errno.h>

#include <algorithm>

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/JobControllerPlugin.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/FileCache.h>

#include "Job.h"

#define JXMLTOSTRING(NAME) \
    if (job[ #NAME ]) {\
      NAME = (std::string)job[ #NAME ];\
    }

#define JXMLSTRINGTO(TYPE, NAME) \
    if (job[ #NAME ]) {\
      TYPE temp##TYPE##NAME;\
      if (stringto((std::string)job[ #NAME ], temp##TYPE##NAME)) {\
        NAME = temp##TYPE##NAME;\
      }\
    }

#define JXMLTOTIME(NAME) \
    if (job[ #NAME ]) {\
      Time temp##NAME((std::string)job[ #NAME ]);\
      if (temp##NAME.GetTime() != -1) {\
        NAME = temp##NAME;\
      }\
    }

#define JXMLTOSTRINGLIST(NAME) \
    NAME.clear();\
    for (XMLNode n = job[ #NAME ]; n; ++n) {\
      NAME.push_back((std::string)n);\
    }

#define STRINGTOXML(NAME) \
    if (!(NAME).empty()) {\
      node.NewChild( #NAME ) = NAME;\
    }

#define URLTOXML(NAME) \
    if (NAME) {\
      node.NewChild( #NAME ) = NAME.fullstr();\
    }

#define INTTOXML(NAME) \
    if (NAME != -1) {\
      node.NewChild( #NAME ) = tostring(NAME);\
    }

#define TIMETOSTRING(NAME) \
    if (NAME != -1) {\
      node.NewChild( #NAME ) = (NAME).str(UTCTime);\
    }

#define PERIODTOSTRING(NAME) \
    if (NAME != -1) {\
      node.NewChild( #NAME ) = (std::string)NAME;\
    }

#define STRINGLISTTOXML(NAME) \
    for (std::list<std::string>::const_iterator it = NAME.begin();\
         it != NAME.end(); it++) {\
      node.NewChild( #NAME ) = *it;\
    }

namespace Arc {

  Logger Job::logger(Logger::getRootLogger(), "Job");

  JobControllerPluginLoader& Job::getLoader() {
    // For C++ it would be enough to have 
    //   static JobControllerPluginLoader loader;
    // But Java sometimes does not destroy objects causing
    // PluginsFactory destructor loop forever waiting for
    // plugins to exit.
    static JobControllerPluginLoader* loader = NULL;
    if(!loader) {
      loader = new JobControllerPluginLoader();
    }
    return *loader;
  }
  
  DataHandle* Job::data_source = NULL;
  DataHandle* Job::data_destination = NULL;

  Job::Job()
    : ExitCode(-1),
      WaitingPosition(-1),
      RequestedTotalWallTime(-1),
      RequestedTotalCPUTime(-1),
      RequestedSlots(-1),
      UsedTotalWallTime(-1),
      UsedTotalCPUTime(-1),
      UsedMainMemory(-1),
      LocalSubmissionTime(-1),
      SubmissionTime(-1),
      ComputingManagerSubmissionTime(-1),
      StartTime(-1),
      ComputingManagerEndTime(-1),
      EndTime(-1),
      WorkingAreaEraseTime(-1),
      ProxyExpirationTime(-1),
      CreationTime(-1),
      Validity(-1),
      jc(NULL) {}

  Job::~Job() {}

  Job::Job(const Job& j)
    : ExitCode(-1),
      WaitingPosition(-1),
      RequestedTotalWallTime(-1),
      RequestedTotalCPUTime(-1),
      RequestedSlots(-1),
      UsedTotalWallTime(-1),
      UsedTotalCPUTime(-1),
      UsedMainMemory(-1),
      LocalSubmissionTime(-1),
      SubmissionTime(-1),
      ComputingManagerSubmissionTime(-1),
      StartTime(-1),
      ComputingManagerEndTime(-1),
      EndTime(-1),
      WorkingAreaEraseTime(-1),
      ProxyExpirationTime(-1),
      CreationTime(-1),
      Validity(-1),
      jc(NULL) {
    *this = j;
  }

  Job::Job(XMLNode j)
    : ExitCode(-1),
      WaitingPosition(-1),
      RequestedTotalWallTime(-1),
      RequestedTotalCPUTime(-1),
      RequestedSlots(-1),
      UsedTotalWallTime(-1),
      UsedTotalCPUTime(-1),
      UsedMainMemory(-1),
      LocalSubmissionTime(-1),
      SubmissionTime(-1),
      ComputingManagerSubmissionTime(-1),
      StartTime(-1),
      ComputingManagerEndTime(-1),
      EndTime(-1),
      WorkingAreaEraseTime(-1),
      ProxyExpirationTime(-1),
      CreationTime(-1),
      Validity(-1) {
    *this = j;
  }

  Job& Job::operator=(const Job& j) {
    jc = j.jc;

    // Proposed mandatory attributes for ARC 3.0
    Name = j.Name;
    ServiceInformationURL = j.ServiceInformationURL;
    ServiceInformationInterfaceName = j.ServiceInformationInterfaceName;
    JobStatusURL = j.JobStatusURL;
    JobStatusInterfaceName = j.JobStatusInterfaceName;
    JobManagementURL = j.JobManagementURL;
    JobManagementInterfaceName = j.JobManagementInterfaceName;

    StageInDir = j.StageInDir;
    StageOutDir = j.StageOutDir;
    SessionDir = j.SessionDir;

    JobID = j.JobID;

    Type = j.Type;
    IDFromEndpoint = j.IDFromEndpoint;
    LocalIDFromManager = j.LocalIDFromManager;
    JobDescription = j.JobDescription;
    JobDescriptionDocument = j.JobDescriptionDocument;
    State = j.State;
    RestartState = j.RestartState;
    ExitCode = j.ExitCode;
    ComputingManagerExitCode = j.ComputingManagerExitCode;
    Error = j.Error;
    WaitingPosition = j.WaitingPosition;
    UserDomain = j.UserDomain;
    Owner = j.Owner;
    LocalOwner = j.LocalOwner;
    RequestedTotalWallTime = j.RequestedTotalWallTime;
    RequestedTotalCPUTime = j.RequestedTotalCPUTime;
    RequestedSlots = j.RequestedSlots;
    RequestedApplicationEnvironment = j.RequestedApplicationEnvironment;
    StdIn = j.StdIn;
    StdOut = j.StdOut;
    StdErr = j.StdErr;
    LogDir = j.LogDir;
    ExecutionNode = j.ExecutionNode;
    Queue = j.Queue;
    UsedTotalWallTime = j.UsedTotalWallTime;
    UsedTotalCPUTime = j.UsedTotalCPUTime;
    UsedMainMemory = j.UsedMainMemory;
    RequestedApplicationEnvironment = j.RequestedApplicationEnvironment;
    RequestedSlots = j.RequestedSlots;
    LocalSubmissionTime = j.LocalSubmissionTime;
    SubmissionTime = j.SubmissionTime;
    ComputingManagerSubmissionTime = j.ComputingManagerSubmissionTime;
    StartTime = j.StartTime;
    ComputingManagerEndTime = j.ComputingManagerEndTime;
    EndTime = j.EndTime;
    WorkingAreaEraseTime = j.WorkingAreaEraseTime;
    ProxyExpirationTime = j.ProxyExpirationTime;
    SubmissionHost = j.SubmissionHost;
    SubmissionClientName = j.SubmissionClientName;
    CreationTime = j.CreationTime;
    Validity = j.Validity;
    OtherMessages = j.OtherMessages;

    ActivityOldID = j.ActivityOldID;
    LocalInputFiles = j.LocalInputFiles;

    return *this;
  }

  int Job::operator==(const Job& other) { return JobID == other.JobID; }

  Job& Job::operator=(XMLNode job) {
    jc = NULL;
  
    // Detect format:
    if      (job["JobID"] && job["IDFromEndpoint"] && 
               (job["ServiceInformationURL"] || job["ServiceInformationInterfaceName"] ||
                job["JobStatusURL"] || job["JobStatusInterfaceName"] ||
                job["JobManagementURL"] || job["JobManagementInterfaceName"])
            )
    { // Version >= 3.x format.
      JobID = (std::string)job["JobID"];
      IDFromEndpoint = (std::string)job["IDFromEndpoint"];
      ServiceInformationURL = URL((std::string)job["ServiceInformationURL"]);
      ServiceInformationInterfaceName = (std::string)job["ServiceInformationInterfaceName"];
      JobStatusURL = URL((std::string)job["JobStatusURL"]);
      JobStatusInterfaceName = (std::string)job["JobStatusInterfaceName"];
      JobManagementURL = URL((std::string)job["JobManagementURL"]);
      JobManagementInterfaceName = (std::string)job["JobManagementInterfaceName"];

      if (job["StageInDir"])  StageInDir  = URL((std::string)job["StageInDir"]);
      if (job["StageOutDir"]) StageOutDir = URL((std::string)job["StageOutDir"]);
      if (job["SessionDir"])  SessionDir  = URL((std::string)job["SessionDir"]);
    }
    else if (job["JobID"] && job["Cluster"] && job["InterfaceName"] && job["IDFromEndpoint"])
    { // Version 2.x format.
      JobID = (std::string)job["JobID"];
      ServiceInformationURL = URL((std::string)job["Cluster"]);
      JobStatusURL = URL((std::string)job["IDFromEndpoint"]);

      JobManagementURL = URL(JobID);
      StageInDir  = JobManagementURL;
      StageOutDir = JobManagementURL;
      SessionDir  = JobManagementURL;
      
      const std::string path = JobManagementURL.Path();
      std::size_t slashpos = path.rfind("/");
      IDFromEndpoint = path.substr(slashpos+1);

      JobManagementURL.ChangePath(path.substr(0, slashpos));
      JobManagementInterfaceName = (std::string)job["InterfaceName"];
      if      (JobManagementInterfaceName == "org.nordugrid.gridftpjob") {
        ServiceInformationInterfaceName = "org.nordugrid.ldapng";
        JobStatusInterfaceName          = "org.nordugrid.ldapng";
      }
      else if (JobManagementInterfaceName == "org.ogf.bes") {
        ServiceInformationInterfaceName = "org.ogf.bes";
        JobStatusInterfaceName          = "org.ogf.bes";
      }
      else if (JobManagementInterfaceName == "org.ogf.xbes") {
        ServiceInformationInterfaceName = "org.nordugrid.wsrfglue2";
        JobStatusInterfaceName          = "org.ogf.xbes";
      }
    }
    else if (job["Cluster"] && job["InfoEndpoint"] && job["IDFromEndpoint"] && job["Flavour"])
    { // Version 1.x format.
      JobID = (std::string)job["IDFromEndpoint"];
      ServiceInformationURL = URL((std::string)job["Cluster"]);
      JobStatusURL = URL((std::string)job["InfoEndpoint"]);
      
      JobManagementURL = URL(JobID);
      StageInDir = JobManagementURL;
      StageOutDir = JobManagementURL;
      SessionDir = JobManagementURL;
      
      const std::string path = JobManagementURL.Path();
      std::size_t slashpos = path.rfind("/");
      IDFromEndpoint = path.substr(slashpos+1);
      JobManagementURL.ChangePath(path.substr(0, slashpos));

      if      ((std::string)job["Flavour"] == "ARC0")  {
        ServiceInformationInterfaceName = "org.nordugrid.ldapng";
        JobStatusInterfaceName          = "org.nordugrid.ldapng";
        JobManagementInterfaceName      = "org.nordugrid.gridftpjob";
      }
      else if ((std::string)job["Flavour"] == "BES") {
        ServiceInformationInterfaceName = "org.ogf.bes";
        JobStatusInterfaceName          = "org.ogf.bes";
        JobManagementInterfaceName      = "org.ogf.bes";
      }
      else if ((std::string)job["Flavour"] == "ARC1") {
        ServiceInformationInterfaceName = "org.nordugrid.wsrfglue2";
        JobStatusInterfaceName          = "org.nordugrid.xbes";
        JobManagementInterfaceName      = "org.nordugrid.xbes";
      }
    }
    else {
      logger.msg(WARNING, "Unable to detect format of job record.");
    }

    JXMLTOSTRING(Name)

    if (job["JobDescription"]) {
      const std::string sjobdesc = job["JobDescription"];
      if (job["JobDescriptionDocument"] || job["State"] ||
          !job["LocalSubmissionTime"]) {
        // If the 'JobDescriptionDocument' or 'State' element is set assume that the 'JobDescription' element is the GLUE2 one.
        // Default is to assume it is the GLUE2 one.
        JobDescription = sjobdesc;
      }
      else {
        // If the 'LocalSubmissionTime' element is set assume that the 'JobDescription' element contains the actual job description.
        JobDescriptionDocument = sjobdesc;
      }
    }

    JXMLTOSTRING(JobDescriptionDocument)
    JXMLTOTIME(LocalSubmissionTime)

    if (job["Associations"]["ActivityOldID"]) {
      ActivityOldID.clear();
      for (XMLNode n = job["Associations"]["ActivityOldID"]; n; ++n) {
        ActivityOldID.push_back((std::string)n);
      }
    }
    else if (job["OldJobID"]) { // Included for backwards compatibility.
      ActivityOldID.clear();
      for (XMLNode n = job["OldJobID"]; n; ++n) {
        ActivityOldID.push_back((std::string)n);
      }
    }

    if (job["Associations"]["LocalInputFile"]) {
      LocalInputFiles.clear();
      for (XMLNode n = job["Associations"]["LocalInputFile"]; n; ++n) {
        if (n["Source"] && n["CheckSum"]) {
          LocalInputFiles[(std::string)n["Source"]] = (std::string)n["CheckSum"];
        }
      }
    }
    else if (job["LocalInputFiles"]["File"]) { // Included for backwards compatibility.
      LocalInputFiles.clear();
      for (XMLNode n = job["LocalInputFiles"]["File"]; n; ++n) {
        if (n["Source"] && n["CheckSum"]) {
          LocalInputFiles[(std::string)n["Source"]] = (std::string)n["CheckSum"];
        }
      }
    }

    // Pick generic GLUE2 information
    SetFromXML(job);

    return *this;
  }

  void Job::SetFromXML(XMLNode job) {

    JXMLTOSTRING(Type)

    // TODO: find out how to treat IDFromEndpoint in case of pure GLUE2
    
    JXMLTOSTRING(LocalIDFromManager)

    /* Earlier the 'JobDescription' element in a XMLNode representing a Job
     * object contained the actual job description, but in GLUE2 the name
     * 'JobDescription' specifies the job description language which was used to
     * describe the job. Due to the name clash we must guess what is meant when
     * parsing the 'JobDescription' element.
     */

    //  TODO: same for JobDescription

    // Parse libarccompute special state format.
    if (job["State"]["General"] && job["State"]["Specific"]) {
      State.state = (std::string)job["State"]["Specific"];
      State.type = JobState::GetStateType((std::string)job["State"]["General"]);
    }
    // Only use the first state. ACC modules should set the state them selves.
    else if (job["State"] && job["State"].Size() == 0) {
      State.state = (std::string)job["State"];
      State.type = JobState::OTHER;
    }
    if (job["RestartState"]["General"] && job["RestartState"]["Specific"]) {
      RestartState.state = (std::string)job["RestartState"]["Specific"];
      RestartState.type = JobState::GetStateType((std::string)job["RestartState"]["General"]);
    }
    // Only use the first state. ACC modules should set the state them selves.
    else if (job["RestartState"] && job["RestartState"].Size() == 0) {
      RestartState.state = (std::string)job["RestartState"];
      RestartState.type = JobState::OTHER;
    }

    JXMLSTRINGTO(int, ExitCode)
    JXMLTOSTRING(ComputingManagerExitCode)
    JXMLTOSTRINGLIST(Error)
    JXMLSTRINGTO(int, WaitingPosition)
    JXMLTOSTRING(UserDomain)
    JXMLTOSTRING(Owner)
    JXMLTOSTRING(LocalOwner)
    JXMLSTRINGTO(long, RequestedTotalWallTime)
    JXMLSTRINGTO(long, RequestedTotalCPUTime)
    JXMLSTRINGTO(int, RequestedSlots)
    JXMLTOSTRINGLIST(RequestedApplicationEnvironment)
    JXMLTOSTRING(StdIn)
    JXMLTOSTRING(StdOut)
    JXMLTOSTRING(StdErr)
    JXMLTOSTRING(LogDir)
    JXMLTOSTRINGLIST(ExecutionNode)
    JXMLTOSTRING(Queue)
    JXMLSTRINGTO(long, UsedTotalWallTime)
    JXMLSTRINGTO(long, UsedTotalCPUTime)
    JXMLSTRINGTO(int, UsedMainMemory)
    JXMLTOTIME(SubmissionTime)
    JXMLTOTIME(ComputingManagerSubmissionTime)
    JXMLTOTIME(StartTime)
    JXMLTOTIME(ComputingManagerEndTime)
    JXMLTOTIME(EndTime)
    JXMLTOTIME(WorkingAreaEraseTime)
    JXMLTOTIME(ProxyExpirationTime)
    JXMLTOSTRING(SubmissionHost)
    JXMLTOSTRING(SubmissionClientName)
    JXMLTOSTRINGLIST(OtherMessages)
  }

  void Job::ToXML(XMLNode node) const {

    // Proposed mandatory attributes for ARC 3.0
    STRINGTOXML(Name)
    URLTOXML(ServiceInformationURL)
    STRINGTOXML(ServiceInformationInterfaceName)
    URLTOXML(JobStatusURL)
    STRINGTOXML(JobStatusInterfaceName)
    URLTOXML(JobManagementURL)
    STRINGTOXML(JobManagementInterfaceName)

    URLTOXML(StageInDir)
    URLTOXML(StageOutDir)
    URLTOXML(SessionDir)

    STRINGTOXML(JobID)
    STRINGTOXML(Type)
    STRINGTOXML(IDFromEndpoint)
    STRINGTOXML(LocalIDFromManager)
    STRINGTOXML(JobDescription)
    STRINGTOXML(JobDescriptionDocument)
    if (State) {
      node.NewChild("State");
      node["State"].NewChild("Specific") = State();
      node["State"].NewChild("General") = State.GetGeneralState();
    }
    if (RestartState) {
      node.NewChild("RestartState");
      node["RestartState"].NewChild("Specific") = RestartState();
      node["RestartState"].NewChild("General") = RestartState.GetGeneralState();
    }
    INTTOXML(ExitCode)
    STRINGTOXML(ComputingManagerExitCode)
    STRINGLISTTOXML(Error)
    INTTOXML(WaitingPosition)
    STRINGTOXML(UserDomain)
    STRINGTOXML(Owner)
    STRINGTOXML(LocalOwner)
    PERIODTOSTRING(RequestedTotalWallTime)
    PERIODTOSTRING(RequestedTotalCPUTime)
    INTTOXML(RequestedSlots)
    STRINGLISTTOXML(RequestedApplicationEnvironment)
    STRINGTOXML(StdIn)
    STRINGTOXML(StdOut)
    STRINGTOXML(StdErr)
    STRINGTOXML(LogDir)
    STRINGLISTTOXML(ExecutionNode)
    STRINGTOXML(Queue)
    PERIODTOSTRING(UsedTotalWallTime)
    PERIODTOSTRING(UsedTotalCPUTime)
    INTTOXML(UsedMainMemory)
    TIMETOSTRING(LocalSubmissionTime)
    TIMETOSTRING(SubmissionTime)
    TIMETOSTRING(ComputingManagerSubmissionTime)
    TIMETOSTRING(StartTime)
    TIMETOSTRING(ComputingManagerEndTime)
    TIMETOSTRING(EndTime)
    TIMETOSTRING(WorkingAreaEraseTime)
    TIMETOSTRING(ProxyExpirationTime)
    STRINGTOXML(SubmissionHost)
    STRINGTOXML(SubmissionClientName)
    STRINGLISTTOXML(OtherMessages)

    if ((ActivityOldID.size() > 0 || LocalInputFiles.size() > 0) && !node["Associations"]) {
      node.NewChild("Associations");
    }

    for (std::list<std::string>::const_iterator it = ActivityOldID.begin();
         it != ActivityOldID.end(); it++) {
      node["Associations"].NewChild("ActivityOldID") = *it;
    }

    for (std::map<std::string, std::string>::const_iterator it = LocalInputFiles.begin();
         it != LocalInputFiles.end(); it++) {
      XMLNode lif = node["Associations"].NewChild("LocalInputFile");
      lif.NewChild("Source") = it->first;
      lif.NewChild("CheckSum") = it->second;
    }
  }

  void Job::SaveToStream(std::ostream& out, bool longlist) const {
    out << IString("Job: %s", JobID) << std::endl;
    if (!Name.empty())
      out << IString(" Name: %s", Name) << std::endl;
    if (!State().empty())
      out << IString(" State: %s (%s)", State.GetGeneralState(), State.GetSpecificState())
                << std::endl;
    if (State == JobState::QUEUING && WaitingPosition != -1) {
      out << IString(" Waiting Position: %d", WaitingPosition) << std::endl;
    }

    if (ExitCode != -1)
      out << IString(" Exit Code: %d", ExitCode) << std::endl;
    if (!Error.empty()) {
      for (std::list<std::string>::const_iterator it = Error.begin();
           it != Error.end(); it++)
        out << IString(" Job Error: %s", *it) << std::endl;
    }

    if (longlist) {
      if (!Owner.empty())
        out << IString(" Owner: %s", Owner) << std::endl;
      if (!OtherMessages.empty())
        for (std::list<std::string>::const_iterator it = OtherMessages.begin();
             it != OtherMessages.end(); it++)
          out << IString(" Other Messages: %s", *it)
                    << std::endl;
      if (!Queue.empty())
        out << IString(" Queue: %s", Queue) << std::endl;
      if (RequestedSlots != -1)
        out << IString(" Requested Slots: %d", RequestedSlots) << std::endl;
      if (WaitingPosition != -1)
        out << IString(" Waiting Position: %d", WaitingPosition)
                  << std::endl;
      if (!StdIn.empty())
        out << IString(" Stdin: %s", StdIn) << std::endl;
      if (!StdOut.empty())
        out << IString(" Stdout: %s", StdOut) << std::endl;
      if (!StdErr.empty())
        out << IString(" Stderr: %s", StdErr) << std::endl;
      if (!LogDir.empty())
        out << IString(" Computing Service Log Directory: %s", LogDir)
                  << std::endl;
      if (SubmissionTime != -1)
        out << IString(" Submitted: %s",
                             (std::string)SubmissionTime) << std::endl;
      if (EndTime != -1)
        out << IString(" End Time: %s", (std::string)EndTime)
                  << std::endl;
      if (!SubmissionHost.empty())
        out << IString(" Submitted from: %s", SubmissionHost)
                  << std::endl;
      if (!SubmissionClientName.empty())
        out << IString(" Submitting client: %s",
                             SubmissionClientName) << std::endl;
      if (RequestedTotalCPUTime != -1)
        out << IString(" Requested CPU Time: %s",
                             RequestedTotalCPUTime.istr())
                  << std::endl;
      if (UsedTotalCPUTime != -1)
        out << IString(" Used CPU Time: %s",
                             UsedTotalCPUTime.istr()) << std::endl;
      if (UsedTotalWallTime != -1)
        out << IString(" Used Wall Time: %s",
                             UsedTotalWallTime.istr()) << std::endl;
      if (UsedMainMemory != -1)
        out << IString(" Used Memory: %d", UsedMainMemory)
                  << std::endl;
      if (WorkingAreaEraseTime != -1)
        out << IString((State == JobState::DELETED) ?
                             istring(" Results were deleted: %s") :
                             istring(" Results must be retrieved before: %s"),
                             (std::string)WorkingAreaEraseTime)
                  << std::endl;
      if (ProxyExpirationTime != -1)
        out << IString(" Proxy valid until: %s",
                             (std::string)ProxyExpirationTime)
                  << std::endl;
      if (CreationTime != -1)
        out << IString(" Entry valid from: %s",
                             (std::string)CreationTime) << std::endl;
      if (Validity != -1)
        out << IString(" Entry valid for: %s",
                             Validity.istr()) << std::endl;

      if (!ActivityOldID.empty()) {
        out << IString(" Old job IDs:") << std::endl;
        for (std::list<std::string>::const_iterator it = ActivityOldID.begin();
             it != ActivityOldID.end(); ++it) {
          out << "  " << *it << std::endl;
        }
      }
      
      // Proposed mandatory attributes for ARC 3.0
      out << IString(" ID on service: %s", IDFromEndpoint) << std::endl;
      out << IString(" Service information URL: %s (%s)", ServiceInformationURL.fullstr(), ServiceInformationInterfaceName) << std::endl;
      out << IString(" Job status URL: %s (%s)", JobStatusURL.fullstr(), JobStatusInterfaceName) << std::endl;
      out << IString(" Job management URL: %s (%s)", JobManagementURL.fullstr(), JobManagementInterfaceName) << std::endl;
      if (StageInDir) out << IString(" Stagein directory URL: %s", StageInDir.fullstr()) << std::endl;
      if (StageOutDir) out << IString(" Stageout directory URL: %s", StageOutDir.fullstr()) << std::endl;
      if (SessionDir) out << IString(" Session directory URL: %s", SessionDir.fullstr()) << std::endl;
    }

    out << std::endl;
  } // end Print

  bool Job::PrepareHandler(const UserConfig& uc) {
    if (jc != NULL) return true;

    // If InterfaceName is not specified then all JobControllerPlugin classes should be tried.
    if (JobManagementInterfaceName.empty()) {
      logger.msg(VERBOSE, "Unable to handle job (%s), no interface specified.", JobID);
    }
    
    jc = getLoader().loadByInterfaceName(JobManagementInterfaceName, uc);
    if (!jc) {
      logger.msg(VERBOSE, "Unable to handle job (%s), no plugin associated with the specified interface (%s)", JobID, JobManagementInterfaceName);
      return false;
    }

    return true; 
  }

  bool Job::Update() { if (!jc) return false; std::list<Job*> jobs(1, this); jc->UpdateJobs(jobs); return true; }
  
  bool Job::Clean() { return jc ? jc->CleanJobs(std::list<Job*>(1, this)) : false; }
  
  bool Job::Cancel() { return jc ? jc->CancelJobs(std::list<Job*>(1, this)) : false; }

  bool Job::Resume() { return jc ? jc->ResumeJobs(std::list<Job*>(1, this)) : false; }

  bool Job::Renew() { return jc ? jc->RenewJobs(std::list<Job*>(1, this)) : false; }


  bool Job::GetURLToResource(ResourceType resource, URL& url) const { return jc ? jc->GetURLToJobResource(*this, resource, url) : false; }

  bool Job::Retrieve(const UserConfig& uc, const URL& destination, bool force) const {
    if (!destination) {
      logger.msg(ERROR, "Invalid download destination path specified (%s)", destination.fullstr());
      return false;
    }
    
    if (jc == NULL) {
      logger.msg(DEBUG, "Unable to download job (%s), no JobControllerPlugin plugin was set to handle the job.", JobID);
      return false;
    }

    logger.msg(VERBOSE, "Downloading job: %s", JobID);
    
    URL src, dst(destination);
    if (!jc->GetURLToJobResource(*this, STAGEOUTDIR, src)) {
      logger.msg(ERROR, "Cant retrieve job files for job (%s) - unable to determine URL of stage out directory", JobID);
      return false;
    }

    if (!src) {
      logger.msg(ERROR, "Invalid stage out path specified (%s)", src.fullstr());
      return false;
    }

    if (!force && Glib::file_test(dst.Path(), Glib::FILE_TEST_EXISTS)) {
      logger.msg(WARNING, "%s directory exist! Skipping job.", dst.Path());
      return false;
    }

    std::list<std::string> files;
    if (!ListFilesRecursive(uc, src, files)) {
      logger.msg(ERROR, "Unable to retrieve list of job files to download for job %s", JobID);
      return false;
    }

    const std::string srcpath = src.Path() + (src.Path().empty() || *src.Path().rbegin() != '/' ? "/" : "");
    const std::string dstpath = dst.Path() + (dst.Path().empty() || *dst.Path().rbegin() != G_DIR_SEPARATOR ? G_DIR_SEPARATOR_S : "");

    bool ok = true;
    for (std::list<std::string>::const_iterator it = files.begin(); it != files.end(); ++it) {
      src.ChangePath(srcpath + *it);
      dst.ChangePath(dstpath + *it);
      if (!CopyJobFile(uc, src, dst)) {
        logger.msg(INFO, "Failed downloading %s to %s", src.str(), dst.str());
        ok = false;
      }
    }

    return ok;
  }

  bool Job::ListFilesRecursive(const UserConfig& uc, const URL& dir, std::list<std::string>& files, const std::string& prefix) {
    std::list<FileInfo> outputfiles;

    DataHandle handle(dir, uc);
    if (!handle) {
      logger.msg(INFO, "Unable to list files at %s", dir.str());
      return false;
    }
    if(!handle->List(outputfiles, (Arc::DataPoint::DataPointInfoType)
                                  (DataPoint::INFO_TYPE_NAME | DataPoint::INFO_TYPE_TYPE))) {
      logger.msg(INFO, "Unable to list files at %s", dir.str());
      return false;
    }

    for (std::list<FileInfo>::iterator i = outputfiles.begin();
         i != outputfiles.end(); i++) {
      if (i->GetName() == ".." || i->GetName() == ".") {
        continue;
      }

      if (i->GetType() == FileInfo::file_type_unknown ||
          i->GetType() == FileInfo::file_type_file) {
        files.push_back(prefix + i->GetName());
      }
      else if (i->GetType() == FileInfo::file_type_dir) {
        std::string path = dir.Path();
        if (path[path.size() - 1] != '/') {
          path += "/";
        }
        URL tmpdir(dir);
        tmpdir.ChangePath(path + i->GetName());

        std::string dirname = prefix + i->GetName();
        if (dirname[dirname.size() - 1] != '/') {
          dirname += "/";
        }
        if (!ListFilesRecursive(uc, tmpdir, files, dirname)) {
          return false;
        }
      }
    }

    return true;
  }

  bool Job::CopyJobFile(const UserConfig& uc, const URL& src, const URL& dst) {
    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    logger.msg(VERBOSE, "Now copying (from -> to)");
    logger.msg(VERBOSE, " %s -> %s", src.str(), dst.str());

    URL src_(src);
    URL dst_(dst);
    src_.AddOption("checksum=no");
    dst_.AddOption("checksum=no");
    src_.AddOption("blocksize=1048576",false);
    dst_.AddOption("blocksize=1048576",false);

    if ((!data_source) || (!*data_source) ||
        (!(*data_source)->SetURL(src_))) {
      if(data_source) delete data_source;
      data_source = new DataHandle(src_, uc);
    }
    DataHandle& source = *data_source;
    if (!source) {
      logger.msg(ERROR, "Unable to initialise connection to source: %s", src.str());
      return false;
    }

    if ((!data_destination) || (!*data_destination) ||
        (!(*data_destination)->SetURL(dst_))) {
      if(data_destination) delete data_destination;
      data_destination = new DataHandle(dst_, uc);
    }
    DataHandle& destination = *data_destination;
    if (!destination) {
      logger.msg(ERROR, "Unable to initialise connection to destination: %s",
                 dst.str());
      return false;
    }

    // Set desired number of retries. Also resets any lost
    // tries from previous files.
    source->SetTries((src.Protocol() == "file")?1:3);
    destination->SetTries((dst.Protocol() == "file")?1:3);

    // Turn off all features we do not need
    source->SetAdditionalChecks(false);
    destination->SetAdditionalChecks(false);

    FileCache cache;
    DataStatus res =
      mover.Transfer(*source, *destination, cache, URLMap(), 0, 0, 0,
                     uc.Timeout());
    if (!res.Passed()) {
      logger.msg(ERROR, "File download failed: %s", std::string(res));
      // Reset connection because one can't be sure how failure
      // affects server and/or connection state.
      // TODO: Investigate/define DMC behavior in such case.
      delete data_source;
      data_source = NULL;
      delete data_destination;
      data_destination = NULL;
      return false;
    }

    return true;
  }


  Logger JobInformationStorageXML::logger(Logger::getRootLogger(), "JobInformationStorageXML");

  bool JobInformationStorageXML::ReadAll(std::list<Job>& jobs, const std::list<std::string>& rEndpoints) {
    jobs.clear();
    Config jobstorage;

    FileLock lock(name);
    bool acquired = false;
    for (int tries = (int)nTries; tries > 0; --tries) {
      acquired = lock.acquire();
      if (acquired) {
        if (!jobstorage.ReadFromFile(name)) {
          lock.release();
          return false;
        }
        lock.release();
        break;
      }

      if (tries == 6) {
        logger.msg(VERBOSE, "Waiting for lock on job list file %s", name);
      }

      Glib::usleep(tryInterval);
    }

    if (!acquired) {
      return false;
    }
    
    XMLNodeList xmljobs = jobstorage.Path("Job");
    for (XMLNodeList::iterator xit = xmljobs.begin(); xit != xmljobs.end(); ++xit) {
      jobs.push_back(*xit);
      for (std::list<std::string>::const_iterator rEIt = rEndpoints.begin();
           rEIt != rEndpoints.end(); ++rEIt) {
        if (jobs.back().JobManagementURL.StringMatches(*rEIt)) {
          jobs.pop_back();
          break;
        }
      }
    }

    return true;
  }

  bool JobInformationStorageXML::Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers, const std::list<std::string>& endpoints, const std::list<std::string>& rEndpoints) {
    if (!ReadAll(jobs, rEndpoints)) { return false; }

    std::list<std::string> jobIdentifiersCopy = jobIdentifiers;
    for (std::list<Job>::iterator itJ = jobs.begin();
         itJ != jobs.end();) {
      // Check if the job (itJ) is selected by the job identifies, either by job ID or Name.
      std::list<std::string>::iterator itJIdentifier = jobIdentifiers.begin();
      for (;itJIdentifier != jobIdentifiers.end(); ++itJIdentifier) {
        if ((!itJ->Name.empty() && itJ->Name == *itJIdentifier) ||
            (itJ->JobID == *itJIdentifier)) {
          break;
        }
      }
      if (itJIdentifier != jobIdentifiers.end()) {
        // Job explicitly specified. Remove id from the copy list, in order to keep track of used identifiers.
        std::list<std::string>::iterator itJIdentifierCopy;
        while ((itJIdentifierCopy = std::find(jobIdentifiersCopy.begin(), jobIdentifiersCopy.end(), *itJIdentifier))
               != jobIdentifiersCopy.end()) {
          jobIdentifiersCopy.erase(itJIdentifierCopy);
        }
        ++itJ;
        continue;
      }

      // Check if the job (itJ) is selected by endpoints.
      std::list<std::string>::const_iterator itC = endpoints.begin();
      for (; itC != endpoints.end(); ++itC) {
        if (itJ->JobManagementURL.StringMatches(*itC)) {
          break;
        }
      }
      if (itC != endpoints.end()) {
        // Cluster on which job reside is explicitly specified.
        ++itJ;
        continue;
      }

      // Job is not selected - remove it.
      itJ = jobs.erase(itJ);
    }

    jobIdentifiers = jobIdentifiersCopy;

    return true;
  }

  bool JobInformationStorageXML::Clean() {
    Config jobfile;
    FileLock lock(name);
    for (int tries = (int)nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        if (!jobfile.SaveToFile(name)) {
          lock.release();
          return false;
        }
        lock.release();
        return true;
      }

      if (tries == 6) {
        logger.msg(WARNING, "Waiting for lock on job list file %s", name);
      }

      Glib::usleep(tryInterval);
    }

    return false;
  }

  bool JobInformationStorageXML::Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs) {
    FileLock lock(name);
    for (int tries = (int)nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        Config jobfile;
        jobfile.ReadFromFile(name);

        // Use std::map to store job IDs to be searched for duplicates.
        std::map<std::string, XMLNode> jobIDXMLMap;
        std::map<std::string, XMLNode> jobsToRemove;
        for (Arc::XMLNode j = jobfile["Job"]; j; ++j) {
          if (!((std::string)j["JobID"]).empty()) {
            std::string serviceName = URL(j["ServiceInformationURL"]).Host();
            if (!serviceName.empty() && prunedServices.count(serviceName)) {
              logger.msg(DEBUG, "Will remove %s on service %s.",
                         ((std::string)j["JobID"]).c_str(), serviceName);
              jobsToRemove[(std::string)j["JobID"]] = j;
            }
            else {
              jobIDXMLMap[(std::string)j["JobID"]] = j;
            }
          }
        }

        // Remove jobs which belong to our list of endpoints to prune.
        for (std::map<std::string, XMLNode>::iterator it = jobsToRemove.begin();
             it != jobsToRemove.end(); ++it) {
          it->second.Destroy();
        }

        std::map<std::string, const Job*> newJobsMap;
        for (std::list<Job>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
          std::map<std::string, XMLNode>::iterator itJobXML = jobIDXMLMap.find(it->JobID);
          if (itJobXML == jobIDXMLMap.end()) {
            XMLNode xJob = jobfile.NewChild("Job");
            it->ToXML(xJob);
            jobIDXMLMap[it->JobID] = xJob;

            std::map<std::string, XMLNode>::iterator itRemovedJobs = jobsToRemove.find(it->JobID);
            if (itRemovedJobs == jobsToRemove.end()) {
              newJobsMap[it->JobID] = &(*it);
            }
          }
          else {
            // Duplicate found, replace it.
            itJobXML->second.Replace(XMLNode(NS(), "Job"));
            it->ToXML(itJobXML->second);

            // Only add to newJobsMap if this is a new job, i.e. not previous present in jobfile.
            std::map<std::string, const Job*>::iterator itNewJobsMap = newJobsMap.find(it->JobID);
            if (itNewJobsMap != newJobsMap.end()) {
              itNewJobsMap->second = &(*it);
            }
          }
        }

        // Add pointers to new Job objects to the newJobs list.
        for (std::map<std::string, const Job*>::const_iterator it = newJobsMap.begin();
             it != newJobsMap.end(); ++it) {
          newJobs.push_back(it->second);
        }

        if (!jobfile.SaveToFile(name)) {
          lock.release();
          return false;
        }
        lock.release();
        return true;
      }

      if (tries == 6) {
        logger.msg(WARNING, "Waiting for lock on job list file %s", name);
      }

      Glib::usleep(tryInterval);
    }

    return false;
  }

  bool JobInformationStorageXML::Remove(const std::list<std::string>& jobids) {
    if (jobids.empty()) {
      return true;
    }

    FileLock lock(name);
    for (int tries = nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        Config jobstorage;
        if (!jobstorage.ReadFromFile(name)) {
          lock.release();
          return false;
        }

        XMLNodeList xmlJobs = jobstorage.Path("Job");
        for (std::list<std::string>::const_iterator it = jobids.begin(); it != jobids.end(); ++it) {
          for (XMLNodeList::iterator xJIt = xmlJobs.begin(); xJIt != xmlJobs.end(); ++xJIt) {
            if ((*xJIt)["JobID"] == *it ||
                (*xJIt)["IDFromEndpoint"] == *it // Included for backwards compatibility.
                ) {
              xJIt->Destroy(); // Do not break, since for some reason there might be multiple identical jobs in the file.
            }
          }
        }

        if (!jobstorage.SaveToFile(name)) {
          lock.release();
          return false;
        }
        lock.release();
        return true;
      }

      if (tries == 6) {
        logger.msg(VERBOSE, "Waiting for lock on job list file %s", name);
      }
      Glib::usleep(tryInterval);
    }

    return false;
  }


#ifdef DBJSTORE_ENABLED
  static void* store_string(const std::string& str, void* buf) {
    uint32_t l = str.length();
    unsigned char* p = (unsigned char*)buf;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    ::memcpy(p,str.c_str(),str.length());
    p += str.length();
    return (void*)p;
  }

  static void* parse_string(std::string& str, const void* buf, uint32_t& size) {
    uint32_t l = 0;
    const unsigned char* p = (unsigned char*)buf;
    if(size < 4) { p += size; size = 0; return (void*)p; };
    l |= ((uint32_t)(*p)) << 0; ++p; --size;
    l |= ((uint32_t)(*p)) << 8; ++p; --size;
    l |= ((uint32_t)(*p)) << 16; ++p; --size;
    l |= ((uint32_t)(*p)) << 24; ++p; --size;
    if(l > size) l = size;
    // TODO: sanity check
    str.assign((const char*)p,l);
    p += l; size -= l;
    return (void*)p;
  }
   
  static void serialiseJob(const Job& j, Dbt& data) {
    const std::string version = "3.0.0";
    const unsigned nItems = 14;
    const std::string dataItems[] =
      {version, j.IDFromEndpoint, j.Name,
       j.JobStatusInterfaceName, j.JobStatusURL.fullstr(),
       j.JobManagementInterfaceName, j.JobManagementURL.fullstr(),
       j.ServiceInformationInterfaceName, j.ServiceInformationURL.fullstr(),
       j.SessionDir.fullstr(), j.StageInDir.fullstr(), j.StageOutDir.fullstr(),
       j.JobDescriptionDocument, tostring(j.LocalSubmissionTime.GetTime())};
      
    data.set_data(NULL); data.set_size(0);
    uint32_t l = 0;
    for (unsigned i = 0; i < nItems; ++i) l += 4 + dataItems[i].length();
    void* d = (void*)::malloc(l);
    if(!d) return;
    data.set_data(d); data.set_size(l);
    
    for (unsigned i = 0; i < nItems; ++i) d = store_string(dataItems[i], d);
  }
 
  static void deserialiseJob(Job& j, const Dbt& data) {
    uint32_t size = 0;
    void* d = NULL;

    d = (void*)data.get_data();
    size = (uint32_t)data.get_size();
    
    std::string version;
    d = parse_string(version, d, size);
    if (version == "3.0.0") {
      /* Order of items in record. Version 3.0.0
          {version, j.IDFromEndpoint, j.Name,
           j.JobStatusInterfaceName, j.JobStatusURL.fullstr(),
           j.JobManagementInterfaceName, j.JobManagementURL.fullstr(),
           j.ServiceInformationInterfaceName, j.ServiceInformationURL.fullstr(),
           j.SessionDir.fullstr(), j.StageInDir.fullstr(), j.StageOutDir.fullstr(),
           j.JobDescriptionDocument, tostring(j.LocalSubmissionTime.GetTime())};
       */
      std::string s;
      d = parse_string(j.IDFromEndpoint, d, size);
      d = parse_string(j.Name, d, size);
      d = parse_string(j.JobStatusInterfaceName, d, size);
      d = parse_string(s, d, size); j.JobStatusURL = URL(s);
      d = parse_string(j.JobManagementInterfaceName, d, size);
      d = parse_string(s, d, size); j.JobManagementURL = URL(s);
      d = parse_string(j.ServiceInformationInterfaceName, d, size);
      d = parse_string(s, d, size); j.ServiceInformationURL = URL(s);
      d = parse_string(s, d, size); j.SessionDir = URL(s);
      d = parse_string(s, d, size); j.StageInDir = URL(s);
      d = parse_string(s, d, size); j.StageOutDir = URL(s);
      d = parse_string(j.JobDescriptionDocument, d, size);
      d = parse_string(s, d, size); j.LocalSubmissionTime.SetTime(stringtoi(s));
    }
  }
  
  static void deserialiseNthJobAttribute(std::string& attr, const Dbt& data, unsigned n) {
    uint32_t size = 0;
    void* d = NULL;

    d = (void*)data.get_data();
    size = (uint32_t)data.get_size();
    
    std::string version;
    d = parse_string(version, d, size);
    if (version == "3.0.0") {
      for (unsigned i = 0; i < n-1; ++i) {
        d = parse_string(attr, d, size);
      }
    }
  }
  
  static int getNameKey(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result) {
    std::string name;
    // 3rd attribute in job record is job name.
    deserialiseNthJobAttribute(name, *data, 3);
    result->set_size(name.size());
    result->set_data((char *)name.c_str());
    return 0;
  }
  
  static int getEndpointKey(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result) {
    std::string endpointS;
    // 7th attribute in job record is job management URL.
    deserialiseNthJobAttribute(endpointS, *data, 7);
    endpointS = URL(endpointS).Host();
    if (endpointS.empty()) {
      return DB_DONOTINDEX;
    }
    result->set_size(endpointS.size());
    result->set_data((char *)endpointS.c_str());
    return 0;
  }
  
  static int getServiceInfoHostnameKey(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result) {
    std::string endpointS;
    // 9th attribute in job record is service information URL.
    deserialiseNthJobAttribute(endpointS, *data, 9);
    endpointS = URL(endpointS).Host();
    if (endpointS.empty()) {
      return DB_DONOTINDEX;
    }
    result->set_size(endpointS.size());
    result->set_data((char *)endpointS.c_str());
    return 0;
  }
  
  Logger JobInformationStorageBDB::logger(Logger::getRootLogger(), "JobInformationStorageBDB");

  JobInformationStorageBDB::JobDB::JobDB(const std::string& name, u_int32_t flags)
    : dbEnv(NULL), jobDB(NULL), endpointSecondaryKeyDB(NULL), nameSecondaryKeyDB(NULL), serviceInfoSecondaryKeyDB(NULL)
  {
    int ret;
    const DBTYPE type = (flags == DB_CREATE ? DB_BTREE : DB_UNKNOWN);
    std::string basepath = "";
    
    dbEnv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    if ((ret = dbEnv->open(NULL, DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL, 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to create data base environment (%s)", name);
      throw std::exception();
    }

    jobDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);
    nameSecondaryKeyDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);
    endpointSecondaryKeyDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);
    serviceInfoSecondaryKeyDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);

    if ((ret = nameSecondaryKeyDB->set_flags(DB_DUPSORT)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to set duplicate flags for secondary key DB (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }
    if ((ret = endpointSecondaryKeyDB->set_flags(DB_DUPSORT)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to set duplicate flags for secondary key DB (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }
    if ((ret = serviceInfoSecondaryKeyDB->set_flags(DB_DUPSORT)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to set duplicate flags for secondary key DB (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }

    if ((ret = jobDB->open(NULL, name.c_str(), "job_records", type, flags, 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to create job database (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }
    if ((ret = nameSecondaryKeyDB->open(NULL, name.c_str(), "name_keys", type, flags, 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to create DB for secondary name keys (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }
    if ((ret = endpointSecondaryKeyDB->open(NULL, name.c_str(), "endpoint_keys", type, flags, 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to create DB for secondary endpoint keys (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }
    if ((ret = serviceInfoSecondaryKeyDB->open(NULL, name.c_str(), "serviceinfo_keys", type, flags, 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to create DB for secondary service info keys (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }

    if ((ret = jobDB->associate(NULL, nameSecondaryKeyDB, (flags != DB_RDONLY ? getNameKey : NULL), 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to associate secondary DB with primary DB (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }
    if ((ret = jobDB->associate(NULL, endpointSecondaryKeyDB, (flags != DB_RDONLY ? getEndpointKey : NULL), 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to associate secondary DB with primary DB (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }
    if ((ret = jobDB->associate(NULL, serviceInfoSecondaryKeyDB, (flags != DB_RDONLY ? getServiceInfoHostnameKey : NULL), 0)) != 0) {
      JobInformationStorageBDB::logger.msg(ERROR, "Unable to associate secondary DB with primary DB (%s)", name);
      logErrorMessage(ret);
      throw std::exception();
    }

    JobInformationStorageBDB::logger.msg(DEBUG, "Job database created successfully (%s)", name);
  }

  JobInformationStorageBDB::JobDB::~JobDB() {
    if (nameSecondaryKeyDB) {
      nameSecondaryKeyDB->close(0);
    }
    if (endpointSecondaryKeyDB) {
      endpointSecondaryKeyDB->close(0);
    }
    if (serviceInfoSecondaryKeyDB) {
      serviceInfoSecondaryKeyDB->close(0);
    }
    if (jobDB) {
      jobDB->close(0);
    }
    if (dbEnv) {
      dbEnv->close(0);
    }

    delete endpointSecondaryKeyDB; endpointSecondaryKeyDB = NULL; 
    delete nameSecondaryKeyDB; nameSecondaryKeyDB = NULL;
    delete serviceInfoSecondaryKeyDB; serviceInfoSecondaryKeyDB = NULL;
    delete jobDB; jobDB = NULL;
    delete dbEnv; dbEnv = NULL;

    dbEnv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    dbEnv->remove(NULL, 0);
    delete dbEnv; dbEnv = NULL;
  }

  bool JobInformationStorageBDB::Write(const std::list<Job>& jobs) {
    if (jobs.empty()) return true;
    
    try {
      JobDB db(name, DB_CREATE);
      int ret;
      std::list<Job>::const_iterator it = jobs.begin();
      void* pdata = NULL;
      Dbt key, data;
      do {
        ::free(pdata);
        key.set_size(it->JobID.size());
        key.set_data((char*)it->JobID.c_str());
        serialiseJob(*it, data);
        pdata = data.get_data();
      } while ((ret = db->put(NULL, &key, &data, 0)) == 0 && ++it != jobs.end());
      ::free(pdata);
      
      if (ret != 0) {
        logger.msg(ERROR, "Unable to write key/value pair to job database (%s): Key \"%s\"", name, (char*)key.get_data());
        logErrorMessage(ret);
        return false;
      }
    } catch (const std::exception& e) {
      return false;
    }

    return true;
  }

  bool JobInformationStorageBDB::Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs) {
    if (jobs.empty()) return true;
    
    try {
      JobDB db(name, DB_CREATE);
      int ret = 0;
      
      std::set<std::string> idsOfPrunedJobs;
      Dbc *cursor = NULL;
      if ((ret = db.viaServiceInfoKeys()->cursor(NULL, &cursor, DB_WRITECURSOR)) != 0) return false;
      for (std::set<std::string>::const_iterator itPruned = prunedServices.begin();
           itPruned != prunedServices.end(); ++itPruned) {
        Dbt key((char *)itPruned->c_str(), itPruned->size()), pkey, data;
        if (cursor->pget(&key, &pkey, &data, DB_SET) != 0) continue;
        do {
          idsOfPrunedJobs.insert(std::string((char *)pkey.get_data(), pkey.get_size()));
          cursor->del(0);
        } while (cursor->pget(&key, &pkey, &data, DB_NEXT_DUP) == 0);
      }
      cursor->close();
      
      std::list<Job>::const_iterator it = jobs.begin();
      void* pdata = NULL;
      Dbt key, data;
      bool jobWasPruned;
      do {
        ::free(pdata);
        key.set_size(it->JobID.size());
        key.set_data((char*)it->JobID.c_str());
        serialiseJob(*it, data);
        pdata = data.get_data();
        jobWasPruned = (idsOfPrunedJobs.count(it->JobID) != 0);
        if (!jobWasPruned) { // Check if job already exist.
          Dbt existingData;
          if (db->get(NULL, &key, &existingData, 0) == DB_NOTFOUND) {
            newJobs.push_back(&*it);
          }
        }
      } while (((ret = db->put(NULL, &key, &data, 0)) == 0 && ++it != jobs.end()));
      ::free(pdata);
      
      if (ret != 0) {
        logger.msg(ERROR, "Unable to write key/value pair to job database (%s): Key \"%s\"", name, (char*)key.get_data());
        logErrorMessage(ret);
        return false;
      }
    } catch (const std::exception& e) {
      return false;
    }

    return true;
  }

  bool JobInformationStorageBDB::ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints) {
    jobs.clear();

    try {
      int ret;
      JobDB db(name);
  
      Dbc *cursor;
      if ((ret = db->cursor(NULL, &cursor, 0)) != 0) {
        //dbp->err(dbp, ret, "DB->cursor");
        return false;
      }
      
      Dbt key, data;
      while ((ret = cursor->get(&key, &data, DB_NEXT)) == 0) {
        jobs.push_back(Job());
        jobs.back().JobID = std::string((char *)key.get_data(), key.get_size());
        deserialiseJob(jobs.back(), data);
        for (std::list<std::string>::const_iterator it = rejectEndpoints.begin();
             it != rejectEndpoints.end(); ++it) {
          if (jobs.back().JobManagementURL.StringMatches(*it)) {
            jobs.pop_back();
            break;
          }
        }
      }
  
      cursor->close();
      
      if (ret != DB_NOTFOUND) {
        //dbp->err(dbp, ret, "DBcursor->get");
        return false;
      }
    } catch (const std::exception& e) {
      return false;
    }
    
    return true;
  }
  
  static void addJobFromDB(const Dbt& key, const Dbt& data, std::list<Job>& jobs, std::set<std::string>& idsOfAddedJobs, const std::list<std::string>& rejectEndpoints) {
    jobs.push_back(Job());
    jobs.back().JobID.assign((char *)key.get_data(), key.get_size());
    deserialiseJob(jobs.back(), data);
    if (idsOfAddedJobs.count(jobs.back().JobID) != 0) { // Look for duplicates and remove them.
      jobs.pop_back();
      return;
    }
    idsOfAddedJobs.insert(jobs.back().JobID);
    for (std::list<std::string>::const_iterator it = rejectEndpoints.begin();
         it != rejectEndpoints.end(); ++it) {
      if (jobs.back().JobManagementURL.StringMatches(*it)) {
        idsOfAddedJobs.erase(jobs.back().JobID);
        jobs.pop_back();
        return;
      }
    }
  }

  static bool addJobsFromDuplicateKeys(Db& db, Dbt& key, std::list<Job>& jobs, std::set<std::string>& idsOfAddedJobs, const std::list<std::string>& rejectEndpoints) {
    int ret;
    Dbt pkey, data;
    Dbc *cursor;
    if ((ret = db.cursor(NULL, &cursor, 0)) != 0) {
      //dbp->err(dbp, ret, "DB->cursor");
      return false;
    }
    ret = cursor->pget(&key, &pkey, &data, DB_SET);
    if (ret != 0) return false;
    
    addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
    while ((ret = cursor->pget(&key, &pkey, &data, DB_NEXT_DUP)) == 0) {
       addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
    }
    return true;
  }
  
  bool JobInformationStorageBDB::Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                                      const std::list<std::string>& endpoints,
                                      const std::list<std::string>& rejectEndpoints) {
    jobs.clear();
    
    try {
      JobDB db(name);
      int ret;
      std::set<std::string> idsOfAddedJobs;
      for (std::list<std::string>::iterator it = jobIdentifiers.begin();
           it != jobIdentifiers.end();) {
        if (it->empty()) continue;
        
        Dbt key((char *)it->c_str(), it->size()), data;
        ret = db->get(NULL, &key, &data, 0);
        if (ret == DB_NOTFOUND) {
          if (addJobsFromDuplicateKeys(*db.viaNameKeys(), key, jobs, idsOfAddedJobs, rejectEndpoints)) {
            it = jobIdentifiers.erase(it);
          }
          else {
            ++it;
          }
          continue;
        }
        addJobFromDB(key, data, jobs, idsOfAddedJobs, rejectEndpoints);
        it = jobIdentifiers.erase(it);
      }
      if (endpoints.empty()) return true;
      
      Dbc *cursor;
      if ((ret = db.viaEndpointKeys()->cursor(NULL, &cursor, 0)) != 0) return false;
      for (std::list<std::string>::const_iterator it = endpoints.begin();
           it != endpoints.end(); ++it) {
        // Extract hostname from iterator.
        URL u(*it);
        if (u.Protocol() == "file") {
          u = URL("http://" + *it); // Only need to extract hostname. Prefix with "http://".
        }
        if (u.Host().empty()) continue;

        Dbt key((char *)u.Host().c_str(), u.Host().size()), pkey, data;
        ret = cursor->pget(&key, &pkey, &data, DB_SET);
        if (ret != 0) {
          continue;
        }
        std::string tmpEndpoint;
        deserialiseNthJobAttribute(tmpEndpoint, data, 7);
        URL jobManagementURL(tmpEndpoint);
        if (jobManagementURL.StringMatches(*it)) {
          addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
        }
        while ((ret = cursor->pget(&key, &pkey, &data, DB_NEXT_DUP)) == 0) {
          deserialiseNthJobAttribute(tmpEndpoint, data, 7);
          URL jobManagementURL(tmpEndpoint);
          if (jobManagementURL.StringMatches(*it)) {
           addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
         }
        }
      }
    } catch (const std::exception& e) {
      return false;
    }

    return true;
  }
  
  bool JobInformationStorageBDB::Clean() {
    if (remove(name.c_str()) != 0) {
      if (errno == ENOENT) return true; // No such file. DB already cleaned.
      logger.msg(ERROR, "Unable to truncate job database (%s)", name);
      perror("Error");
      return false;
    }
    
    return true;
  }
  
  bool JobInformationStorageBDB::Remove(const std::list<std::string>& jobids) {
    try {
      JobDB db(name, DB_CREATE);
      for (std::list<std::string>::const_iterator it = jobids.begin();
           it != jobids.end(); ++it) {
        Dbt key((char *)it->c_str(), it->size());
        db->del(NULL, &key, 0);
      }
    } catch (const std::exception& e) {
      return false;
    }
    
    return true;
  }
  
  void JobInformationStorageBDB::logErrorMessage(int err) {
    switch (err) {
    case ENOENT:
      logger.msg(DEBUG, "ENOENT: The file or directory does not exist, Or a nonexistent re_source file was specified.");
      break;
    case DB_OLD_VERSION:
      logger.msg(DEBUG, "DB_OLD_VERSION: The database cannot be opened without being first upgraded.");
      break;
    case EEXIST:
      logger.msg(DEBUG, "EEXIST: DB_CREATE and DB_EXCL were specified and the database exists.");
    case EINVAL:
      logger.msg(DEBUG, "EINVAL");
      break;
    default:
      logger.msg(DEBUG, "Unable to determine error (%d)", err);
    }
  }
#endif

  bool Job::ReadJobIDsFromFile(const std::string& filename, std::list<std::string>& jobids, unsigned nTries, unsigned tryInterval) {
    if (!Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) return false;

    FileLock lock(filename);
    for (int tries = (int)nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        std::ifstream is(filename.c_str());
        if (!is.good()) {
          is.close();
          lock.release();
          return false;
        }
        std::string line;
        while (std::getline(is, line)) {
          line = Arc::trim(line, " \t");
          if (!line.empty() && line[0] != '#') {
            jobids.push_back(line);
          }
        }
        is.close();
        lock.release();
        return true;
      }

      if (tries == 6) {
        logger.msg(WARNING, "Waiting for lock on file %s", filename);
      }

      Glib::usleep(tryInterval);
    }

    return false;
  }

  bool Job::WriteJobIDToFile(const std::string& jobid, const std::string& filename, unsigned nTries, unsigned tryInterval) {
    if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) return false;

    FileLock lock(filename);
    for (int tries = (int)nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        std::ofstream os(filename.c_str(), std::ios::app);
        if (!os.good()) {
          os.close();
          lock.release();
          return false;
        }
        os << jobid << std::endl;
        bool good = os.good();
        os.close();
        lock.release();
        return good;
      }

      if (tries == 6) {
        logger.msg(WARNING, "Waiting for lock on file %s", filename);
      }

      Glib::usleep(tryInterval);
    }

    return false;
  }

  bool Job::WriteJobIDsToFile(const std::list<std::string>& jobids, const std::string& filename, unsigned nTries, unsigned tryInterval) {
    if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) return false;
    FileLock lock(filename);
    for (int tries = (int)nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        std::ofstream os(filename.c_str(), std::ios::app);
        if (!os.good()) {
          os.close();
          lock.release();
          return false;
        }
        for (std::list<std::string>::const_iterator it = jobids.begin();
             it != jobids.end(); ++it) {
          os << *it << std::endl;
        }

        bool good = os.good();
        os.close();
        lock.release();
        return good;
      }

      if (tries == 6) {
        logger.msg(WARNING, "Waiting for lock on file %s", filename);
      }

      Glib::usleep(tryInterval);
    }

    return false;
  }

  bool Job::WriteJobIDsToFile(const std::list<Job>& jobs, const std::string& filename, unsigned nTries, unsigned tryInterval) {
    if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) return false;

    FileLock lock(filename);
    for (int tries = (int)nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        std::ofstream os(filename.c_str(), std::ios::app);
        if (!os.good()) {
          os.close();
          lock.release();
          return false;
        }
        for (std::list<Job>::const_iterator it = jobs.begin();
             it != jobs.end(); ++it) {
          os << it->JobID << std::endl;
        }

        bool good = os.good();
        os.close();
        lock.release();
        return good;
      }

      if (tries == 6) {
        logger.msg(WARNING, "Waiting for lock on file %s", filename);
      }

      Glib::usleep(tryInterval);
    }

    return false;
  }

} // namespace Arc
