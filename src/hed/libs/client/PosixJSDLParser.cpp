// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/JobDescription.h>

#include "PosixJSDLParser.h"

namespace Arc {

  PosixJSDLParser::PosixJSDLParser()
    : JobDescriptionParser() {}

  PosixJSDLParser::~PosixJSDLParser() {}

  static long get_limit(XMLNode range) {
    if (!range)
      return -1;
    XMLNode n = range["LowerBoundRange"];
    if (n)
      return stringtol((std::string)n);
    n = range["UpperBoundRange"];
    if (n)
      return stringtol((std::string)n);
    return -1;
  }

  JobDescription PosixJSDLParser::Parse(const std::string& source) const {
    JobDescription job;

    XMLNode node(source);
    if (node.Size() == 0) {
      logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML structure! ");
      return JobDescription();
    }

    // The source parsing start now.
    NS nsList;
    nsList.insert(std::pair<std::string, std::string>("jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-posix", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-arc", "http://www.nordugrid.org/ws/schemas/jsdl-arc"));

    node.Namespaces(nsList);

    XMLNode jobdescription = node["JobDescription"];

    if (bool(jobdescription["LocalLogging"])) {
      if (bool(jobdescription["LocalLogging"]["Directory"]))
        job.LogDir = (std::string)jobdescription["LocalLogging"]["Directory"];
      else {
        logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML: \"LocalLogging.Directory\" element missed!");
        return JobDescription();
      }
      if (bool(jobdescription["LocalLogging"][1])) {
        logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML: too many \"LocalLogging\" elements!");
        return JobDescription();
      }
    }

    if (bool(jobdescription["RemoteLogging"]["URL"])) {
      URL url((std::string)jobdescription["RemoteLogging"]["URL"]);
      job.RemoteLogging = url;
    }

    if (bool(jobdescription["CredentialServer"]["URL"])) {
      URL url((std::string)jobdescription["CredentialServer"]["URL"]);
      job.CredentialService = url;
    }

    if (bool(jobdescription["ProcessingStartTime"])) {
      Time stime((std::string)jobdescription["ProcessingStartTime"]);
      job.ProcessingStartTime = stime;
    }

    if (bool(jobdescription["Reruns"]))
      job.LRMSReRun = stringtoi((std::string)jobdescription["Reruns"]);

    if (bool(jobdescription["AccessControl"]["Content"])) {
      XMLNode accesscontrol(jobdescription["AccessControl"]["Content"]);
      accesscontrol.Child(0).New(job.AccessControl);
    }

    if (bool(jobdescription["Notify"]))
      for (int i = 0; bool(jobdescription["Notify"][i]); i++) {
        NotificationType notify;
        for (int j = 0; bool(jobdescription["Notify"][i]["Endpoint"][j]); j++)
          notify.Address.push_back((std::string)jobdescription["Notify"][i]["Endpoint"][j]);

        for (int j = 0; bool(jobdescription["Notify"][i]["State"][j]); j++)
          notify.State.push_back((std::string)jobdescription["Notify"][i]["State"][j]);
        job.Notification.push_back(notify);
      }

    // JobIdentification
    XMLNode jobidentification = node["JobDescription"]["JobIdentification"];

    if (bool(jobidentification["JobName"]))
      job.JobName = (std::string)jobidentification["JobName"];

    for (int i = 0; (bool)(jobidentification["OldJobID"][i]); i++)
      job.OldJobIDs.push_back(URL((std::string)jobidentification["OldJobID"][i]));

    if (bool(jobidentification["JobProject"]))
      job.JobProject = (std::string)jobidentification["JobProject"];
    // end of JobIdentificatio

    // Application
    XMLNode application = node["JobDescription"]["Application"]["POSIXApplication"];

    if (bool(application["Executable"]))
      job.Executable = (std::string)application["Executable"];

    for (int i = 0; (bool)(application["Argument"][i]); i++) {
      std::string value = (std::string)application["Argument"][i];
      job.Argument.push_back(value);
    }

    if (bool(application["Input"]))
      job.Input = (std::string)application["Input"];

    if (bool(application["Output"]))
      job.Output = (std::string)application["Output"];

    if (bool(application["Error"]))
      job.Error = (std::string)application["Error"];

    for (int i = 0; (bool)(application["Environment"][i]); i++) {
      XMLNode env = application["Environment"][i];
      XMLNode name = env.Attribute("name");
      if (!name) {
        logger.msg(DEBUG, "[PosixJSDLParser] Error during the parsing: missed the name attributes of the \"%s\" Environment", (std::string)env);
        return JobDescription();
      }
      EnvironmentType env_tmp;
      env_tmp.name_attribute = (std::string)name;
      env_tmp.value = (std::string)env;
      job.Environment.push_back(env_tmp);
    }

    if (bool(application["VirtualMemoryLimit"]))
      if (job.IndividualVirtualMemory < stringto<int>((std::string)application["VirtualMemoryLimit"]))
        job.IndividualVirtualMemory = stringto<int>((std::string)application["VirtualMemoryLimit"]);

    if (bool(application["CPUTimeLimit"])) {
      Period time((std::string)application["CPUTimeLimit"]);
      if (job.TotalCPUTime < time)
        job.TotalCPUTime = time;
    }

    if (bool(application["WallTimeLimit"])) {
      Period time((std::string)application["WallTimeLimit"]);
      job.TotalWallTime = time;
    }

    if (bool(application["MemoryLimit"]))
      if (job.IndividualPhysicalMemory < stringto<int>((std::string)application["MemoryLimit"]))
        job.IndividualPhysicalMemory = stringto<int>((std::string)application["MemoryLimit"]);

    if (bool(application["ProcessCountLimit"])) {
      job.Slots = stringtoi((std::string)application["ProcessCountLimit"]);
      job.NumberOfProcesses = stringtoi((std::string)application["ProcessCountLimit"]);
    }

    if (bool(application["ThreadCountLimit"]))
      job.ThreadPerProcesses = stringtoi((std::string)application["ThreadCountLimit"]);
    // end of Application

    // Resources
    XMLNode resource = node["JobDescription"]["Resources"];

    if (bool(resource["SessionLifeTime"])) {
      Period time((std::string)resource["SessionLifeTime"]);
      job.SessionLifeTime = time;
    }

    if (bool(resource["TotalCPUTime"])) {
      long value = get_limit(resource["TotalCPUTime"]);
      if (value != -1) {
        Period time((time_t)value);
        if (job.TotalCPUTime < time)
          job.TotalCPUTime = time;
      }
    }

    if (bool(resource["IndividualCPUTime"])) {
      long value = get_limit(resource["IndividualCPUTime"]);
      if (value != -1) {
        Period time((time_t)value);
        job.IndividualCPUTime = time;
      }
    }

    if (bool(resource["IndividualPhysicalMemory"])) {
      long value = get_limit(resource["IndividualPhysicalMemory"]);
      if (value != -1)
        if (job.IndividualPhysicalMemory < value)
          job.IndividualPhysicalMemory = value;
    }

    if (bool(resource["IndividualVirtualMemory"])) {
      long value = get_limit(resource["IndividualVirtualMemory"]);
      if (value != -1)
        if (job.IndividualVirtualMemory < value)
          job.IndividualVirtualMemory = value;
    }

    if (bool(resource["IndividualDiskSpace"])) {
      long value = get_limit(resource["IndividualDiskSpace"]);
      if (value != -1)
        job.IndividualDiskSpace = value;
    }

    if (bool(resource["FileSystem"]["DiskSpace"])) {
      long value = get_limit(resource["FileSystem"]["DiskSpace"]);
      if (value != -1)
        job.DiskSpace = value;
    }

    if (bool(resource["GridTimeLimit"])) {
      ReferenceTimeType rt;
      rt.benchmark = "gridtime";
      rt.value = 2800;
      rt.time = (std::string)resource["GridTimeLimit"];
      job.ReferenceTime.push_back(rt);
    }

    if (bool(resource["ExclusiveExecution"])) {
      if (lower((std::string)resource["ExclusiveExecution"]) == "true")
        job.ExclusiveExecution = true;
      else
        job.ExclusiveExecution = false;
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
      job.NetworkInfo = value;
    }

    if (bool(resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]))
      job.OSName = (std::string)resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"];

    if (bool(resource["OperatingSystem"]["OperatingSystemVersion"]))
      job.OSVersion = (std::string)resource["OperatingSystem"]["OperatingSystemVersion"];

    if (bool(resource["CPUArchitecture"]["CPUArchitectureName"]))
      job.Platform = (std::string)resource["CPUArchitecture"]["CPUArchitectureName"];

    if (bool(resource["CandidateTarget"]["HostName"])) {
      URL host_url((std::string)resource["CandidateTarget"]["HostName"]);
      job.EndPointURL = host_url;
    }

    if (bool(resource["CandidateTarget"]["QueueName"]))
      job.QueueName = (std::string)resource["CandidateTarget"]["QueueName"];

    if (bool(resource["Middleware"]["Name"]))
      job.CEType = (std::string)resource["Middleware"]["Name"];

    if (bool(resource["TotalCPUCount"]))
      job.ProcessPerHost = (int)get_limit(resource["TotalCPUCount"]);

    if (bool(resource["RunTimeEnvironment"]))
      for (int i = 0; (bool)(resource["RunTimeEnvironment"][i]); i++) {
        RunTimeEnvironmentType rt;
        std::string version;
        rt.Name = trim((std::string)resource["RunTimeEnvironment"][i]["Name"]);
        version = trim((std::string)resource["RunTimeEnvironment"][i]["Version"]["Exact"]);
        rt.Version.push_back(version);
        job.RunTimeEnvironment.push_back(rt);
      }
    // end of Resources

    // Datastaging
    XMLNode datastaging = node["JobDescription"]["DataStaging"];

    for (int i = 0; datastaging[i]; i++) {
      XMLNode ds = datastaging[i];
      XMLNode source_uri = ds["Source"]["URI"];
      XMLNode target_uri = ds["Target"]["URI"];
      XMLNode filenameNode = ds["FileName"];

      FileType file;

      if (filenameNode) {
        file.Name = (std::string)filenameNode;
        if (bool(source_uri)) {
          SourceType source;
          source.URI = (std::string)source_uri;
          source.Threads = -1;
          file.Source.push_back(source);
        }
        if (bool(target_uri)) {
          TargetType target;
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
        job.File.push_back(file);
      }
    }
    // end of Datastaging
    return job;
  }

  std::string PosixJSDLParser::UnParse(const JobDescription& job) const {
    NS nsList;
    nsList.insert(std::pair<std::string, std::string>("jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-posix", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-arc", "http://www.nordugrid.org/ws/schemas/jsdl-arc"));


    XMLNode jobdefinition("<JobDefinition/>");

    jobdefinition.Namespaces(nsList);

    XMLNode jobdescription = jobdefinition.NewChild("JobDescription");

    // JobIdentification
    if (!job.JobName.empty()) {
      if (!bool(jobdescription["JobIdentification"]))
        jobdescription.NewChild("JobIdentification");
      jobdescription["JobIdentification"].NewChild("JobName") = job.JobName;
    }

    if (job.OldJobIDs.size() != 0) {
      if (!bool(jobdescription["JobIdentification"]))
        jobdescription.NewChild("JobIdentification");
      for (std::list<URL>::const_iterator it = job.OldJobIDs.begin();
           it != job.OldJobIDs.end(); it++)
        jobdescription["JobIdentification"].NewChild("jsdl-arc:OldJobID") = it->str();
    }
    if (!job.JobProject.empty()) {
      if (!bool(jobdescription["JobIdentification"]))
        jobdescription.NewChild("JobIdentification");
      jobdescription["JobIdentification"].NewChild("JobProject") = job.JobProject;
    }
    // end of JobIdentification

    // Application
    if (!job.Executable.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Executable") = job.Executable;
    }
    if (!job.Argument.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      for (std::list<std::string>::const_iterator it = job.Argument.begin();
           it != job.Argument.end(); it++)
        jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Argument") = *it;
    }
    if (!job.Input.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Input") = job.Input;
    }
    if (!job.Output.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Output") = job.Output;
    }
    if (!job.Error.empty()) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Error") = job.Error;
    }
    for (std::list<EnvironmentType>::const_iterator it = job.Environment.begin();
         it != job.Environment.end(); it++) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      XMLNode environment = jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Environment");
      environment = (*it).value;
      environment.NewAttribute("name") = (*it).name_attribute;
    }
    if (job.TotalWallTime != -1) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      Period time((std::string)job.TotalWallTime);
      std::ostringstream oss;
      oss << time.GetPeriod();
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:WallTimeLimit") = oss.str();
    }
    if (job.NumberOfProcesses > -1 &&
        job.NumberOfProcesses >= job.Slots) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      if (!bool(jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"]))
        jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ProcessCountLimit");
      std::ostringstream oss;
      oss << job.NumberOfProcesses;
      jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] = oss.str();
    }
    if (job.Slots > -1 &&
        job.Slots >= job.NumberOfProcesses) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      if (!bool(jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"]))
        jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ProcessCountLimit");
      std::ostringstream oss;
      oss << job.Slots;
      jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] = oss.str();
    }
    if (job.ThreadPerProcesses > -1) {
      if (!bool(jobdescription["Application"]))
        jobdescription.NewChild("Application");
      if (!bool(jobdescription["Application"]["POSIXApplication"]))
        jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
      std::ostringstream oss;
      oss << job.ThreadPerProcesses;
      jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ThreadCountLimit") = oss.str();
    }
    // end of Application

    // DataStaging
    for (std::list<FileType>::const_iterator it = job.File.begin();
         it != job.File.end(); it++) {
      XMLNode datastaging = jobdescription.NewChild("DataStaging");
      if (!(*it).Name.empty())
        datastaging.NewChild("FileName") = (*it).Name;
      if ((*it).Source.size() != 0) {
        std::list<SourceType>::const_iterator it2;
        it2 = ((*it).Source).begin();
        if (trim(((*it2).URI).fullstr()) != "")
          datastaging.NewChild("Source").NewChild("URI") = ((*it2).URI).fullstr();
      }
      if ((*it).Target.size() != 0) {
        std::list<TargetType>::const_iterator it3;
        it3 = ((*it).Target).begin();
        if (trim(((*it3).URI).fullstr()) != "")
          datastaging.NewChild("Target").NewChild("URI") = ((*it3).URI).fullstr();
      }
      if ((*it).IsExecutable || (*it).Name == job.Executable)
        datastaging.NewChild("jsdl-arc:IsExecutable") = "true";
      if ((*it).KeepData)
        datastaging.NewChild("DeleteOnTermination") = "false";
      else
        datastaging.NewChild("DeleteOnTermination") = "true";
    }

    // Resources
    for (std::list<RunTimeEnvironmentType>::const_iterator it = job.RunTimeEnvironment.begin();
         it != job.RunTimeEnvironment.end(); it++) {
      XMLNode resources;
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
        XMLNode RTE = resources.NewChild("jsdl-arc:RunTimeEnvironment");
        RTE.NewChild("jsdl-arc:Name") = (*it).Name;
        XMLNode version = RTE.NewChild("jsdl-arc:Version").NewChild("jsdl-arc:Exact");
        version.NewAttribute("epsilon") = "0.5";
        version = *it_version;
      }
    }

    if (!job.CEType.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["Middleware"]))
        jobdescription["Resources"].NewChild("jsdl-arc:Middleware");
      jobdescription["Resources"]["Middleware"].NewChild("jsdl-arc:Name") = job.CEType;
      //jobdescription["Resources"]["Middleware"].NewChild("Version") = ?;
    }

    if (job.SessionLifeTime != -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["SessionLifeTime"]))
        jobdescription["Resources"].NewChild("jsdl-arc:SessionLifeTime");
      std::string outputValue;
      std::stringstream ss;
      ss << job.SessionLifeTime.GetPeriod();
      ss >> outputValue;
      jobdescription["Resources"]["SessionLifeTime"] = outputValue;
    }

    if (!job.ReferenceTime.empty()) {
      // TODO: what is the mapping
    }

    if (bool(job.EndPointURL)) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]))
        jobdescription["Resources"].NewChild("jsdl-arc:CandidateTarget");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]["HostName"]))
        jobdescription["Resources"]["CandidateTarget"].NewChild("jsdl-arc:HostName");
      jobdescription["Resources"]["CandidateTarget"]["HostName"] = job.EndPointURL.str();
    }

    if (!job.QueueName.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]))
        jobdescription["Resources"].NewChild("jsdl-arc:CandidateTarget");
      if (!bool(jobdescription["Resources"]["CandidateTarget"]["QueueName"]))
        jobdescription["Resources"]["CandidateTarget"].NewChild("jsdl-arc:QueueName");
      jobdescription["Resources"]["CandidateTarget"]["QueueName"] = job.QueueName;
    }

    if (!job.Platform.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["CPUArchitecture"]))
        jobdescription["Resources"].NewChild("CPUArchitecture");
      if (!bool(jobdescription["Resources"]["CPUArchitecture"]["CPUArchitectureName"]))
        jobdescription["Resources"]["CPUArchitecture"].NewChild("CPUArchitectureName");
      jobdescription["Resources"]["CPUArchitecture"]["CPUArchitectureName"] = job.Platform;
    }

    if (!job.OSName.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]))
        jobdescription["Resources"].NewChild("OperatingSystem");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]))
        jobdescription["Resources"]["OperatingSystem"].NewChild("OperatingSystemType");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]))
        jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"].NewChild("OperatingSystemName");
      jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"] = job.OSName;
    }

    if (!job.OSVersion.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]))
        jobdescription["Resources"].NewChild("OperatingSystem");
      if (!bool(jobdescription["Resources"]["OperatingSystem"]["OperatingSystemVersion"]))
        jobdescription["Resources"]["OperatingSystem"].NewChild("OperatingSystemVersion");
      jobdescription["Resources"]["OperatingSystem"]["OperatingSystemVersion"] = job.OSVersion;
    }

    if (job.ProcessPerHost > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["TotalCPUCount"]))
        jobdescription["Resources"].NewChild("TotalCPUCount");
      if (!bool(jobdescription["Resources"]["TotalCPUCount"]["LowerBoundRange"]))
        jobdescription["Resources"]["TotalCPUCount"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << job.ProcessPerHost;
      jobdescription["Resources"]["TotalCPUCount"]["LowerBoundRange"] = oss.str();
    }

    if (job.IndividualPhysicalMemory > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualPhysicalMemory"]))
        jobdescription["Resources"].NewChild("IndividualPhysicalMemory");
      if (!bool(jobdescription["Resources"]["IndividualPhysicalMemory"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualPhysicalMemory"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << job.IndividualPhysicalMemory;
      jobdescription["Resources"]["IndividualPhysicalMemory"]["LowerBoundRange"] = oss.str();
    }

    if (job.IndividualVirtualMemory > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualVirtualMemory"]))
        jobdescription["Resources"].NewChild("IndividualVirtualMemory");
      if (!bool(jobdescription["Resources"]["IndividualVirtualMemory"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualVirtualMemory"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << job.IndividualVirtualMemory;
      jobdescription["Resources"]["IndividualVirtualMemory"]["LowerBoundRange"] = oss.str();
    }

    if (job.IndividualDiskSpace > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualDiskSpace"]))
        jobdescription["Resources"].NewChild("IndividualDiskSpace");
      if (!bool(jobdescription["Resources"]["IndividualDiskSpace"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualDiskSpace"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << job.IndividualDiskSpace;
      jobdescription["Resources"]["IndividualDiskSpace"]["LowerBoundRange"] = oss.str();
    }

    if (job.TotalCPUTime != -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["TotalCPUTime"]))
        jobdescription["Resources"].NewChild("TotalCPUTime");
      if (!bool(jobdescription["Resources"]["TotalCPUTime"]["LowerBoundRange"]))
        jobdescription["Resources"]["TotalCPUTime"].NewChild("LowerBoundRange");
      Period time((std::string)job.TotalCPUTime);
      std::ostringstream oss;
      oss << time.GetPeriod();
      jobdescription["Resources"]["TotalCPUTime"]["LowerBoundRange"] = oss.str();
    }

    if (job.IndividualCPUTime != -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["IndividualCPUTime"]))
        jobdescription["Resources"].NewChild("IndividualCPUTime");
      if (!bool(jobdescription["Resources"]["IndividualCPUTime"]["LowerBoundRange"]))
        jobdescription["Resources"]["IndividualCPUTime"].NewChild("LowerBoundRange");
      Period time((std::string)job.IndividualCPUTime);
      std::ostringstream oss;
      oss << time.GetPeriod();
      jobdescription["Resources"]["IndividualCPUTime"]["LowerBoundRange"] = oss.str();
    }

    if (job.DiskSpace > -1) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["FileSystem"]))
        jobdescription["Resources"].NewChild("FileSystem");
      if (!bool(jobdescription["Resources"]["FileSystem"]["DiskSpace"]))
        jobdescription["Resources"]["FileSystem"].NewChild("DiskSpace");
      if (!bool(jobdescription["Resources"]["FileSystem"]["DiskSpace"]["LowerBoundRange"]))
        jobdescription["Resources"]["FileSystem"]["DiskSpace"].NewChild("LowerBoundRange");
      std::ostringstream oss;
      oss << job.DiskSpace;
      jobdescription["Resources"]["FileSystem"]["DiskSpace"]["LowerBoundRange"] = oss.str();
    }

    if (job.ExclusiveExecution) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["ExclusiveExecution"]))
        jobdescription["Resources"].NewChild("ExclusiveExecution") = "true";
    }

    if (!job.NetworkInfo.empty()) {
      std::string value = "";
      if (job.NetworkInfo == "100megabitethernet")
        value = "104857600.0";
      else if (job.NetworkInfo == "gigabitethernet")
        value = "1073741824.0";
      else if (job.NetworkInfo == "myrinet")
        value = "2147483648.0";
      else if (job.NetworkInfo == "infiniband")
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
    if (bool(job.AccessControl)) {
      if (!bool(jobdescription["AccessControl"]))
        jobdescription.NewChild("jsdl-arc:AccessControl");
      if (!bool(jobdescription["AccessControl"]["Type"]))
        jobdescription["AccessControl"].NewChild("jsdl-arc:Type") = "GACL";
      jobdescription["AccessControl"].NewChild("jsdl-arc:Content").NewChild(job.AccessControl);
    }

    // Notify
    if (!job.Notification.empty())
      for (std::list<NotificationType>::const_iterator it = job.Notification.begin();
           it != job.Notification.end(); it++) {
        XMLNode notify = jobdescription.NewChild("jsdl-arc:Notify");
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
    if (job.ProcessingStartTime != -1) {
      if (!bool(jobdescription["ProcessingStartTime"]))
        jobdescription.NewChild("jsdl-arc:ProcessingStartTime");
      jobdescription["ProcessingStartTime"] = job.ProcessingStartTime.str();
    }

    // Reruns
    if (job.LRMSReRun != -1) {
      if (!bool(jobdescription["Reruns"]))
        jobdescription.NewChild("jsdl-arc:Reruns");
      std::ostringstream oss;
      oss << job.LRMSReRun;
      jobdescription["Reruns"] = oss.str();
    }

    // LocalLogging.Directory
    if (!job.LogDir.empty()) {
      if (!bool(jobdescription["LocalLogging"]))
        jobdescription.NewChild("jsdl-arc:LocalLogging");
      if (!bool(jobdescription["LocalLogging"]["Directory"]))
        jobdescription["LocalLogging"].NewChild("jsdl-arc:Directory");
      jobdescription["LocalLogging"]["Directory"] = job.LogDir;
    }

    // CredentialServer.URL
    if (bool(job.CredentialService)) {
      if (!bool(jobdescription["CredentialServer"]))
        jobdescription.NewChild("jsdl-arc:CredentialServer");
      if (!bool(jobdescription["CredentialServer"]["URL"]))
        jobdescription["CredentialServer"].NewChild("jsdl-arc:URL");
      jobdescription["CredentialServer"]["URL"] = job.CredentialService.fullstr();
    }

    // RemoteLogging.URL
    if (bool(job.RemoteLogging)) {
      if (!bool(jobdescription["RemoteLogging"]))
        jobdescription.NewChild("jsdl-arc:RemoteLogging");
      if (!bool(jobdescription["RemoteLogging"]["URL"]))
        jobdescription["RemoteLogging"].NewChild("jsdl-arc:URL");
      jobdescription["RemoteLogging"]["URL"] = job.RemoteLogging.fullstr();
    }

    std::string product;
    jobdefinition.GetDoc(product, true);

    return product;
  }

} // namespace Arc
