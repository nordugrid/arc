// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <glibmm.h>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/JobDescription.h>

#include "ARCJSDLParser.h"

namespace Arc {

  ARCJSDLParser::ARCJSDLParser()
    : JobDescriptionParser() {}

  ARCJSDLParser::~ARCJSDLParser() {}

  static void XmlErrorHandler(void* ctx, const char* msg) {
    return;
  }

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

  JobDescription ARCJSDLParser::Parse(const std::string& source) const {
    JobDescription job;

    xmlParserCtxtPtr ctxt = xmlNewParserCtxt();

    if (ctxt == NULL) {
        logger.msg(DEBUG, "[ARCJSDLParser] Failed to create parser context");
    }
    xmlSetGenericErrorFunc(NULL, (xmlGenericErrorFunc)XmlErrorHandler);
    XMLNode node(source);
    xmlSetGenericErrorFunc(NULL, NULL);

    if (!node) {
        logger.msg(DEBUG, "[ARCJSDLParser] Parsing error: %s\n", (xmlGetLastError())->message);
    }
    else if (ctxt->valid == 0) {
        logger.msg(DEBUG, "[ARCJSDLParser] Validating error");
    }

    xmlFreeParserCtxt(ctxt);

    if (node.Size() == 0) {
      logger.msg(DEBUG, "[ARCJSDLParser] Wrong XML structure! ");
      return JobDescription();
    }

    // The source parsing start now.
    NS nsList;
    nsList.insert(std::pair<std::string, std::string>("jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-posix", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-arc", "http://www.nordugrid.org/ws/schemas/jsdl-arc"));

    node.Namespaces(nsList);

    XMLNode jobdescription = node["JobDescription"];

    // JobIdentification
    XMLNode jobidentification = node["JobDescription"]["JobIdentification"];

    // std::string JobName;
    if (bool(jobidentification["JobName"]))
      job.Identification.JobName = (std::string)jobidentification["JobName"];

    // std::string Description;
    if (bool(jobidentification["Description"]))
      job.Identification.Description = (std::string)jobidentification["Description"];

    // std::string JobVOName;
    if (bool(jobidentification["JobVOName"]))
      job.Identification.JobVOName = (std::string)jobidentification["JobVOName"];

    // ComputingActivityType JobType; // Skou: Unclear...
    // std::list<std::string> UserTag; // Skou: Unclear...

    // std::list<std::string> ActivityOldId;
    for (int i = 0; (bool)(jobidentification["ActivityOldId"][i]); i++)
      job.Identification.ActivityOldId.push_back((std::string)jobidentification["ActivityOldId"][i]);
    // end of JobIdentification


    // Application
    XMLNode xmlApplication = node["JobDescription"]["Application"];
    XMLNode xmlXApplication = xmlApplication["POSIXApplication"];
    if (!xmlXApplication)
      xmlXApplication = xmlApplication["HPCProfileApplication"];
    if (!xmlXApplication) {
      logger.msg(ERROR, "[ARCJSDLParser] An extended application element (POSIXApplication or HPCProfileApplication) was not found in the job description.");
      return JobDescription();
    }


    // ExecutableType Executable;
    if (bool(xmlXApplication["Executable"]))
      job.Application.Executable.Name = (std::string)xmlXApplication["Executable"];
    for (int i = 0; (bool)(xmlXApplication["Argument"][i]); i++)
      job.Application.Executable.Argument.push_back((std::string)xmlXApplication["Argument"][i]);

    // std::string Input;
    if (bool(xmlXApplication["Input"]))
      job.Application.Input = (std::string)xmlXApplication["Input"];

    // std::string Output;
    if (bool(xmlXApplication["Output"]))
      job.Application.Output = (std::string)xmlXApplication["Output"];

    // std::string Error;
    if (bool(xmlXApplication["Error"]))
      job.Application.Error = (std::string)xmlXApplication["Error"];

    // bool Join;

    // std::list< std::pair<std::string, std::string> > Environment;
    for (int i = 0; (bool)(xmlXApplication["Environment"][i]); i++) {
      XMLNode env = xmlXApplication["Environment"][i];
      XMLNode name = env.Attribute("name");
      if (!name) {
        logger.msg(DEBUG, "[ARCJSDLParser] Error during the parsing: missed the name attributes of the \"%s\" Environment", (std::string)env);
        return JobDescription();
      }
      job.Application.Environment.push_back(std::pair<std::string, std::string>(name, env));
    }

    // ExecutableType Prologue;
    if (bool(xmlApplication["Prologue"]))
      job.Application.Prologue.Name = (std::string)xmlApplication["Prologue"];
    for (int i = 0; (bool)(xmlApplication["PrologueArgument"][i]); i++)
      job.Application.Prologue.Argument.push_back((std::string)xmlApplication["PrologueArgument"][i]);

    // ExecutableType Epilogue;
    if (bool(xmlApplication["Epilogue"]))
      job.Application.Epilogue.Name = (std::string)xmlApplication["Epilogue"];
    for (int i = 0; (bool)(xmlApplication["EpilogueArgument"][i]); i++)
      job.Application.Epilogue.Argument.push_back((std::string)xmlApplication["EpilogueArgument"][i]);

    // std::string LogDir;
    if (bool(xmlApplication["LocalLogging"]))
      job.Application.LogDir = (std::string)xmlApplication["LocalLogging"];

    // std::list<URL> RemoteLogging;
    for (int i = 0; (bool)(xmlApplication["RemoteLogging"][i]); i++)
      job.Application.RemoteLogging.push_back(URL((std::string)xmlApplication["RemoteLogging"][i]));

    // int Rerun;
    if (bool(xmlApplication["Rerun"]))
      job.Application.Rerun = stringtoi((std::string)xmlApplication["Rerun"]);

    // Period SessionLifeTime;
    if (bool(xmlApplication["SessionLifeTime"]))
      job.Application.SessionLifeTime = Period((std::string)xmlApplication["SessionLifeTime"]);

    // Time ExpiryTime;
    if (bool(xmlApplication["ExpiryTime"]))
      job.Application.ExpiryTime = Time((std::string)xmlApplication["ExpiryTime"]);

    // Time ProcessingStartTime;
    if (bool(xmlApplication["ProcessingStartTime"]))
      job.Application.ProcessingStartTime = Time((std::string)xmlApplication["ProcessingStartTime"]);

/** Skou: How should this element be interpretted.
    // XMLNode Notification;
    if (bool(jobdescription["Notify"]))
      for (int i = 0; bool(jobdescription["Notify"][i]); i++) {
        NotificationType notify;
        for (int j = 0; bool(jobdescription["Notify"][i]["Endpoint"][j]); j++)
          notify.Address.push_back((std::string)jobdescription["Notify"][i]["Endpoint"][j]);

        for (int j = 0; bool(jobdescription["Notify"][i]["State"][j]); j++)
          notify.State.push_back((std::string)jobdescription["Notify"][i]["State"][j]);
        job.Notification.push_back(notify);
      }
*/

    // std::list<URL> CredentialService;
    for (int i = 0; (bool)(xmlApplication["CredentialService"][i]); i++)
      job.Application.CredentialService.push_back(URL((std::string)xmlApplication["CredentialService"][i]));

    // XMLNode AccessControl;
    if (bool(xmlApplication["AccessControl"]))
      xmlApplication["AccessControl"][0].New(job.Application.AccessControl);

    // End of Application

    // Resources
    XMLNode resource = node["JobDescription"]["Resources"];

    // std::string OSFamily;
    if (bool(resource["OSFamily"]))
      job.Resources.OSFamily = (std::string)resource["OSFamily"];

    // std::string OSName;
    if (bool(resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]))
      job.Resources.OSName = (std::string)resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"];

    // std::string OSVersion;
    if (bool(resource["OperatingSystem"]["OperatingSystemVersion"]))
      job.Resources.OSVersion = (std::string)resource["OperatingSystem"]["OperatingSystemVersion"];

    // std::string Platform;
    if (bool(resource["CPUArchitecture"]["CPUArchitectureName"]))
      job.Resources.Platform = (std::string)resource["CPUArchitecture"]["CPUArchitectureName"];

    // std::string NetworkInfo;
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
      job.Resources.NetworkInfo = value;
    }

    // Range<unsigned int> IndividualPhysicalMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualPhysicalMemory"])) {
      long value = get_limit(resource["IndividualPhysicalMemory"]);
      if (value > -1)
        job.Resources.IndividualPhysicalMemory = value;
    }
    else if (bool(xmlXApplication["MemoryLimit"])) {
      int64_t value = stringto<int64_t>((std::string)xmlXApplication["MemoryLimit"]);
      if (value > -1)
        job.Resources.IndividualPhysicalMemory = value;
    }

    // Range<unsigned int> IndividualVirtualMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualVirtualMemory"])) {
      long value = get_limit(resource["IndividualVirtualMemory"]);
      if (value > -1)
        job.Resources.IndividualVirtualMemory = value;
    }
    else if (bool(xmlXApplication["VirtualMemoryLimit"])) {
      int64_t value = stringto<int64_t>((std::string)xmlXApplication["VirtualMemoryLimit"]);
      if (value > -1)
        job.Resources.IndividualVirtualMemory = value;
    }

    // Range<unsigned int> TotalCPUTime;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["TotalCPUTime"])) {
      long value = get_limit(resource["TotalCPUTime"]);
      if (value > -1)
        job.Resources.TotalCPUTime = value;
    }
    else if (bool(xmlXApplication["CPUTimeLimit"])) {
      int64_t value = stringto<int64_t>((std::string)xmlXApplication["CPUTimeLimit"]);
      if (value > -1)
        job.Resources.TotalCPUTime = value;
    }

    // Range<unsigned int> IndividualCPUTime;
    if (bool(resource["IndividualCPUTime"])) {
      long value = get_limit(resource["IndividualCPUTime"]);
      if (value > -1)
        job.Resources.IndividualCPUTime = value;
    }

    // Range<unsigned int> TotalWallTime;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["TotalWallTime"])) {
      long value = get_limit(resource["TotalWallTime"]);
      if (value > -1)
        job.Resources.TotalWallTime = value;
    }
    else if (bool(xmlXApplication["WallTimeLimit"])) {
      int64_t value = stringto<int64_t>((std::string)xmlXApplication["WallTimeLimit"]);
      if (value > -1)
        job.Resources.TotalWallTime = value;
    }

    // Range<unsigned int> IndividualWallTime;
    if (bool(resource["IndividualWallTime"])) {
      long value = get_limit(resource["IndividualWallTime"]);
      if (value > -1)
        job.Resources.IndividualWallTime = value;
    }

/** Skou: Structure need to be defined.
 * Old structure follows...
    // XMLNode ReferenceTime;
    if (bool(resource["GridTimeLimit"])) {
      ReferenceTimeType rt;
      rt.benchmark = "gridtime";
      rt.value = 2800;
      rt.time = (std::string)resource["GridTimeLimit"];
      job.ReferenceTime.push_back(rt);
    }
*/

    // unsigned int IndividualDiskSpace;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualDiskSpace"])) {
      long value = get_limit(resource["IndividualDiskSpace"]);
      if (value > -1)
        job.Resources.IndividualDiskSpace = value;
    }
    else if (bool(resource["FileSystem"]["DiskSpace"])) {
      long value = get_limit(resource["FileSystem"]["DiskSpace"]);
      if (value > -1)
        job.Resources.IndividualDiskSpace = value;
    }

    // unsigned int CacheDiskSpace; // Skou: Unclear.
    // unsigned int SessionDiskSpace; // Skou: Unclear.
    // SessionDirectoryAccessMode SessionDirectoryAccess; // Skou: Need to be defined.

/** Skou: Currently not supported...
    // SoftwareRequirement CEType;
    if (bool(resource["Middleware"]["Name"]))
      job.Resources.CEType = (std::string)resource["Middleware"]["Name"];
*/

    // NodeAccessType NodeAccess; // Skou: Need to be defined.

    // ResourceSlotType Slots;
    if (bool(resource["Slots"]["NumberOfProcesses"])) {
      long value = get_limit(resource["Slots"]["NumberOfProcesses"]);
      if (value > -1)
        job.Resources.Slots.NumberOfProcesses = value;
    }
    else if (bool(xmlXApplication["ProcessCountLimit"])) {
      int64_t value = stringto<int64_t>((std::string)xmlXApplication["ProcessCountLimit"]);
      if (value > -1)
        job.Resources.Slots.NumberOfProcesses = value;
    }
    if (bool(resource["Slots"]["ThreadsPerProcesses"])) {
      long value = get_limit(resource["Slots"]["ThreadsPerProcesses"]);
      if (value > -1)
        job.Resources.Slots.ThreadsPerProcesses = value;
    }
    else if (bool(xmlXApplication["ThreadCountLimit"])) {
      int64_t value = stringto<int64_t>((std::string)xmlXApplication["ThreadCountLimit"]);
      if (value > -1)
        job.Resources.Slots.ThreadsPerProcesses = value;
    }
    if (bool(resource["TotalCPUCount"]))
      job.Resources.Slots.ProcessPerHost = abs(get_limit(resource["TotalCPUCount"]));
    // XMLNode SPMDVariation;

    // std::list<ResourceTargetType> CandidateTarget;
    if (bool(resource["CandidateTarget"])) {
      for (int i = 0; (bool)(resource["CandidateTarget"][i]); i++)
        if (bool(resource["CandidateTarget"][i]["HostName"]) || bool(resource["CandidateTarget"][i]["QueueName"])) {
          ResourceTargetType candidateTarget;
          candidateTarget.EndPointURL = URL((std::string)resource["CandidateTarget"][i]["HostName"]);
          candidateTarget.QueueName = (std::string)resource["CandidateTarget"][i]["QueueName"];
          job.Resources.CandidateTarget.push_back(candidateTarget);
      }
    }
    else if (bool(resource["CandidateHosts"]))
      for (int i = 0; (bool)(resource["CandidateHosts"]["HostName"][i]); i++) {
        ResourceTargetType candidateTarget;
        candidateTarget.EndPointURL = URL((std::string)resource["CandidateHosts"]["HostName"][i]);
        job.Resources.CandidateTarget.push_back(candidateTarget);
      }

/** Skou: Structure need to be defined.
    // SoftwareRequirement RunTimeEnvironment;
    if (bool(resource["RunTimeEnvironment"]))
      for (int i = 0; (bool)(resource["RunTimeEnvironment"][i]); i++) {
        RunTimeEnvironmentType rt;
        std::string version;
        rt.Name = trim((std::string)resource["RunTimeEnvironment"][i]["Name"]);
        version = trim((std::string)resource["RunTimeEnvironment"][i]["Version"]["Exact"]);
        rt.Version.push_back(version);
        job.RunTimeEnvironment.push_back(rt);
      }
*/

    // end of Resources

    // Datastaging
    XMLNode datastaging = node["JobDescription"]["DataStaging"];

    for (int i = 0; datastaging[i]; i++) {
      XMLNode ds = datastaging[i];
      XMLNode source = ds["Source"];
      XMLNode source_uri = source["URI"];
      XMLNode target = ds["Target"];
      XMLNode target_uri = target["URI"];
      XMLNode filenameNode = ds["FileName"];

      if (filenameNode) {
        FileType file;
        file.Name = (std::string)filenameNode;
        if (bool(source)) {
          DataSourceType source;
          if (bool(source_uri))
            source.URI = (std::string)source_uri;
          else
            source.URI = file.Name;
          source.Threads = -1;
          file.Source.push_back(source);
        }
        if (bool(target)) {
          DataTargetType target;
          if (bool(target_uri)) {
            target.URI = (std::string)target_uri;
          }
          target.Threads = -1;
          file.Target.push_back(target);
        }

        file.KeepData = !(ds["DeleteOnTermination"] && lower(((std::string)ds["DeleteOnTermination"])) == "true");
        file.IsExecutable = ds["IsExecutable"] && lower(((std::string)ds["IsExecutable"])) == "true";
        file.DownloadToCache = ds["DownloadToCache"] && lower(((std::string)ds["DownloadToCache"])) == "true";
        job.DataStaging.File.push_back(file);
      }
    }
    // end of Datastaging

    return job;
  }

  std::string ARCJSDLParser::UnParse(const JobDescription& job) const {
    NS nsList;
    nsList.insert(std::pair<std::string, std::string>("", "http://schemas.ggf.org/jsdl/2005/11/jsdl"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-posix", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
    nsList.insert(std::pair<std::string, std::string>("jsdl-arc", "http://www.nordugrid.org/ws/schemas/jsdl-arc"));


    XMLNode jobdefinition("<JobDefinition/>");

    jobdefinition.Namespaces(nsList);

    XMLNode jobdescription = jobdefinition.NewChild("JobDescription");

    // JobIdentification

    // std::string JobName;
    XMLNode xmlIdentification("<JobIdentification/>");
    if (!job.Identification.JobName.empty())
      xmlIdentification.NewChild("JobName") = job.Identification.JobName;

    // std::string Description;
    if (!job.Identification.Description.empty())
      xmlIdentification.NewChild("Description") = job.Identification.Description;

    // std::string JobVOName;
    if (!job.Identification.JobVOName.empty())
      xmlIdentification.NewChild("JobVOName") = job.Identification.JobVOName;

    // ComputingActivityType JobType; // Skou: Need to be defined.
    // std::list<std::string> UserTag; // Skou: Need to be defined.

    // std::list<std::string> ActivityOldId;
    for (std::list<std::string>::const_iterator it = job.Identification.ActivityOldId.begin();
         it != job.Identification.ActivityOldId.end(); it++)
      xmlIdentification.NewChild("ActivityOldId") = *it;

    if (xmlIdentification.Size() > 0)
      jobdescription.NewChild(xmlIdentification);
    // end of JobIdentification

    // Application
    XMLNode xmlApplication("<Application/>");
    XMLNode xmlPApplication("<POSIXApplication/>");
    XMLNode xmlHApplication("<HPCProfileApplication/>");

    // ExecutableType Executable;
    if (!job.Application.Executable.Name.empty()) {
      xmlPApplication.NewChild("jsdl-posix:Executable") = job.Application.Executable.Name;
      xmlHApplication.NewChild("jsdl-hpcpa:Executable") = job.Application.Executable.Name;
      for (std::list<std::string>::const_iterator it = job.Application.Executable.Argument.begin();
           it != job.Application.Executable.Argument.end(); it++) {
        xmlPApplication.NewChild("jsdl-posix:Argument") = *it;
        xmlHApplication.NewChild("jsdl-hpcpa:Argument") = *it;
      }
    }

    // std::string Input;
    if (!job.Application.Input.empty()) {
      xmlPApplication.NewChild("jsdl-posix:Input") = job.Application.Input;
      xmlHApplication.NewChild("jsdl-hpcpa:Input") = job.Application.Input;
    }

    // std::string Output;
    if (!job.Application.Output.empty()) {
      xmlPApplication.NewChild("jsdl-posix:Output") = job.Application.Output;
      xmlHApplication.NewChild("jsdl-hpcpa:Output") = job.Application.Output;
    }

    // std::string Error;
    if (!job.Application.Error.empty()) {
      xmlPApplication.NewChild("jsdl-posix:Error") = job.Application.Error;
      xmlHApplication.NewChild("jsdl-hpcpa:Error") = job.Application.Error;
    }

    // bool Join; // Skou: Unclear...

    // std::list< std::pair<std::string, std::string> > Environment;
    for (std::list< std::pair<std::string, std::string> >::const_iterator it = job.Application.Environment.begin();
         it != job.Application.Environment.end(); it++) {
      XMLNode pEnvironment = xmlPApplication.NewChild("jsdl-posix:Environment");
      XMLNode hEnvironment = xmlHApplication.NewChild("jsdl-hpcpa:Environment");
      pEnvironment.NewAttribute("name") = it->first;
      pEnvironment = it->second;
      hEnvironment.NewAttribute("name") = it->first;
      hEnvironment = it->second;
    }

    // ExecutableType Prologue;
    if (!job.Application.Prologue.Name.empty()) {
      xmlApplication.NewChild("Prologue") = job.Application.Prologue.Name;
      for (std::list<std::string>::const_iterator it = job.Application.Prologue.Argument.begin();
           it != job.Application.Prologue.Argument.end(); it++)
        xmlApplication.NewChild("PrologueArgument") = *it;
    }

    // ExecutableType Epilogue;
    if (!job.Application.Epilogue.Name.empty()) {
      xmlApplication.NewChild("Epilogue") = job.Application.Epilogue.Name;
      for (std::list<std::string>::const_iterator it = job.Application.Epilogue.Argument.begin();
           it != job.Application.Epilogue.Argument.end(); it++)
        xmlApplication.NewChild("EpilogueArgument") = *it;
    }

    // std::string LogDir;
    if (!job.Application.LogDir.empty())
      xmlApplication.NewChild("LocalLogging") = job.Application.LogDir;

    // std::list<URL> RemoteLogging;
    for (std::list<URL>::const_iterator it = job.Application.RemoteLogging.begin();
         it != job.Application.RemoteLogging.end(); it++)
      xmlApplication.NewChild("RemoteLogging") = it->str();

    // int Rerun;
    if (job.Application.Rerun > -1)
      xmlApplication.NewChild("Rerun") = tostring(job.Application.Rerun);

    // Period SessionLifeTime;
    if (job.Application.SessionLifeTime > -1)
      xmlApplication.NewChild("SessionLifeTime") = tostring(job.Application.SessionLifeTime);

    // Time ExpiryTime;
    if (job.Application.ExpiryTime > -1)
      xmlApplication.NewChild("ExpiryTime") = job.Application.ExpiryTime.str();


    // Time ProcessingStartTime;
    if (job.Application.ProcessingStartTime > -1)
      xmlApplication.NewChild("ProcessingStartTime") = job.Application.ProcessingStartTime.str();

/** Skou: Structure need to be defined.
 * Old structure follows...
    // XMLNode Notification;
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
*/

    // XMLNode AccessControl;
    if (bool(job.Application.AccessControl))
      xmlApplication.NewChild("AccessControl").NewChild(job.Application.AccessControl);


    // std::list<URL> CredentialService;
    for (std::list<URL>::const_iterator it = job.Application.CredentialService.begin();
         it != job.Application.CredentialService.end(); it++)
      xmlApplication.NewChild("CredentialService") = it->fullstr();

    // POSIX compliance...
    if (job.Resources.TotalWallTime > -1)
      xmlPApplication.NewChild("jsdl-posix:WallTimeLimit") = tostring(job.Resources.TotalWallTime);
    if (job.Resources.IndividualPhysicalMemory > -1)
      xmlPApplication.NewChild("jsdl-posix:MemoryLimit") = tostring(job.Resources.IndividualPhysicalMemory);
    if (job.Resources.TotalCPUTime > -1)
      xmlPApplication.NewChild("jsdl-posix:CPUTimeLimit") = tostring(job.Resources.TotalCPUTime);
    if (job.Resources.Slots.NumberOfProcesses > -1)
      xmlPApplication.NewChild("jsdl-posix:ProcessCountLimit") = tostring(job.Resources.Slots.NumberOfProcesses);
    if (job.Resources.IndividualVirtualMemory > -1)
      xmlPApplication.NewChild("jsdl-posix:VirtualMemoryLimit") = tostring(job.Resources.IndividualVirtualMemory);
    if (job.Resources.Slots.ThreadsPerProcesses > -1)
      xmlPApplication.NewChild("jsdl-posix:ThreadCountLimit") = tostring(job.Resources.Slots.ThreadsPerProcesses);

    if (xmlPApplication.Size() > 0)
      xmlApplication.NewChild(xmlPApplication);
    if (xmlHApplication.Size() > 0)
      xmlApplication.NewChild(xmlHApplication);
    if (xmlApplication.Size() > 0)
      jobdescription.NewChild(xmlApplication);
    // end of Application

    // Resources
    XMLNode xmlResources("<Resources/>");

    // std::string OSFamily;
    if (!job.Resources.OSFamily.empty())
      xmlResources.NewChild("OSFamily") = job.Resources.OSFamily;

    // std::string OSName;
    if (!job.Resources.OSName.empty())
      xmlResources.NewChild("OperatingSystem").
                   NewChild("OperatingSystemType").
                   NewChild("OperatingSystemName") = job.Resources.OSName;

    // std::string OSVersion;
    if (!job.Resources.OSVersion.empty()) {
      if (!(bool)xmlResources["OperatingSystem"])
        xmlResources.NewChild("OperatingSystem");
      xmlResources["OperatingSystem"].NewChild("OperatingSystemVersion") = job.Resources.OSVersion;
    }

    // std::string Platform;
    if (!job.Resources.Platform.empty())
      xmlResources.NewChild("CPUArchitecture").NewChild("CPUArchitectureName") = job.Resources.Platform;

    // std::string NetworkInfo;
    if (!job.Resources.NetworkInfo.empty()) {
      std::string value = "";
      if (job.Resources.NetworkInfo == "100megabitethernet")
        value = "104857600.0";
      else if (job.Resources.NetworkInfo == "gigabitethernet")
        value = "1073741824.0";
      else if (job.Resources.NetworkInfo == "myrinet")
        value = "2147483648.0";
      else if (job.Resources.NetworkInfo == "infiniband")
        value = "10737418240.0";

      if (value != "")
        xmlResources.NewChild("IndividualNetworkBandwidth").NewChild("LowerBoundRange") = value;
    }

    // Range<int64_t> IndividualPhysicalMemory;
    if (job.Resources.IndividualPhysicalMemory > -1)
      xmlResources.NewChild("IndividualPhysicalMemory").NewChild("LowerBoundRange") = tostring(job.Resources.IndividualPhysicalMemory);

    // Range<int64_t> IndividualVirtualMemory;
    if (job.Resources.IndividualVirtualMemory > -1)
      xmlResources.NewChild("IndividualVirtualMemory").NewChild("LowerBoundRange") = tostring(job.Resources.IndividualVirtualMemory);

    // Range<int> TotalCPUTime;
    if (job.Resources.TotalCPUTime != -1)
      xmlResources.NewChild("TotalCPUTime").NewChild("LowerBoundRange") = tostring(job.Resources.TotalCPUTime);

    // Range<int> IndividualCPUTime;
    if (job.Resources.IndividualCPUTime != -1)
      xmlResources.NewChild("IndividualCPUTime").NewChild("LowerBoundRange") = tostring(job.Resources.IndividualCPUTime);

    // Range<int> TotalWallTime;
    if (job.Resources.TotalWallTime != -1)
      xmlResources.NewChild("TotalWallTime").NewChild("LowerBoundRange") = tostring(job.Resources.TotalWallTime);

    // Range<int> IndividualWallTime;
    if (job.Resources.IndividualWallTime != -1)
      xmlResources.NewChild("IndividualWallTime").NewChild("LowerBoundRange") = tostring(job.Resources.IndividualWallTime);

/** Skou: Currently not supported...
    // XMLNode ReferenceTime;
    if (!job.ReferenceTime.empty()) {
    }
*/

    // int64_t IndividualDiskSpace;
    if (job.Resources.IndividualDiskSpace > -1) {
      xmlResources.NewChild("IndividualDiskSpace").NewChild("LowerBoundRange") = tostring(job.Resources.IndividualDiskSpace);
      // JSDL Compliance...
      xmlResources.NewChild("FileSystem").NewChild("DiskSpace").NewChild("LowerBoundRange") = tostring(job.Resources.IndividualDiskSpace);
    }

    // int64_t CacheDiskSpace; // Skou: Unclear...
    // int64_t SessionDiskSpace; // Skou: Unclear...
    // SessionDirectoryAccessMode SessionDirectoryAccess; // Skou: Need to be defined...

/** Skou: Currently not supported...
    // SoftwareRequirement CEType;
    if (!job.CEType.empty()) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");
      if (!bool(jobdescription["Resources"]["Middleware"]))
        jobdescription["Resources"].NewChild("jsdl-arc:Middleware");
      jobdescription["Resources"]["Middleware"].NewChild("jsdl-arc:Name") = job.CEType;
      //jobdescription["Resources"]["Middleware"].NewChild("Version") = ?;
    }
*/

    // NodeAccessType NodeAccess; // Skou: Need to be defined.

    // ResourceSlotType Slots;
    XMLNode xmlSlots("<Slots/>");
    if (job.Resources.Slots.NumberOfProcesses > -1)
      xmlSlots.NewChild("NumberOfProcesses").NewChild("LowerBoundedRange") = tostring(job.Resources.Slots.NumberOfProcesses);
    if (job.Resources.Slots.ProcessPerHost > -1)
      xmlSlots.NewChild("TotalCPUCount").NewChild("LowerBoundRange") = tostring(job.Resources.Slots.ProcessPerHost);
    if (job.Resources.Slots.ThreadsPerProcesses > -1)
      xmlSlots.NewChild("ThreadsPerProcesses").NewChild("LowerBoundedRange") = tostring(job.Resources.Slots.ThreadsPerProcesses);
    // XMLNode SPMDVariation;
    if (xmlSlots.Size() > 0)
      xmlResources.NewChild(xmlSlots);

    // std::list<ResourceTargetType> CandidateTarget;
    for (std::list<ResourceTargetType>::const_iterator it = job.Resources.CandidateTarget.begin();
         it != job.Resources.CandidateTarget.end(); it++) {
      XMLNode xmlCandidateTarget = xmlResources.NewChild("CandidateTarget");
      if (it->EndPointURL)
        xmlCandidateTarget.NewChild("HostName") = it->EndPointURL.str();
      if (!it->QueueName.empty())
        xmlCandidateTarget.NewChild("QueueName") = it->QueueName;
      if (xmlCandidateTarget.Size() > 0)
        xmlResources.NewChild(xmlCandidateTarget);
    }

/** Skou: Currently not supported...
    // SoftwareRequirement RunTimeEnvironment;
    for (std::list<RunTimeEnvironmentType>::const_iterator it = job.RunTimeEnvironment.begin();
         it != job.RunTimeEnvironment.end(); it++) {
      if (!bool(jobdescription["Resources"]))
        jobdescription.NewChild("Resources");

      XMLNode resources = jobdescription["Resources"];

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
*/
    // end of Resources

    // DataStaging
    for (std::list<FileType>::const_iterator it = job.DataStaging.File.begin();
         it != job.DataStaging.File.end(); it++) {
      XMLNode datastaging = jobdescription.NewChild("DataStaging");
      if (!(*it).Name.empty())
        datastaging.NewChild("FileName") = (*it).Name;
      if ((*it).Source.size() != 0) {
        XMLNode source = datastaging.NewChild("Source");
        if (trim((it->Source[0].URI).fullstr()) != "")
          source.NewChild("URI") = (it->Source[0].URI).fullstr();
      }
      if ((*it).Target.size() != 0) {
        std::list<DataTargetType>::const_iterator it3;
        it3 = ((*it).Target).begin();
        XMLNode target = datastaging.NewChild("Target");
        if (trim(((*it3).URI).fullstr()) != "")
          target.NewChild("URI") = ((*it3).URI).fullstr();
      }
      if (it->IsExecutable || it->Name == job.Application.Executable.Name)
        datastaging.NewChild("jsdl-arc:IsExecutable") = "true";
      datastaging.NewChild("DeleteOnTermination") = (it->KeepData ? "false" : "true");
      if (it->DownloadToCache)
        datastaging.NewChild("DownloadToCache") = "true";
    }
    // End of DataStaging

    std::string product;
    jobdefinition.GetDoc(product, true);

    return product;
  }

} // namespace Arc
