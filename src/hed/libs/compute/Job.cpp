// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/FileUtils.h>
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
    DelegationID = j.DelegationID;

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
      else if ((std::string)job["Flavour"] == "INTERNAL") {
        ServiceInformationInterfaceName = "org.nordugrid.internal";
        JobStatusInterfaceName          = "org.nordugrid.internal";
        JobManagementInterfaceName      = "org.nordugrid.internal";
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

    if (job["Associations"]["DelegationID"]) {
      DelegationID.clear();
      for (XMLNode n = job["Associations"]["DelegationID"]; n; ++n) {
        DelegationID.push_back((std::string)n);
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

    if ((ActivityOldID.size() > 0 || LocalInputFiles.size() > 0 || DelegationID.size() > 0) &&
        !node["Associations"]) {
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

    for (std::list<std::string>::const_iterator it = DelegationID.begin();
         it != DelegationID.end(); it++) {
      node["Associations"].NewChild("DelegationID") = *it;
    }
  }

  void Job::SaveToStream(std::ostream& out, bool longlist) const {
    out << IString("Job: %s", JobID) << std::endl;
    if (!Name.empty())
      out << IString(" Name: %s", Name) << std::endl;
    out << IString(" State: %s", State.GetGeneralState())
              << std::endl;
    if (longlist && !State().empty()) {
      out << IString(" Specific state: %s", State.GetSpecificState())
                << std::endl;
    }
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
      if (UsedTotalCPUTime != -1) {
        if (RequestedSlots > 1) {
          out << IString(" Used CPU Time: %s (%s per slot)",
                             UsedTotalCPUTime.istr(),
                             Arc::Period(UsedTotalCPUTime.GetPeriod() / RequestedSlots).istr()) << std::endl;
        } else {
          out << IString(" Used CPU Time: %s",
                             UsedTotalCPUTime.istr()) << std::endl;
        }
      }
      if (UsedTotalWallTime != -1) {
        if (RequestedSlots > 1) {
          out << IString(" Used Wall Time: %s (%s per slot)",
                             UsedTotalWallTime.istr(),
                             Arc::Period(UsedTotalWallTime.GetPeriod() / RequestedSlots).istr()) << std::endl;
        } else {
          out << IString(" Used Wall Time: %s",
                             UsedTotalWallTime.istr()) << std::endl;
        }
      }
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
      if (!DelegationID.empty()) {
        out << IString(" Delegation IDs:") << std::endl;
        for (std::list<std::string>::const_iterator it = DelegationID.begin();
             it != DelegationID.end(); ++it) {
          out << "  " << *it << std::endl;
        }
      }
    }

    out << std::endl;
  } // end Print

  namespace _utils {

    class JSONPair {
      public:
        JSONPair(char const * tag, std::string const & value) : tag(tag), value(value) {};
        char const * tag;
        std::string const & value;
    };

    class JSONPairNum {
      public:
        JSONPairNum(char const * tag, int64_t value) : tag(tag), value(value) {};
        char const * tag;
        int64_t value;
    };

    class JSONSimpleArray {
      public:
        JSONSimpleArray(char const * tag, std::list<std::string> const & values) : tag(tag), values(values) {};
        char const * tag;
        std::list<std::string> const & values;
    };

    class JSONStart {
      public:
        JSONStart(int ident = 0):ident(ident),first(true) {};
        int operator++() { ++ident; first=true; return ident; };
        int operator--() { --ident; return ident; };
        int ident;
        bool first;
    };


    std::ostream& operator<<(std::ostream& out, JSONPair const & pair) {
      out << "\"" << pair.tag << "\": \"" << pair.value << "\"";
      return out;
    }

    std::ostream& operator<<(std::ostream& out, JSONPairNum const & pair) {
      out << "\"" << pair.tag << "\": " << pair.value;
      return out;
    }

    std::ostream& operator<<(std::ostream& out, JSONSimpleArray const & list) {
      out << "\"" << list.tag << "\": [";
      for (std::list<std::string>::const_iterator it = list.values.begin(); it != list.values.end(); it++) {
        out << (it==list.values.begin()?"":",") << std::endl;
        out << "\"" << *it << "\"";
      }
      out << std::endl;
      out << "]";
      return out;
    }

    std::ostream& operator<<(std::ostream& out, JSONStart& st) {
      if(!st.first) out << "," << std::endl;
      st.first=false;
      for(int n=0; n<st.ident; ++n) out << " ";
      return out;
    }

  } // namespace _utils

  void Job::SaveToStreamJSON(std::ostream& out, bool longlist) const {
    using namespace _utils;

    out << "{" << std::endl;
    JSONStart s(1);
    out << s << JSONPair("id", JobID);
    if (!Name.empty())
      out << s << JSONPair("name", Name);
    out << s << JSONPair("state", State.GetGeneralState());
    if (longlist && !State().empty()) {
      out << s << JSONPair("specificState", State.GetSpecificState());
    }
    if (State == JobState::QUEUING && WaitingPosition != -1) {
      out << s << JSONPairNum("waitingPosition", WaitingPosition);
    }

    if (ExitCode != -1)
      out << s << JSONPairNum("exitCode", ExitCode);
    if (!Error.empty()) {
      out << s << JSONSimpleArray("jobError", Error);
    }

    if (longlist) {
      if (!Owner.empty())
        out << s << JSONPair("owner", Owner);
      if (!OtherMessages.empty()) {
        out << s << JSONSimpleArray("otherMessages", OtherMessages);
      }
      if (!Queue.empty())
        out << s << JSONPair("queue", Queue);
      if (RequestedSlots != -1)
        out << s << JSONPairNum("requestedSlots", RequestedSlots);
      if (WaitingPosition != -1)
        out << s << JSONPairNum("waitingPosition", WaitingPosition);
      if (!StdIn.empty())
        out << s << JSONPair("stdin", StdIn);
      if (!StdOut.empty())
        out << s << JSONPair("stdout", StdOut);
      if (!StdErr.empty())
        out << s << JSONPair("stderr", StdErr);
      if (!LogDir.empty())
        out << s << JSONPair("logDirectory", LogDir);
      if (SubmissionTime != -1)
        out << s << JSONPair("submitted", (std::string)SubmissionTime);
      if (EndTime != -1)
        out << s << JSONPair("endTime", (std::string)EndTime);
      if (!SubmissionHost.empty())
        out << s << JSONPair("submittedFrom", SubmissionHost);
      if (!SubmissionClientName.empty())
        out << s << JSONPair("submittingClient", SubmissionClientName);
      if (RequestedTotalCPUTime != -1)
        out << s << JSONPair("requestedCPUTime", (std::string)RequestedTotalCPUTime);
      if (UsedTotalCPUTime != -1) {
        if (RequestedSlots > 1) {
          out << s << JSONPair("usedCPUTimeTotal", (std::string)UsedTotalCPUTime);
          out << s << JSONPair("usedCPUTimePerSlot", (std::string)Arc::Period(UsedTotalCPUTime.GetPeriod() / RequestedSlots));
        } else {
          out << s << JSONPair("usedCPUTimeTotal", (std::string)UsedTotalCPUTime);
        }
      }
      if (UsedTotalWallTime != -1) {
        if (RequestedSlots > 1) {
          out << s << JSONPair("usedWallTimeTotal", (std::string)UsedTotalWallTime);
          out << s << JSONPair("usedWallTimePerSlot", (std::string)Arc::Period(UsedTotalWallTime.GetPeriod() / RequestedSlots));
        } else {
          out << s << JSONPair("usedWallTimeTotal", (std::string)UsedTotalWallTime);
        }
      }
      if (UsedMainMemory != -1)
        out << s << JSONPairNum("usedMemory", UsedMainMemory);
      if (WorkingAreaEraseTime != -1) {
        if (State == JobState::DELETED)
          out << s << JSONPair("resultsDeleted", (std::string)WorkingAreaEraseTime);
        else
          out << s << JSONPair("resultsMustBeRetrieved", (std::string)WorkingAreaEraseTime);
      }
      if (ProxyExpirationTime != -1)
        out << s << JSONPair("proxyValidUntil", (std::string)ProxyExpirationTime);
      if (CreationTime != -1)
        out << s << JSONPair("validFrom", (std::string)CreationTime);
      if (Validity != -1)
        out << s << JSONPair("validFor", (std::string)Validity);

      if (!ActivityOldID.empty()) out << s << JSONSimpleArray("jobOldId", ActivityOldID);
      
      // Proposed mandatory attributes for ARC 3.0
      out << s << JSONPair("serviceId", IDFromEndpoint);
      out << s << JSONPair("serviceInformationURL", ServiceInformationURL.fullstr());
      out << s << JSONPair("serviceInformationInterface", ServiceInformationInterfaceName);
      out << s << JSONPair("jobStatusURL", JobStatusURL.fullstr());
      out << s << JSONPair("jobStatusInterface", JobStatusInterfaceName);
      out << s << JSONPair("jobManagementURL", JobManagementURL.fullstr());
      out << s << JSONPair("jobManagementInterface", JobManagementInterfaceName);
      if (StageInDir) out << s << JSONPair("stageinURL", StageInDir.fullstr());
      if (StageOutDir) out << s << JSONPair("stageoutURL", StageOutDir.fullstr());
      if (SessionDir) out << s << JSONPair("sessionURL", SessionDir.fullstr());
      if (!DelegationID.empty()) out << s << JSONSimpleArray("delegationId", DelegationID);
    }

    out << std::endl;
    out << "}";
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
    
    URL src;
    URL logsrc;
    URL dst(destination);
    if (!jc->GetURLToJobResource(*this, STAGEOUTDIR, src)) {
      logger.msg(ERROR, "Can't retrieve job files for job (%s) - unable to determine URL of stage out directory", JobID);
      return false;
    }
    if (!LogDir.empty()) {
      // retrieve log files only if corresponding in job description is specified
      if (!jc->GetURLToJobResource(*this, LOGDIR, logsrc)) {
        logger.msg(ERROR, "Can't retrieve job files for job (%s) - unable to determine URL of log directory", JobID);
        return false;
      }
    }

    if (!src) {
      logger.msg(ERROR, "Invalid stage out path specified (%s)", src.fullstr());
      return false;
    }
    // logsrc is optional

    // TODO: can destination be remote?

    if (!force && Glib::file_test(dst.Path(), Glib::FILE_TEST_EXISTS)) {
      logger.msg(WARNING, "%s directory exist! Skipping job.", dst.Path());
      return false;
    }

    const std::string srcpath = src.Path() + (src.Path().empty() || *src.Path().rbegin() != '/' ? "/" : "");
    const std::string dstpath = dst.Path() + (dst.Path().empty() || *dst.Path().rbegin() != G_DIR_SEPARATOR ? G_DIR_SEPARATOR_S : "");

    std::list<URLLocation> files;
    if(src.Locations().empty()) {
      std::list<std::string> paths;
      if (!ListFilesRecursive(uc, src, paths)) {
        logger.msg(ERROR, "Unable to retrieve list of job files to download for job %s", JobID);
        return false;
      }
      for(std::list<std::string>::iterator it = paths.begin(); it != paths.end(); ++it) {
        URLLocation file(src, *it);
        file.ChangePath(srcpath + *it);
        files.push_back(file);
      }
    } else {
      std::list<URLLocation> const & locations = src.Locations();
      files.insert(files.end(), locations.begin(), locations.end());
    }
    if (logsrc) {
      const std::string logsrcpath = logsrc.Path() + (logsrc.Path().empty() || *logsrc.Path().rbegin() != '/' ? "/" : "");
      if(logsrc.Locations().empty()) {
        std::list<std::string> paths;
        if (!ListFilesRecursive(uc, logsrc, paths)) {
          logger.msg(ERROR, "Unable to retrieve list of log files to download for job %s", JobID);
          return false;
        }
        for(std::list<std::string>::iterator it = paths.begin(); it != paths.end(); ++it) {
	  URLLocation file(logsrc, LogDir+"/"+*it);
          file.ChangePath(logsrcpath + *it);
          files.push_back(file);
        }
      } else {
        std::list<URLLocation> const & locations = logsrc.Locations();
        for(std::list<URLLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
          URLLocation location(*it, LogDir+"/"+it->Name());
          files.insert(files.end(), location);
        }
      }
    }

    if (files.empty()) {
      logger.msg(WARNING, "No files to retrieve for job %s", JobID);
      return true;
    }

    // We must make it sure it is directory and it exists 
    if (!DirCreate(dst.Path(), S_IRWXU, true)) {
      logger.msg(WARNING, "Failed to create directory %s! Skipping job.", dst.Path());
      return false;
    }

    bool ok = true;
    std::set<std::string> processed;
    for (std::list<URLLocation>::const_iterator it = files.begin(); it != files.end(); ++it) {
      src = *it;
      dst.ChangePath(dstpath + it->Name());
      if(!processed.insert(it->Name()).second) continue; // skip already processed input files
      if (Glib::file_test(dst.Path(), Glib::FILE_TEST_EXISTS)) {
        if (!force) {
          logger.msg(ERROR, "Failed downloading %s to %s, destination already exist", src.str(), dst.Path());
          ok = false;
          continue;
        }

        if (!FileDelete(dst.Path())) {
          logger.msg(ERROR, "Failed downloading %s to %s, unable to remove existing destination", src.str(), dst.Path());
          ok = false;
          continue;
        }
      }
      if (!CopyJobFile(uc, src, dst)) {
        logger.msg(INFO, "Failed downloading %s to %s", src.str(), dst.Path());
        ok = false;
      }
    }

    return ok;
  }

  bool Job::ListFilesRecursive(const UserConfig& uc, const URL& dir, std::list<std::string>& files, const std::string& prefix) {
    std::list<FileInfo> outputfiles;

    DataHandle handle(dir, uc);
    if (!handle) {
      logger.msg(INFO, "Unable to initialize handler for %s", dir.str());
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
    mover.retry(false);
    mover.retry_retryable(true);
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
      // Missing file is not fatal error because dir listing or plugin query may provide incorrect inforamtion.
      bool fatal = (res.GetErrno() != ENOENT);
      logger.msg(fatal?ERROR:WARNING, "File download failed: %s", std::string(res));
      // Reset connection because one can't be sure how failure
      // affects server and/or connection state.
      // TODO: Investigate/define DMC behavior in such case.
      delete data_source;
      data_source = NULL;
      delete data_destination;
      data_destination = NULL;
      return fatal?false:true;
    }

    return true;
  }

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
