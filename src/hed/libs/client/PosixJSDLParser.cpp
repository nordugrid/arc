// -*- indent-tabs-mode: nil -*-

#include <string>
#include <time.h>
#include <arc/XMLNode.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/client/JobInnerRepresentation.h>
#include <arc/Logger.h>

#include "PosixJSDLParser.h"


namespace Arc {

  bool PosixJSDLParser::parse(Arc::JobInnerRepresentation& innerRepresentation, const std::string source) {
    //Source checking
    Arc::XMLNode node(source);
    if (node.Size() == 0) {
      logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML structure! ");
      return false;
    }
    if (!Element_Search(node, true))
      return false;

    // The source parsing start now.
    Arc::NS nsList;
    nsList.insert(std::pair<std::string, std::string>("jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-posix", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-arc", "http://www.nordugrid.org/ws/schemas/jsdl-arc"));

    node.Namespaces(nsList);

    Arc::XMLNode jobdescription = node["JobDescription"];

    if (bool(jobdescription["LocalLogging"])) {
      if (bool(jobdescription["LocalLogging"]["Directory"]))
        innerRepresentation.LogDir = (std::string)jobdescription["LocalLogging"]["Directory"];
      else {
        logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML: \"LocalLogging.Directory\" element missed!");
        return false;
      }
      if (bool(jobdescription["LocalLogging"][1])) {
        logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML: too many \"LocalLogging\" elements!");
        return false;
      }
    }

    if (bool(jobdescription["RemoteLogging"]["URL"])) {
      URL url((std::string)jobdescription["RemoteLogging"]["URL"]);
      innerRepresentation.RemoteLogging = url;
    }

    if (bool(jobdescription["CredentialServer"]["URL"])) {
      URL url((std::string)jobdescription["CredentialServer"]["URL"]);
      innerRepresentation.CredentialService = url;
    }

    if (bool(jobdescription["ProcessingStartTime"])) {
      Time stime((std::string)jobdescription["ProcessingStartTime"]);
      innerRepresentation.ProcessingStartTime = stime;
    }

    if (bool(jobdescription["Reruns"]))
      innerRepresentation.LRMSReRun = stringtoi((std::string)jobdescription["Reruns"]);

    if (bool(jobdescription["AccessControl"]["Content"])) {
      Arc::XMLNode accesscontrol(jobdescription["AccessControl"]["Content"]);
      accesscontrol.Child(0).New(innerRepresentation.AccessControl);
    }

    if (bool(jobdescription["Notify"]))
      for (int i = 0; bool(jobdescription["Notify"][i]); i++) {
        Arc::NotificationType notify;
        for (int j = 0; bool(jobdescription["Notify"][i]["Endpoint"][j]); j++)
          notify.Address.push_back((std::string)jobdescription["Notify"][i]["Endpoint"][j]);

        for (int j = 0; bool(jobdescription["Notify"][i]["State"][j]); j++)
          notify.State.push_back((std::string)jobdescription["Notify"][i]["State"][j]);
        innerRepresentation.Notification.push_back(notify);
      }

    // JobIdentification
    Arc::XMLNode jobidentification = node["JobDescription"]["JobIdentification"];

    if (bool(jobidentification["JobName"]))
      innerRepresentation.JobName = (std::string)jobidentification["JobName"];

    for (int i = 0; (bool)(jobidentification["OldJobID"][i]); i++)
      innerRepresentation.OldJobIDs.push_back(URL((std::string)jobidentification["OldJobID"][i]));

    if (bool(jobidentification["JobProject"]))
      innerRepresentation.JobProject = (std::string)jobidentification["JobProject"];
    // end of JobIdentificatio

    // Application
    Arc::XMLNode application = node["JobDescription"]["Application"]["POSIXApplication"];

    if (bool(application["Executable"]))
      innerRepresentation.Executable = (std::string)application["Executable"];

    for (int i = 0; (bool)(application["Argument"][i]); i++) {
      std::string value = (std::string)application["Argument"][i];
      innerRepresentation.Argument.push_back(value);
    }

    if (bool(application["Input"]))
      innerRepresentation.Input = (std::string)application["Input"];

    if (bool(application["Output"]))
      innerRepresentation.Output = (std::string)application["Output"];

    if (bool(application["Error"]))
      innerRepresentation.Error = (std::string)application["Error"];

    for (int i = 0; (bool)(application["Environment"][i]); i++) {
      Arc::XMLNode env = application["Environment"][i];
      Arc::XMLNode name = env.Attribute("name");
      if (!name) {
        logger.msg(DEBUG, "[PosixJSDLParser] Error during the parsing: missed the name attributes of the \"%s\" Environment", (std::string)env);
        return false;
      }
      Arc::EnvironmentType env_tmp;
      env_tmp.name_attribute = (std::string)name;
      env_tmp.value = (std::string)env;
      innerRepresentation.Environment.push_back(env_tmp);
    }

    if (bool(application["VirtualMemoryLimit"]))
      if (innerRepresentation.IndividualVirtualMemory < Arc::stringto<int>((std::string)application["VirtualMemoryLimit"]))
        innerRepresentation.IndividualVirtualMemory = Arc::stringto<int>((std::string)application["VirtualMemoryLimit"]);

    if (bool(application["CPUTimeLimit"])) {
      Period time((std::string)application["CPUTimeLimit"]);
      if (innerRepresentation.TotalCPUTime < time)
        innerRepresentation.TotalCPUTime = time;
    }

    if (bool(application["WallTimeLimit"])) {
      Period time((std::string)application["WallTimeLimit"]);
      innerRepresentation.TotalWallTime = time;
    }

    if (bool(application["MemoryLimit"]))
      if (innerRepresentation.IndividualPhysicalMemory < Arc::stringto<int>((std::string)application["MemoryLimit"]))
        innerRepresentation.IndividualPhysicalMemory = Arc::stringto<int>((std::string)application["MemoryLimit"]);

    if (bool(application["ProcessCountLimit"])) {
      innerRepresentation.Slots = stringtoi((std::string)application["ProcessCountLimit"]);
      innerRepresentation.NumberOfProcesses = stringtoi((std::string)application["ProcessCountLimit"]);
    }

    if (bool(application["ThreadCountLimit"]))
      innerRepresentation.ThreadPerProcesses = stringtoi((std::string)application["ThreadCountLimit"]);
    // end of Application

    // Resources
    Arc::XMLNode resource = node["JobDescription"]["Resources"];

    if (bool(resource["SessionLifeTime"])) {
      Period time((std::string)resource["SessionLifeTime"]);
      innerRepresentation.SessionLifeTime = time;
    }

    if (bool(resource["TotalCPUTime"])) {
      long value = get_limit(resource["TotalCPUTime"]);
      if (value != -1) {
        Period time((time_t)value);
        if (innerRepresentation.TotalCPUTime < time)
          innerRepresentation.TotalCPUTime = time;
      }
    }

    if (bool(resource["IndividualCPUTime"])) {
      long value = get_limit(resource["IndividualCPUTime"]);
      if (value != -1) {
        Period time((time_t)value);
        innerRepresentation.IndividualCPUTime = time;
      }
    }

    if (bool(resource["IndividualPhysicalMemory"])) {
      long value = get_limit(resource["IndividualPhysicalMemory"]);
      if (value != -1)
        if (innerRepresentation.IndividualPhysicalMemory < value)
          innerRepresentation.IndividualPhysicalMemory = value;
    }

    if (bool(resource["IndividualVirtualMemory"])) {
      long value = get_limit(resource["IndividualVirtualMemory"]);
      if (value != -1)
        if (innerRepresentation.IndividualVirtualMemory < value)
          innerRepresentation.IndividualVirtualMemory = value;
    }

    if (bool(resource["IndividualDiskSpace"])) {
      long value = get_limit(resource["IndividualDiskSpace"]);
      if (value != -1)
        innerRepresentation.IndividualDiskSpace = value;
    }

    if (bool(resource["FileSystem"]["DiskSpace"])) {
      long value = get_limit(resource["FileSystem"]["DiskSpace"]);
      if (value != -1)
        innerRepresentation.DiskSpace = value;
    }

    if (bool(resource["GridTimeLimit"])) {
      ReferenceTimeType rt;
      rt.benchmark = "gridtime";
      rt.value = 2800;
      rt.time = (std::string)resource["GridTimeLimit"];
      innerRepresentation.ReferenceTime.push_back(rt);
    }

    if (bool(resource["ExclusiveExecution"])) {
      if (lower((std::string)resource["ExclusiveExecution"]) == "true")
        innerRepresentation.ExclusiveExecution = true;
      else
        innerRepresentation.ExclusiveExecution = false;
    }

    if (bool(resource["IndividualNetworkBandwidth"])) {
      long bits_per_sec = get_limit(resource["IndividualNetworkBandwidth"]);
      std::string value = "";
      long network = 1024 * 1024;
      if (bits_per_sec < 100 * network)
        value = "100megabitethernet";
      else if (bits_per_sec < 1024 * network)
        value = "gigabitethernet";
      else if (bits_per_sec < 10 * 1024 * network)
        value = "myrinet";
      else
        value = "infiniband";
      innerRepresentation.NetworkInfo = value;
    }

    if (bool(resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]))
      innerRepresentation.OSName = (std::string)resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"];

    if (bool(resource["OperatingSystem"]["OperatingSystemVersion"]))
      innerRepresentation.OSVersion = (std::string)resource["OperatingSystem"]["OperatingSystemVersion"];

    if (bool(resource["CPUArchitecture"]["CPUArchitectureName"]))
      innerRepresentation.Platform = (std::string)resource["CPUArchitecture"]["CPUArchitectureName"];

    if (bool(resource["CandidateTarget"]["HostName"])) {
      URL host_url((std::string)resource["CandidateTarget"]["HostName"]);
      innerRepresentation.EndPointURL = host_url;
    }

    if (bool(resource["CandidateTarget"]["QueueName"]))
      innerRepresentation.QueueName = (std::string)resource["CandidateTarget"]["QueueName"];

    if (bool(resource["Middleware"]["Name"]))
      innerRepresentation.CEType = (std::string)resource["Middleware"]["Name"];

    if (bool(resource["TotalCPUCount"]))
      innerRepresentation.ProcessPerHost = (int)get_limit(resource["TotalCPUCount"]);

    if (bool(resource["RunTimeEnvironment"]))
      for (int i = 0; (bool)(resource["RunTimeEnvironment"][i]); i++) {
        Arc::RunTimeEnvironmentType rt;
        std::string version;
        rt.Name = trim((std::string)resource["RunTimeEnvironment"][i]["Name"]);
        version = trim((std::string)resource["RunTimeEnvironment"][i]["Version"]["Exact"]);
        rt.Version.push_back(version);
        innerRepresentation.RunTimeEnvironment.push_back(rt);
      }
    // end of Resources

    // Datastaging
    Arc::XMLNode datastaging = node["JobDescription"]["DataStaging"];

    for (int i = 0; datastaging[i]; i++) {
      Arc::XMLNode ds = datastaging[i];
      Arc::XMLNode source_uri = ds["Source"]["URI"];
      Arc::XMLNode target_uri = ds["Target"]["URI"];
      Arc::XMLNode filenameNode = ds["FileName"];

      Arc::FileType file;

      if (filenameNode) {
        file.Name = (std::string)filenameNode;
        if (bool(source_uri)) {
          Arc::SourceType source;
          source.URI = (std::string)source_uri;
          source.Threads = -1;
          file.Source.push_back(source);
        }
        if (bool(target_uri)) {
          Arc::TargetType target;
          target.URI = (std::string)target_uri;
          target.Threads = -1;
          file.Target.push_back(target);
        }

        if (lower(((std::string)ds["IsExecutable"])) == "true")
          file.IsExecutable = true;
        else
          file.IsExecutable = false;
        if (lower(((std::string)ds["DeleteOnTermination"])) == "true")
          file.KeepData = false;
        else
          file.KeepData = true;
        file.DownloadToCache = false;
        innerRepresentation.File.push_back(file);
      }
    }
    // end of Datastaging
    return true;
  }

  bool PosixJSDLParser::getProduct(const Arc::JobInnerRepresentation& innerRepresentation, std::string& product) const {
    Arc::NS nsList;
    nsList.insert(std::pair<std::string, std::string>("jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-posix", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-arc", "http://www.nordugrid.org/ws/schemas/jsdl-arc"));


    Arc::XMLNode jobdefinition("<JobDefinition/>");

    jobdefinition.Namespaces(nsList);

    Arc::XMLNode jobdescription = jobdefinition.NewChild("JobDescription");

    // JobIdentification
    if (!innerRepresentation.JobName.empty()) {
      if (!bool(jobdescription["JobIdentification"]))
        jobdescription.NewChild("JobIdentification");
      jobdescription["JobIdentification"].NewChild("JobName") = innerRepresentation.JobName;
    }

    if (innerRepresentation.OldJobIDs.size() != 0) {
      if (!bool(jobdescription["JobIdentification"]))
        jobdescription.NewChild("JobIdentification");
      for (std::list<URL>::const_iterator it = innerRepresentation.OldJobIDs.begin();
           it != innerRepresentation.OldJobIDs.end(); it++)
        jobdescription["JobIdentification"].NewChild("jsdl-arc:OldJobID") = it->str();
    }
    if (!innerRepresentation.JobProject.empty()) {
      if (!bool(jobdescription["JobIdentification"]))
        jobdescription.NewChild("JobIdentification");
      jobdescription["JobIdentification"].NewChild("JobProject") = innerRepresentation.JobProject;
    }
    // end of JobIdentification

    // Application
    if (!innerRepresentation.Executable.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Executable") = innerRepresentation.Executable;
    }
    if (!innerRepresentation.Argument.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      for (std::list<std::string>::const_iterator it = innerRepresentation.Argument.begin();
           it != innerRepresentation.Argument.end(); it++)
        jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Argument") = *it;
    }
    if (!innerRepresentation.Input.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Input") = innerRepresentation.Input;
    }
    if (!innerRepresentation.Output.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Output") = innerRepresentation.Output;
    }
    if (!innerRepresentation.Error.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Error") = innerRepresentation.Error;
    }
    for (std::list<Arc::EnvironmentType>::const_iterator it = innerRepresentation.Environment.begin();
         it != innerRepresentation.Environment.end(); it++) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      Arc::XMLNode environment = jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Environment");
      environment = (*it).value;
      environment.NewAttribute("name") = (*it).name_attribute;
    }
    if (innerRepresentation.TotalWallTime != -1) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      Period time((std::string)innerRepresentation.TotalWallTime);
      std::ostringstream oss;
      oss << time.GetPeriod();
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:WallTimeLimit") = oss.str();
    }
    if (innerRepresentation.NumberOfProcesses > -1 &&
        innerRepresentation.NumberOfProcesses >= innerRepresentation.Slots) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      if (!bool(jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"]))
        jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ProcessCountLimit");
      std::ostringstream oss;
      oss << innerRepresentation.NumberOfProcesses;
      jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] = oss.str();
    }
    if (innerRepresentation.Slots > -1 &&
        innerRepresentation.Slots >= innerRepresentation.NumberOfProcesses) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      if (!bool(jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"]))
        jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ProcessCountLimit");
      std::ostringstream oss;
      oss << innerRepresentation.Slots;
      jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] = oss.str();
    }
    if (innerRepresentation.ThreadPerProcesses > -1) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      std::ostringstream oss;
      oss << innerRepresentation.ThreadPerProcesses;
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ThreadCountLimit") = oss.str();
    }
    // end of Application

    // DataStaging
    for (std::list<Arc::FileType>::const_iterator it = innerRepresentation.File.begin();
         it != innerRepresentation.File.end(); it++) {
      Arc::XMLNode datastaging = jobdescription.NewChild("DataStaging");
      if (!(*it).Name.empty())
        datastaging.NewChild("FileName") = (*it).Name;
      if ((*it).Source.size() != 0) {
        std::list<Arc::SourceType>::const_iterator it2;
        it2 = ((*it).Source).begin();
        if (trim(((*it2).URI).fullstr()) != "")
          datastaging.NewChild("Source").NewChild("URI") = ((*it2).URI).fullstr();
      }
      if ((*it).Target.size() != 0) {
        std::list<Arc::TargetType>::const_iterator it3;
        it3 = ((*it).Target).begin();
        if (trim(((*it3).URI).fullstr()) != "")
          datastaging.NewChild("Target").NewChild("URI") = ((*it3).URI).fullstr();
      }
      if ((*it).IsExecutable || (*it).Name == innerRepresentation.Executable)
        datastaging.NewChild("jsdl-arc:IsExecutable") = "true";
      if ((*it).KeepData)
        datastaging.NewChild("DeleteOnTermination") = "false";
      else
        datastaging.NewChild("DeleteOnTermination") = "true";
    }

    // Resources
    for (std::list<Arc::RunTimeEnvironmentType>::const_iterator it = innerRepresentation.RunTimeEnvironment.begin();
         it != innerRepresentation.RunTimeEnvironment.end(); it++) {
      Arc::XMLNode resources;
      if (!bool(jobdescription["Resources"]))
        resources = jobdescription.NewChild("Resources");
      else
        resources = jobdescription["Resources"];
      // When the version is not parsing
      if (!(*it).Name.empty() && (*it).Version.empty())
        resources.NewChild("jsdl-arc:RunTimeEnvironment").NewChild("jsdl-arc:Name") = (*it).Name;

      // When more then one version are parsing
      for (std::list<std::string>::const_iterator it_version = (*it).Version.begin();
           it_version != (*it).Version.end(); it_version++) {
        Arc::XMLNode RTE = resources.NewChild("jsdl-arc:RunTimeEnvironment");
        RTE.NewChild("jsdl-arc:Name") = (*it).Name;
        Arc::XMLNode version = RTE.NewChild("jsdl-arc:Version").NewChild("jsdl-arc:Exact");
        version.NewAttribute("epsilon") = "0.5";
        version = *it_version;
      }
    }

    if (!innerRepresentation.CEType.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["Middleware"]))
        jobdescription["Resources"].NewChild("jsdl-arc:Middleware");
      jobdescription["Resources"]["Middleware"].NewChild("jsdl-arc:Name") = innerRepresentation.CEType;
      //jobdescription["Resources"]["Middleware"].NewChild("Version") = ?;
    }

    if (innerRepresentation.SessionLifeTime != -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["SessionLifeTime"]))
        jobdescription["Resources"].NewChild("jsdl-arc:SessionLifeTime");
      std::string outputValue;
      std::stringstream ss;
      ss << innerRepresentation.SessionLifeTime.GetPeriod();
      ss >> outputValue;
      jobdescription["Resources"]["SessionLifeTime"] = outputValue;
    }

    if (!innerRepresentation.ReferenceTime.empty()) {
      // TODO: what is the mapping
    }

    if (bool(innerRepresentation.EndPointURL)) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]))
        jobdescription["Resources"].NewChild("jsdl-arc:CandidateTarget");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]["HostName"]))
        jobdescription["Resources"]["CandidateTarget"].NewChild("jsdl-arc:HostName");
      jobdescription["Resources"]["CandidateTarget"]["HostName"] = innerRepresentation.EndPointURL.str();
    }

    if (!innerRepresentation.QueueName.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]))
        jobdescription["Resources"].NewChild("jsdl-arc:CandidateTarget");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]["QueueName"]))
        jobdescription["Resources"]["CandidateTarget"].NewChild("jsdl-arc:QueueName");
      jobdescription["Resources"]["CandidateTarget"]["QueueName"] = innerRepresentation.QueueName;
    }

    if (!innerRepresentation.Platform.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["CPUArchitecture"]))
        jobdescription["Resources"].NewChild("CPUArchitecture");
      if (!bool(jobdescription["Resources"]["CPUArchitecture"]["CPUArchitectureName"]))
        jobdescription["Resources"]["CPUArchitecture"].NewChild("CPUArchitectureName");
      jobdescription["Resources"]["CPUArchitecture"]["CPUArchitectureName"] = innerRepresentation.Platform;
    }

    if (!innerRepresentation.OSName.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]))
        jobdescription["Resources"].NewChild("OperatingSystem");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]))
        jobdescription["Resources"]["OperatingSystem"].NewChild("OperatingSystemType");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]))
        jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"].NewChild("OperatingSystemName");
      jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"] = innerRepresentation.OSName;
    }

    if (!innerRepresentation.OSVersion.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]))
        jobdescription["Resources"].NewChild("OperatingSystem");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]["OperatingSystemVersion"]))
        jobdescription["Resources"]["OperatingSystem"].NewChild("OperatingSystemVersion");
      jobdescription["Resources"]["OperatingSystem"]["OperatingSystemVersion"] = innerRepresentation.OSVersion;
    }

    if (innerRepresentation.ProcessPerHost > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["TotalCPUCount"]))
        jobdescription["Resources"].NewChild("TotalCPUCount");
      if (!bool(jobdescription["Resources"]["TotalCPUCount"]["LowerBoundRange"]))
        jobdescription["Resources"]["TotalCPUCount"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << innerRepresentation.ProcessPerHost;
      jobdescription["Resources"]["TotalCPUCount"]["LowerBoundRange"] = oss.str();
    }

    if (innerRepresentation.IndividualPhysicalMemory > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualPhysicalMemory"]))
        jobdescription["Resources"].NewChild("IndividualPhysicalMemory");
      if (!bool(jobdescription["Resources"]["IndividualPhysicalMemory"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualPhysicalMemory"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << innerRepresentation.IndividualPhysicalMemory;
      jobdescription["Resources"]["IndividualPhysicalMemory"]["LowerBoundRange"] = oss.str();
    }

    if (innerRepresentation.IndividualVirtualMemory > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualVirtualMemory"]))
        jobdescription["Resources"].NewChild("IndividualVirtualMemory");
      if (!bool(jobdescription["Resources"]["IndividualVirtualMemory"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualVirtualMemory"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << innerRepresentation.IndividualVirtualMemory;
      jobdescription["Resources"]["IndividualVirtualMemory"]["LowerBoundRange"] = oss.str();
    }

    if (innerRepresentation.IndividualDiskSpace > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualDiskSpace"]))
        jobdescription["Resources"].NewChild("IndividualDiskSpace");
      if (!bool(jobdescription["Resources"]["IndividualDiskSpace"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualDiskSpace"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << innerRepresentation.IndividualDiskSpace;
      jobdescription["Resources"]["IndividualDiskSpace"]["LowerBoundRange"] = oss.str();
    }

    if (innerRepresentation.TotalCPUTime != -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["TotalCPUTime"]))
        jobdescription["Resources"].NewChild("TotalCPUTime");
      if (!bool(jobdescription["Resources"]["TotalCPUTime"]["LowerBoundRange"]))
        jobdescription["Resources"]["TotalCPUTime"].NewChild("LowerBoundRange");
      Period time((std::string)innerRepresentation.TotalCPUTime);
      std::ostringstream oss;
      oss << time.GetPeriod();
      jobdescription["Resources"]["TotalCPUTime"]["LowerBoundRange"] = oss.str();
    }

    if (innerRepresentation.IndividualCPUTime != -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualCPUTime"]))
        jobdescription["Resources"].NewChild("IndividualCPUTime");
      if (!bool(jobdescription["Resources"]["IndividualCPUTime"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualCPUTime"].NewChild("LowerBoundRange");
      Period time((std::string)innerRepresentation.IndividualCPUTime);
      std::ostringstream oss;
      oss << time.GetPeriod();
      jobdescription["Resources"]["IndividualCPUTime"]["LowerBoundRange"] = oss.str();
    }

    if (innerRepresentation.DiskSpace > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["FileSystem"]))
        jobdescription["Resources"].NewChild("FileSystem");
      if (!bool(jobdescription["Resources"]["FileSystem"]["DiskSpace"]))
        jobdescription["Resources"]["FileSystem"].NewChild("DiskSpace");
      if (!bool(jobdescription["Resources"]["FileSystem"]["DiskSpace"]["LowerBoundRange"]))
        jobdescription["Resources"]["FileSystem"]["DiskSpace"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << innerRepresentation.DiskSpace;
      jobdescription["Resources"]["FileSystem"]["DiskSpace"]["LowerBoundRange"] = oss.str();
    }

    if (innerRepresentation.ExclusiveExecution) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["ExclusiveExecution"]))
        jobdescription["Resources"].NewChild("ExclusiveExecution") = "true";
    }

    if (!innerRepresentation.NetworkInfo.empty()) {
      std::string value = "";
      if (innerRepresentation.NetworkInfo == "100megabitethernet")
        value = "104857600.0";
      else if (innerRepresentation.NetworkInfo == "gigabitethernet")
        value = "1073741824.0";
      else if (innerRepresentation.NetworkInfo == "myrinet")
        value = "2147483648.0";
      else if (innerRepresentation.NetworkInfo == "infiniband")
        value = "10737418240.0";
      else
        value = "";

      if (value != "") {
        if (!bool(jobdescription["Resources"]))
          jobdescription.NewChild("Resources");
        if (!bool(jobdescription["Resources"]["IndividualNetworkBandwidth"]))
          jobdescription["Resources"].NewChild("IndividualNetworkBandwidth");
        if (!bool(jobdescription["Resources"]["IndividualNetworkBandwidth"]["LowerBoundRange"]))
          jobdescription["Resources"]["IndividualNetworkBandwidth"].NewChild("LowerBoundRange");
        jobdescription["Resources"]["IndividualNetworkBandwidth"]["LowerBoundRange"] = value;
      }
    }
    // end of Resources


    // AccessControl
    if (bool(innerRepresentation.AccessControl)) {
      if (!bool(jobdescription["AccessControl"]))
        jobdescription.NewChild("jsdl-arc:AccessControl");
      if (!bool(jobdescription["AccessControl"]["Type"]))
        jobdescription["AccessControl"].NewChild("jsdl-arc:Type") = "GACL";
      jobdescription["AccessControl"].NewChild("jsdl-arc:Content").NewChild(innerRepresentation.AccessControl);
    }

    // Notify
    if (!innerRepresentation.Notification.empty())
      for (std::list<Arc::NotificationType>::const_iterator it = innerRepresentation.Notification.begin();
           it != innerRepresentation.Notification.end(); it++) {
        Arc::XMLNode notify = jobdescription.NewChild("jsdl-arc:Notify");
        notify.NewChild("jsdl-arc:Type") = "Email";
        for (std::list<std::string>::const_iterator it_address = (*it).Address.begin();
             it_address != (*it).Address.end(); it_address++)
          if (*it_address != "")
            notify.NewChild("jsdl-arc:Endpoint") = *it_address;
        for (std::list<std::string>::const_iterator it_state = (*it).State.begin();
             it_state != (*it).State.end(); it_state++)
          if (*it_state != "")
            notify.NewChild("jsdl-arc:State") = *it_state;
      }

    // ProcessingStartTime
    if (innerRepresentation.ProcessingStartTime != -1) {
      if (!bool(jobdescription["ProcessingStartTime"]))
        jobdescription.NewChild("jsdl-arc:ProcessingStartTime");
      jobdescription["ProcessingStartTime"] = innerRepresentation.ProcessingStartTime.str();
    }

    // Reruns
    if (innerRepresentation.LRMSReRun != -1) {
      if (!bool(jobdescription["Reruns"]))
        jobdescription.NewChild("jsdl-arc:Reruns");
      std::ostringstream oss;
      oss << innerRepresentation.LRMSReRun;
      jobdescription["Reruns"] = oss.str();
    }

    // LocalLogging.Directory
    if (!innerRepresentation.LogDir.empty()) {
      if (!bool(jobdescription["LocalLogging"]))
        jobdescription.NewChild("jsdl-arc:LocalLogging");
      if (!bool(jobdescription["LocalLogging"]["Directory"]))
        jobdescription["LocalLogging"].NewChild("jsdl-arc:Directory");
      jobdescription["LocalLogging"]["Directory"] = innerRepresentation.LogDir;
    }

    // CredentialServer.URL
    if (bool(innerRepresentation.CredentialService)) {
      if (!bool(jobdescription["CredentialServer"]))
        jobdescription.NewChild("jsdl-arc:CredentialServer");
      if (!bool(jobdescription["CredentialServer"]["URL"]))
        jobdescription["CredentialServer"].NewChild("jsdl-arc:URL");
      jobdescription["CredentialServer"]["URL"] = innerRepresentation.CredentialService.fullstr();
    }

    // RemoteLogging.URL
    if (bool(innerRepresentation.RemoteLogging)) {
      if (!bool(jobdescription["RemoteLogging"]))
        jobdescription.NewChild("jsdl-arc:RemoteLogging");
      if (!bool(jobdescription["RemoteLogging"]["URL"]))
        jobdescription["RemoteLogging"].NewChild("jsdl-arc:URL");
      jobdescription["RemoteLogging"]["URL"] = innerRepresentation.RemoteLogging.fullstr();
    }

    jobdefinition.GetDoc(product, true);

    return true;
  }

} // namespace Arc
