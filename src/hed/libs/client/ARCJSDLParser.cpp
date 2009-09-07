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

  bool ARCJSDLParser::parseSoftware(const XMLNode& xmlSoftware, SoftwareRequirement& sr) const {
    for (int i = 0; (bool)(xmlSoftware["Software"][i]); i++) {
      SWComparisonOperator comOp = &Software::operator==;
      if (bool(xmlSoftware["Software"][i]["Version"].Attribute("require"))) {
        const std::string comOpStr = (std::string)xmlSoftware["Software"][i]["Version"].Attribute("require");
        if (comOpStr == "!=" || lower(comOpStr) == "ne")
          comOp = &Software::operator!=;
        else if (comOpStr == ">" || lower(comOpStr) == "gt")
          comOp = &Software::operator>;
        else if (comOpStr == "<" || lower(comOpStr) == "lt")
          comOp = &Software::operator<;
        else if (comOpStr == ">=" || lower(comOpStr) == "ge")
          comOp = &Software::operator>=;
        else if (comOpStr == "<=" || lower(comOpStr) == "le")
          comOp = &Software::operator<=;
        else if (comOpStr != "=" && comOpStr != "==" && lower(comOpStr) != "eq") {
          logger.msg(ERROR, "Unknown operator '%s' in attribute require in Version element", comOpStr);
          return false;
        }
      }
      
      sr.add(Software(trim((std::string)xmlSoftware["Software"][i]["Name"]),
                      trim((std::string)xmlSoftware["Software"][i]["Version"])),
             comOp);
    }

    return true;
  }

  void ARCJSDLParser::outputSoftware(const SoftwareRequirement& sr, XMLNode& arcJSDL) const {
    std::list<Software>::const_iterator itSW = sr.getSoftwareList().begin();
    std::list<SWComparisonOperator>::const_iterator itCO = sr.getComparisonOperatorList().begin();
    for (; itSW != sr.getSoftwareList().end(); itSW++, itCO++) {
      if (itSW->empty()) continue;

      XMLNode xmlSoftware = arcJSDL.NewChild("Software");
      if (!itSW->getFamily().empty()) xmlSoftware.NewChild("Family") = itSW->getFamily();
      xmlSoftware.NewChild("Name") = itSW->getName();
      if (!itSW->getVersion().empty()) {
        XMLNode xmlVersion = xmlSoftware.NewChild("Version");
        xmlVersion = itSW->getVersion();
        if (*itCO != &Software::operator ==)
          xmlVersion.NewAttribute("require") = Software::toString(*itCO);
      }
    }

    if (bool(arcJSDL["Software"]) && sr.isRequiringAll())
      arcJSDL.NewAttribute("require") = "all";
  }

  template<typename T>
  void ARCJSDLParser::parseRange(const XMLNode& xmlRange, Range<T>& range, const T& undefValue) const {
    if (!xmlRange) return;
    
    if (bool(xmlRange["Min"])) {
      if (!stringto<T>((std::string)xmlRange["Min"], range.min))
        range.min = undefValue;
    }
    else if (bool(xmlRange["LowerBoundedRange"])) {
      if (!stringto<T>((std::string)xmlRange["LowerBoundedRange"], range.min))
        range.min = undefValue;
    }
    
    if (bool(xmlRange["Max"])) {
      if (!stringto<T>((std::string)xmlRange["Max"], range.max))
        range.max = undefValue;
    }
    else if (bool(xmlRange["UpperBoundedRange"])) {
      if (!stringto<T>((std::string)xmlRange["UpperBoundedRange"]), range.max)
        range.max = undefValue;
    }
  }

  template<typename T>
  Range<T> ARCJSDLParser::parseRange(const XMLNode& xmlRange, const T& undefValue) const {
    Range<T> range;
    parseRange(xmlRange, range, undefValue);
    return range;
  }

  template<typename T>
  void ARCJSDLParser::outputARCJSDLRange(const Range<T>& range, XMLNode& arcJSDL, const T& undefValue) const {
    if (range.min != undefValue) {
      const std::string min = tostring(range.min);
      if (!min.empty()) arcJSDL.NewChild("Min") = min;
    }

    if (range.max != undefValue) {
      const std::string max = tostring(range.max);
      if (!max.empty()) arcJSDL.NewChild("Max") = max;
    }
  }

  template<typename T>
  void ARCJSDLParser::outputJSDLRange(const Range<T>& range, XMLNode& jsdl, const T& undefValue) const {
    if (range.min != undefValue) {
      const std::string lower = tostring(range.min);
      if (!lower.empty()) jsdl.NewChild("LowerBoundedRange") = lower;
    }

    if (range.max != undefValue) {
      const std::string upper = tostring(range.max);
      if (!upper.empty()) jsdl.NewChild("UpperBoundedRange") = upper;
    }
  }

  void ARCJSDLParser::parseBenchmark(const XMLNode& xmlBenchmark, std::pair<std::string, int>& benchmark) const {
    int value;
    if (bool(xmlBenchmark["BenchmarkType"]) &&
        bool(xmlBenchmark["BenchmarkValue"]) &&
        stringto(xmlBenchmark["BenchmarkValue"], value))
      benchmark = std::make_pair<std::string, int>((std::string)xmlBenchmark["BenchmarkType"], value);
  }

  void ARCJSDLParser::outputBenchmark(const std::pair<std::string, int>& benchmark, XMLNode& arcJSDL) const {
    if (!benchmark.first.empty()) {
      arcJSDL.NewChild("BenchmarkType") = benchmark.first;
      arcJSDL.NewChild("BenchmarkValue") = tostring(benchmark.second);
    }
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

    // ComputingActivityType JobType;
    if (!bool(jobidentification["JobType"]) || lower((std::string)jobidentification["JobType"]) == "single")
      job.Identification.JobType = SINGLE;
    else if (lower((std::string)jobidentification["JobType"]) == "collectionelement")
      job.Identification.JobType = COLLECTIONELEMENT;
    else if (lower((std::string)jobidentification["JobType"]) == "parallelelement")
      job.Identification.JobType = PARALLELELEMENT;
    else if (lower((std::string)jobidentification["JobType"]) == "workflownode")
      job.Identification.JobType = WORKFLOWNODE;
    
    // std::list<std::string> UserTag;
    for (int i = 0; (bool)(jobidentification["UserTag"][i]); i++)
      job.Identification.UserTag.push_back((std::string)jobidentification["UserTag"][i]);

    // std::list<std::string> ActivityOldId;
    for (int i = 0; (bool)(jobidentification["ActivityOldId"][i]); i++)
      job.Identification.ActivityOldId.push_back((std::string)jobidentification["ActivityOldId"][i]);
    // end of JobIdentification


    // Application
    XMLNode xmlApplication = node["JobDescription"]["Application"];

    // Look for extended application element. First look for POSIX and then HPCProfile.
    XMLNode xmlXApplication = xmlApplication["POSIXApplication"];
    if (!xmlXApplication)
      xmlXApplication = xmlApplication["HPCProfileApplication"];
    if (!xmlXApplication) {
      logger.msg(ERROR, "[ARCJSDLParser] An extended application element (POSIXApplication or HPCProfileApplication) was not found in the job description.");
      return JobDescription();
    }


    // ExecutableType Executable;
    if (bool(xmlApplication["Executable"]["Path"])) {
      job.Application.Executable.Name = (std::string)xmlApplication["Executable"]["Path"];
      for (int i = 0; (bool)(xmlApplication["Executable"]["Argument"][i]); i++)
        job.Application.Executable.Argument.push_back((std::string)xmlApplication["Executable"]["Argument"]);
    }
    else if (bool(xmlXApplication["Executable"])) {
      job.Application.Executable.Name = (std::string)xmlXApplication["Executable"];
      for (int i = 0; (bool)(xmlXApplication["Argument"][i]); i++)
        job.Application.Executable.Argument.push_back((std::string)xmlXApplication["Argument"][i]);
    }

    // std::string Input;
    if (bool(xmlApplication["Input"]))
      job.Application.Input = (std::string)xmlApplication["Input"];
    else if (bool(xmlXApplication["Input"]))
      job.Application.Input = (std::string)xmlXApplication["Input"];

    // std::string Output;
    if (bool(xmlApplication["Output"]))
      job.Application.Output = (std::string)xmlApplication["Output"];
    else if (bool(xmlXApplication["Output"]))
      job.Application.Output = (std::string)xmlXApplication["Output"];

    // std::string Error;
    if (bool(xmlApplication["Error"]))
      job.Application.Error = (std::string)xmlApplication["Error"];
    else if (bool(xmlXApplication["Error"]))
      job.Application.Error = (std::string)xmlXApplication["Error"];

    // bool Join;
    job.Application.Join = (lower((std::string)xmlApplication["Join"]) == "true");

    // std::list< std::pair<std::string, std::string> > Environment;
    if (bool(xmlApplication["Environment"]["Name"])) {
      for (int i = 0; (bool)(xmlApplication["Environment"][i]); i++) {
        if (!((std::string)xmlApplication["Environment"][i]["Name"]).empty() && bool(xmlApplication["Environment"][i]["Value"]))
          job.Application.Environment.push_back(std::pair<std::string, std::string>((std::string)xmlApplication["Environment"][i]["Name"],
                                                                                    (std::string)xmlApplication["Environment"][i]["Value"]));
      }
    }
    else if (bool(xmlXApplication["Environment"])) {
      for (int i = 0; (bool)(xmlXApplication["Environment"][i]); i++) {
        XMLNode env = xmlXApplication["Environment"][i];
        XMLNode name = env.Attribute("name");
        if (!name || ((std::string)name).empty()) {
          logger.msg(DEBUG, "[ARCJSDLParser] Error during the parsing: missed the name attributes of the \"%s\" Environment", (std::string)env);
          return JobDescription();
        }
        job.Application.Environment.push_back(std::pair<std::string, std::string>(name, env));
      }
    }

    // ExecutableType Prologue;
    if (bool(xmlApplication["Prologue"]["Path"]))
      job.Application.Prologue.Name = (std::string)xmlApplication["Prologue"]["Path"];
    for (int i = 0; (bool)(xmlApplication["Prologue"]["Argument"][i]); i++)
      job.Application.Prologue.Argument.push_back((std::string)xmlApplication["Prologue"]["Argument"][i]);

    // ExecutableType Epilogue;
    if (bool(xmlApplication["Epilogue"]["Path"]))
      job.Application.Epilogue.Name = (std::string)xmlApplication["Epilogue"]["Path"];
    for (int i = 0; (bool)(xmlApplication["Epilogue"]["Argument"][i]); i++)
      job.Application.Epilogue.Argument.push_back((std::string)xmlApplication["Epilogue"]["Argument"][i]);

    // std::string LogDir;
    if (bool(xmlApplication["LogDir"]))
      job.Application.LogDir = (std::string)xmlApplication["LogDir"];

    // std::list<URL> RemoteLogging;
    for (int i = 0; (bool)(xmlApplication["RemoteLogging"][i]); i++) {
      URL url((std::string)xmlApplication["RemoteLogging"][i]);
      if (!url) {
        logger.msg(DEBUG, "[ARCJSDLParser] RemoteLogging URL is wrongly formatted.");
        return JobDescription();
      }
      job.Application.RemoteLogging.push_back(url);
    }

    // int Rerun;
    if (bool(xmlApplication["Rerun"]))
      job.Application.Rerun = stringtoi((std::string)xmlApplication["Rerun"]);

    // Time ExpiryTime;
    if (bool(xmlApplication["ExpiryTime"]))
      job.Application.ExpiryTime = Time((std::string)xmlApplication["ExpiryTime"]);

    // Time ProcessingStartTime;
    if (bool(xmlApplication["ProcessingStartTime"]))
      job.Application.ProcessingStartTime = Time((std::string)xmlApplication["ProcessingStartTime"]);

    // XMLNode Notification;
    for (int i = 0; bool(jobdescription["Notification"][i]); i++)
      job.Application.Notification.push_back((std::string)jobdescription["Notification"][i]);

    // std::list<URL> CredentialService;
    for (int i = 0; (bool)(xmlApplication["CredentialService"][i]); i++)
      job.Application.CredentialService.push_back(URL((std::string)xmlApplication["CredentialService"][i]));

    // XMLNode AccessControl;
    if (bool(xmlApplication["AccessControl"]))
      xmlApplication["AccessControl"][0].New(job.Application.AccessControl);

    // End of Application

    // Resources
    XMLNode resource = node["JobDescription"]["Resources"];

    // SoftwareRequirement OperatingSystem;
    if (bool(resource["OperatingSystem"])) {
      if (!parseSoftware(resource["OperatingSystem"], job.Resources.OperatingSystem))
        return JobDescription();
    }

    // std::string Platform;
    if (bool(resource["Platform"]))
      job.Resources.Platform = (std::string)resource["Platform"];
    else if (bool(resource["CPUArchitecture"]["CPUArchitectureName"]))
      job.Resources.Platform = (std::string)resource["CPUArchitecture"]["CPUArchitectureName"];

    // std::string NetworkInfo;
    if (bool(resource["NetworkInfo"]))
      job.Resources.NetworkInfo = (std::string)resource["NetworkInfo"];
    else if (bool(resource["IndividualNetworkBandwidth"])) {
      Range<long> bits_per_sec;
      parseRange<long>(resource["IndividualNetworkBandwidth"], bits_per_sec, -1);
      const long network = 1024 * 1024;
      if (bits_per_sec < 100 * network)
        job.Resources.NetworkInfo = "100megabitethernet";
      else if (bits_per_sec < 1024 * network)
        job.Resources.NetworkInfo = "gigabitethernet";
      else if (bits_per_sec < 10 * 1024 * network)
        job.Resources.NetworkInfo = "myrinet";
      else
        job.Resources.NetworkInfo = "infiniband";
    }

    // Range<int64_t> IndividualPhysicalMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualPhysicalMemory"]))
      parseRange<int64_t>(resource["IndividualPhysicalMemory"], job.Resources.IndividualPhysicalMemory, -1);
    else if (bool(xmlXApplication["MemoryLimit"])) {
      if (!stringto<int64_t>((std::string)xmlXApplication["MemoryLimit"], job.Resources.IndividualPhysicalMemory.max))
        job.Resources.IndividualPhysicalMemory = Range<int64_t>(-1);
    }

    // Range<int64_t> IndividualVirtualMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualVirtualMemory"])) {
      parseRange<int64_t>(resource["IndividualVirtualMemory"], job.Resources.IndividualVirtualMemory, -1);
    }
    else if (bool(xmlXApplication["VirtualMemoryLimit"])) {
      if (!stringto<int64_t>((std::string)xmlXApplication["VirtualMemoryLimit"], job.Resources.IndividualVirtualMemory.max))
        job.Resources.IndividualVirtualMemory = Range<int64_t>(-1);
    }

    // Range<int> IndividualCPUTime;
    if (bool(resource["IndividualCPUTime"]["Value"])) {
      parseRange<int>(resource["IndividualCPUTime"]["Value"], job.Resources.IndividualCPUTime.range, -1);
      parseBenchmark(resource["IndividualCPUTime"], job.Resources.IndividualCPUTime.benchmark);
    }
    else if (bool(resource["IndividualCPUTime"])) // JSDL compliance...
      parseRange<int>(resource["IndividualCPUTime"], job.Resources.IndividualCPUTime.range, -1);

    // Range<int> TotalCPUTime;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["TotalCPUTime"]["Value"])) {
      parseRange<int>(resource["TotalCPUTime"]["Value"], job.Resources.TotalCPUTime.range, -1);
      parseBenchmark(resource["TotalCPUTime"], job.Resources.TotalCPUTime.benchmark);
    }
    else if (bool(resource["TotalCPUTime"])) // JSDL compliance...
      parseRange<int>(resource["TotalCPUTime"], job.Resources.TotalCPUTime.range, -1);
    else if (bool(xmlXApplication["CPUTimeLimit"])) { // POSIX compliance...
      if (!stringto<int>((std::string)xmlXApplication["CPUTimeLimit"], job.Resources.TotalCPUTime.range.max))
        job.Resources.TotalCPUTime.range = Range<int>(-1);
    }

    // Range<int> IndividualWallTime;
    if (bool(resource["IndividualWallTime"]["Value"])) {
      parseRange<int>(resource["IndividualWallTime"]["Value"], job.Resources.IndividualWallTime.range, -1);
      parseBenchmark(resource["IndividualCPUTime"], job.Resources.IndividualWallTime.benchmark);
    }

    // Range<int> TotalWallTime;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["TotalWallTime"]["Value"])) {
      parseRange<int>(resource["TotalWallTime"]["Value"], job.Resources.TotalWallTime.range, -1);
      parseBenchmark(resource["TotalWallTime"], job.Resources.TotalWallTime.benchmark);
    }
    else if (bool(xmlXApplication["WallTimeLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["WallTimeLimit"], job.Resources.TotalWallTime.range.max))
        job.Resources.TotalWallTime.range = Range<int>(-1);
    }

    // Range<int64_t> DiskSpace;
    // If the consolidated element exist parse it, else try to parse the JSDL one.
    if (bool(resource["DiskSpaceRequirement"]["DiskSpace"]))
      parseRange<int64_t>(resource["DiskSpaceRequirement"]["DiskSpace"], job.Resources.DiskSpaceRequirement.DiskSpace, -1);
    else if (bool(resource["FileSystem"]["DiskSpace"]))
      parseRange<int64_t>(resource["FileSystem"]["DiskSpace"], job.Resources.DiskSpaceRequirement.DiskSpace, -1);

    // int64_t CacheDiskSpace;
    if (bool(resource["DiskSpaceRequirement"]["CacheDiskSpace"])) {
      if (!stringto<int64_t>((std::string)resource["DiskSpaceRequirement"]["CacheDiskSpace"], job.Resources.DiskSpaceRequirement.CacheDiskSpace))
         job.Resources.DiskSpaceRequirement.CacheDiskSpace = -1;
    }
    
    // int64_t SessionDiskSpace;
    if (bool(resource["DiskSpaceRequirement"]["SessionDiskSpace"])) {
      if (!stringto<int64_t>((std::string)resource["DiskSpaceRequirement"]["SessionDiskSpace"], job.Resources.DiskSpaceRequirement.SessionDiskSpace))
        job.Resources.DiskSpaceRequirement.SessionDiskSpace = -1;
    }

    // Period SessionLifeTime;
    if (bool(resource["SessionLifeTime"]))
      job.Resources.SessionLifeTime = Period((std::string)resource["SessionLifeTime"]);

    // SoftwareRequirement CEType;
    if (bool(resource["CEType"]))
      if (!parseSoftware(resource["CEType"], job.Resources.CEType))
        return JobDescription();

    // NodeAccessType NodeAccess;
    if (lower((std::string)resource["NodeAccess"]) == "inbound")
      job.Resources.NodeAccess = NAT_INBOUND;
    else if (lower((std::string)resource["NodeAccess"]) == "outbound")
      job.Resources.NodeAccess = NAT_OUTBOUND;
    else if (lower((std::string)resource["NodeAccess"]) == "inoutbound")
      job.Resources.NodeAccess = NAT_INOUTBOUND;

    // ResourceSlotType Slots;
    if (bool(resource["SlotRequirement"]["NumberOfSlots"]))
      parseRange<int>(resource["SlotRequirement"]["NumberOfSlots"], job.Resources.SlotRequirement.NumberOfSlots, -1);
    else if (bool(xmlXApplication["ProcessCountLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["ProcessCountLimit"], job.Resources.SlotRequirement.NumberOfSlots.max))
        job.Resources.SlotRequirement.NumberOfSlots = Range<int>(-1);
    }
    if (bool(resource["SlotRequirement"]["ThreadsPerProcesses"]))
      parseRange<int>(resource["SlotRequirement"]["ThreadsPerProcesses"], job.Resources.SlotRequirement.ThreadsPerProcesses, -1);
    else if (bool(xmlXApplication["ThreadCountLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["ThreadCountLimit"], job.Resources.SlotRequirement.ThreadsPerProcesses.max))
        job.Resources.SlotRequirement.ThreadsPerProcesses = Range<int>(-1);
    }
    if (bool(resource["SlotRequirement"]["ProcessPerHost"]))
      parseRange<int>(resource["SlotRequirement"]["ProcessPerHost"], job.Resources.SlotRequirement.ProcessPerHost, -1);
    else if (bool(resource["TotalCPUCount"]))
      parseRange<int>(resource["TotalCPUCount"], job.Resources.SlotRequirement.ProcessPerHost, -1);

    // std::string SPMDVariation;
    if (bool(resource["SlotRequirement"]["SPMDVariation"]))
      job.Resources.SlotRequirement.SPMDVariation = (std::string)resource["Slots"]["SPMDVariation"];
    

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

    // SoftwareRequirement RunTimeEnvironment;
    if (bool(resource["RunTimeEnvironment"])) {
      if (bool(resource["RunTimeEnvironment"].Attribute("require")))
        job.Resources.RunTimeEnvironment.setRequirement(lower((std::string)resource["RunTimeEnvironment"].Attribute("require")) == "all");
      if (!parseSoftware(resource["RunTimeEnvironment"], job.Resources.RunTimeEnvironment))
        return JobDescription();
    }
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
    XMLNode jobdefinition(NS("", "http://schemas.ggf.org/jsdl/2005/11/jsdl"), "JobDefinition");

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

    // ComputingActivityType JobType;
    switch (job.Identification.JobType) {
    case COLLECTIONELEMENT:
      xmlIdentification.NewChild("JobType") = "collectionelement";
      break;
    case PARALLELELEMENT:
      xmlIdentification.NewChild("JobType") = "parallelelement";
      break;
    case WORKFLOWNODE:
      xmlIdentification.NewChild("JobType") = "workflownode";
      break;
    default:
      xmlIdentification.NewChild("JobType") = "single";
      break;
    }
      
    // std::list<std::string> UserTag;
    for (std::list<std::string>::const_iterator it = job.Identification.UserTag.begin();
         it != job.Identification.UserTag.end(); it++)
      xmlIdentification.NewChild("UserTag") = *it;

    // std::list<std::string> ActivityOldId;
    for (std::list<std::string>::const_iterator it = job.Identification.ActivityOldId.begin();
         it != job.Identification.ActivityOldId.end(); it++)
      xmlIdentification.NewChild("ActivityOldId") = *it;

    if (xmlIdentification.Size() > 0)
      jobdescription.NewChild(xmlIdentification);
    // end of JobIdentification

    // Application
    XMLNode xmlApplication("<Application/>");
    XMLNode xmlPApplication(NS("posix-jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"), "POSIXApplication");
    XMLNode xmlHApplication(NS("hpcp-jsdl", "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa"), "HPCProfileApplication");

    // ExecutableType Executable;
    if (!job.Application.Executable.Name.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Executable") = job.Application.Executable.Name;
      xmlHApplication.NewChild("hpcp-jsdl:Executable") = job.Application.Executable.Name;
      for (std::list<std::string>::const_iterator it = job.Application.Executable.Argument.begin();
           it != job.Application.Executable.Argument.end(); it++) {
        xmlPApplication.NewChild("posix-jsdl:Argument") = *it;
        xmlHApplication.NewChild("hpcp-jsdl:Argument") = *it;
      }
    }

    // std::string Input;
    if (!job.Application.Input.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Input") = job.Application.Input;
      xmlHApplication.NewChild("hpcp-jsdl:Input") = job.Application.Input;
    }

    // std::string Output;
    if (!job.Application.Output.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Output") = job.Application.Output;
      xmlHApplication.NewChild("hpcp-jsdl:Output") = job.Application.Output;
    }

    // std::string Error;
    if (!job.Application.Error.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Error") = job.Application.Error;
      xmlHApplication.NewChild("hpcp-jsdl:Error") = job.Application.Error;
    }

    // bool Join;
    if (job.Application.Join)
      xmlApplication.NewChild("Join") = "true";

    // std::list< std::pair<std::string, std::string> > Environment;
    for (std::list< std::pair<std::string, std::string> >::const_iterator it = job.Application.Environment.begin();
         it != job.Application.Environment.end(); it++) {
      XMLNode pEnvironment = xmlPApplication.NewChild("posix-jsdl:Environment");
      XMLNode hEnvironment = xmlHApplication.NewChild("hpcp-jsdl:Environment");
      pEnvironment.NewAttribute("name") = it->first;
      pEnvironment = it->second;
      hEnvironment.NewAttribute("name") = it->first;
      hEnvironment = it->second;
    }

    // ExecutableType Prologue;
    if (!job.Application.Prologue.Name.empty()) {
      xmlApplication.NewChild("Prologue");
      xmlApplication["Prologue"].NewChild("Path") = job.Application.Prologue.Name;
      for (std::list<std::string>::const_iterator it = job.Application.Prologue.Argument.begin();
           it != job.Application.Prologue.Argument.end(); it++)
        xmlApplication["Prologue"].NewChild("Argument") = *it;
    }

    // ExecutableType Epilogue;
    if (!job.Application.Epilogue.Name.empty()) {
      xmlApplication.NewChild("Epilogue");
      xmlApplication["Epilogue"].NewChild("Path") = job.Application.Epilogue.Name;
      for (std::list<std::string>::const_iterator it = job.Application.Epilogue.Argument.begin();
           it != job.Application.Epilogue.Argument.end(); it++)
        xmlApplication["Epilogue"].NewChild("Argument") = *it;
    }

    // std::string LogDir;
    if (!job.Application.LogDir.empty())
      xmlApplication.NewChild("LogDir") = job.Application.LogDir;

    // std::list<URL> RemoteLogging;
    for (std::list<URL>::const_iterator it = job.Application.RemoteLogging.begin();
         it != job.Application.RemoteLogging.end(); it++)
      xmlApplication.NewChild("RemoteLogging") = it->str();

    // int Rerun;
    if (job.Application.Rerun > -1)
      xmlApplication.NewChild("Rerun") = tostring(job.Application.Rerun);

    // Time ExpiryTime;
    if (job.Application.ExpiryTime > -1)
      xmlApplication.NewChild("ExpiryTime") = job.Application.ExpiryTime.str();


    // Time ProcessingStartTime;
    if (job.Application.ProcessingStartTime > -1)
      xmlApplication.NewChild("ProcessingStartTime") = job.Application.ProcessingStartTime.str();

    // XMLNode Notification;
    for (std::list<std::string>::const_iterator it = job.Application.Notification.begin();
         it != job.Application.Notification.end(); it++)
      xmlApplication.NewChild("Notification") = *it;

    // XMLNode AccessControl;
    if (bool(job.Application.AccessControl))
      xmlApplication.NewChild("AccessControl").NewChild(job.Application.AccessControl);

    // std::list<URL> CredentialService;
    for (std::list<URL>::const_iterator it = job.Application.CredentialService.begin();
         it != job.Application.CredentialService.end(); it++)
      xmlApplication.NewChild("CredentialService") = it->fullstr();

    // POSIX compliance...
    if (job.Resources.TotalWallTime.range.max != -1)
      xmlPApplication.NewChild("posix-jsdl:WallTimeLimit") = tostring(job.Resources.TotalWallTime.range.max);
    if (job.Resources.IndividualPhysicalMemory.max != -1)
      xmlPApplication.NewChild("posix-jsdl:MemoryLimit") = tostring(job.Resources.IndividualPhysicalMemory.max);
    if (job.Resources.TotalCPUTime.range.max != -1)
      xmlPApplication.NewChild("posix-jsdl:CPUTimeLimit") = tostring(job.Resources.TotalCPUTime.range.max);
    if (job.Resources.SlotRequirement.NumberOfSlots.max != -1)
      xmlPApplication.NewChild("posix-jsdl:ProcessCountLimit") = tostring(job.Resources.SlotRequirement.NumberOfSlots.max);
    if (job.Resources.IndividualVirtualMemory.max != -1)
      xmlPApplication.NewChild("posix-jsdl:VirtualMemoryLimit") = tostring(job.Resources.IndividualVirtualMemory.max);
    if (job.Resources.SlotRequirement.ThreadsPerProcesses.max != -1)
      xmlPApplication.NewChild("posix-jsdl:ThreadCountLimit") = tostring(job.Resources.SlotRequirement.ThreadsPerProcesses.max);

    if (xmlPApplication.Size() > 0)
      xmlApplication.NewChild(xmlPApplication);
    if (xmlHApplication.Size() > 0)
      xmlApplication.NewChild(xmlHApplication);
    if (xmlApplication.Size() > 0)
      jobdescription.NewChild(xmlApplication);
    // end of Application

    // Resources
    XMLNode xmlResources("<Resources/>");

    // SoftwareRequirement OperatingSystem
    if (!job.Resources.OperatingSystem.empty()) {
      XMLNode xmlOS = xmlResources.NewChild("OperatingSystem");
      
      outputSoftware(job.Resources.OperatingSystem, xmlOS);

      // JSDL compliance. Only the first element in the OperatingSystem object is printed.
      xmlOS.NewChild("jsdl:OperatingSystemType").NewChild("OperatingSystemName") = job.Resources.OperatingSystem.getSoftwareList().front().getName();
      if (!job.Resources.OperatingSystem.getSoftwareList().front().getVersion().empty())
        xmlOS.NewChild("jsdl:OperatingSystemVersion") = job.Resources.OperatingSystem.getSoftwareList().front().getVersion();
    }

    // std::string Platform;
    if (!job.Resources.Platform.empty()) {
      xmlResources.NewChild("Platform") = job.Resources.Platform;
      // JSDL compliance
      xmlResources.NewChild("CPUArchitecture").NewChild("CPUArchitectureName") = job.Resources.Platform;
    }

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
        xmlResources.NewChild("IndividualNetworkBandwidth").NewChild("LowerBoundedRange") = value;
    }

    // NodeAccessType NodeAccess;
    switch (job.Resources.NodeAccess) {
    case NAT_INBOUND:
      xmlResources.NewChild("NodeAccess") = "inbound";
      break;
    case NAT_OUTBOUND:
      xmlResources.NewChild("NodeAccess") = "outbound";
      break;
    case NAT_INOUTBOUND:
      xmlResources.NewChild("NodeAccess") = "inoutbound";
      break;
    }

    // Range<int64_t> IndividualPhysicalMemory;
    {
      XMLNode xmlIPM("<IndividualPhysicalMemory/>");
      outputARCJSDLRange(job.Resources.IndividualPhysicalMemory, xmlIPM, (int64_t)-1);
      // JSDL compliance...
      outputJSDLRange(job.Resources.IndividualPhysicalMemory, xmlIPM, (int64_t)-1);
      if (xmlIPM.Size() > 0)
        xmlResources.NewChild(xmlIPM);
    }

    // Range<int64_t> IndividualVirtualMemory;
    {
      XMLNode xmlIVM("<IndividualVirtualMemory/>");
      outputARCJSDLRange(job.Resources.IndividualVirtualMemory, xmlIVM, (int64_t)-1);
      outputJSDLRange(job.Resources.IndividualVirtualMemory, xmlIVM, (int64_t)-1);
      if (xmlIVM.Size() > 0)
        xmlResources.NewChild(xmlIVM);
    }

    {
      // Range<int64_t> DiskSpace;
      XMLNode xmlDiskSpace("<DiskSpace/>");
      outputARCJSDLRange(job.Resources.DiskSpaceRequirement.DiskSpace, xmlDiskSpace, (int64_t)-1);
      // JSDL Compliance...
      outputJSDLRange(job.Resources.DiskSpaceRequirement.DiskSpace, xmlDiskSpace, (int64_t)-1);

      if (xmlDiskSpace.Size() > 0) {
        xmlResources.NewChild("DiskSpaceRequirement").NewChild(xmlDiskSpace);
          
        // int64_t CacheDiskSpace;
        if (job.Resources.DiskSpaceRequirement.CacheDiskSpace != -1)
          xmlResources["DiskSpaceRequirement"].NewChild("CacheDiskSpace") = tostring(job.Resources.DiskSpaceRequirement.CacheDiskSpace);

        // int64_t SessionDiskSpace;
        if (job.Resources.DiskSpaceRequirement.SessionDiskSpace != -1)
          xmlResources["DiskSpaceRequirement"].NewChild("SessionDiskSpace") = tostring(job.Resources.DiskSpaceRequirement.SessionDiskSpace);
      }
    }

    // Period SessionLifeTime;
    if (job.Resources.SessionLifeTime > -1)
      xmlResources.NewChild("SessionLifeTime") = tostring(job.Resources.SessionLifeTime);

    // ScalableTime<int> IndividualCPUTime;
    {
      XMLNode xmlICPUT("<IndividualCPUTime><Value/></IndividualCPUTime>");
      XMLNode xmlValue = xmlICPUT["Value"];
      outputARCJSDLRange(job.Resources.IndividualCPUTime.range, xmlValue, -1);
      if (xmlICPUT["Value"].Size() > 0) {
        outputBenchmark(job.Resources.IndividualCPUTime.benchmark, xmlICPUT);
        xmlResources.NewChild(xmlICPUT);

        // JSDL compliance...
        outputJSDLRange(job.Resources.IndividualCPUTime.range, xmlICPUT, -1);
      }
    }

    // ScalableTime<int> TotalCPUTime;
    {
      XMLNode xmlTCPUT("<TotalCPUTime><Value/></TotalCPUTime>");
      XMLNode xmlValue = xmlTCPUT["Value"];
      outputARCJSDLRange(job.Resources.TotalCPUTime.range, xmlValue, -1);
      if (xmlTCPUT["Value"].Size() > 0) {
        outputBenchmark(job.Resources.TotalCPUTime.benchmark, xmlTCPUT);
        xmlResources.NewChild(xmlTCPUT);

        // JSDL compliance...
        outputJSDLRange(job.Resources.TotalCPUTime.range, xmlTCPUT, -1);
      }
    }

    // ScalableTime<int> IndividualWallTime;
    {
     XMLNode xmlIWT("<IndividualWallTime><Value/></IndividualWallTime>");
      XMLNode xmlValue = xmlIWT["Value"];
      outputARCJSDLRange(job.Resources.IndividualWallTime.range, xmlValue, -1);
      if (xmlIWT["Value"].Size() > 0) {
        outputBenchmark(job.Resources.IndividualWallTime.benchmark, xmlIWT);
        xmlResources.NewChild(xmlIWT);
      }
    }

    // ScalableTime<int> TotalWallTime;
    {
     XMLNode xmlTWT("<TotalWallTime><Value/></TotalWallTime>");
      XMLNode xmlValue = xmlTWT["Value"];
      outputARCJSDLRange(job.Resources.TotalWallTime.range, xmlValue, -1);
      if (xmlTWT["Value"].Size() > 0) {
        outputBenchmark(job.Resources.TotalWallTime.benchmark, xmlTWT);
        xmlResources.NewChild(xmlTWT);
      }
    }

    // SoftwareRequirement CEType;
    if (!job.Resources.CEType.empty()) {
      XMLNode xmlCEType = xmlResources.NewChild("CEType");
      outputSoftware(job.Resources.CEType, xmlCEType);
    }

    // ResourceSlotType Slots;
    {
      XMLNode xmlSlotRequirement("<SlotRequirement/>");

      // Range<int> NumberOfProcesses;
      XMLNode xmlNOP("<NumberOfProcesses/>");
      outputARCJSDLRange(job.Resources.SlotRequirement.NumberOfSlots, xmlNOP, -1);
      if (xmlNOP.Size() > 0)
        xmlSlotRequirement.NewChild(xmlNOP);
      
      // Range<int> ProcessPerHost;
      XMLNode xmlPPH("<ProcessPerHost/>");
      outputARCJSDLRange(job.Resources.SlotRequirement.ProcessPerHost, xmlPPH, -1);
      if (xmlPPH.Size() > 0)
        xmlSlotRequirement.NewChild(xmlPPH);

      // Range<int> ThreadsPerProcesses;
      XMLNode xmlTPP("<ThreadsPerProcesses/>");
      outputARCJSDLRange(job.Resources.SlotRequirement.ThreadsPerProcesses, xmlTPP, -1);
      if (xmlTPP.Size() > 0)
        xmlSlotRequirement.NewChild(xmlTPP);

      // std::string SPMDVariation;
      if (!job.Resources.SlotRequirement.SPMDVariation.empty())
        xmlSlotRequirement.NewChild("SPMDVariation") = job.Resources.SlotRequirement.SPMDVariation;
      
      if (xmlSlotRequirement.Size() > 0)
        xmlResources.NewChild(xmlSlotRequirement);
    }

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

    // SoftwareRequirement RunTimeEnvironment;
    if (!job.Resources.RunTimeEnvironment.empty()) {
      XMLNode xmlRTE = xmlResources.NewChild("RunTimeEnvironment");
      outputSoftware(job.Resources.RunTimeEnvironment, xmlRTE);
    }

    if (xmlResources.Size() > 0)
      jobdescription.NewChild(xmlResources);

    // end of Resources

    // DataStaging
    for (std::list<FileType>::const_iterator it = job.DataStaging.File.begin();
         it != job.DataStaging.File.end(); it++) {
      XMLNode datastaging = jobdescription.NewChild("DataStaging");
      if (!(*it).Name.empty())
        datastaging.NewChild("FileName") = (*it).Name;
      if ((*it).Source.size() != 0) {
        XMLNode source = datastaging.NewChild("Source");
        if (trim((it->Source.begin()->URI).fullstr()) != "")
          source.NewChild("URI") = (it->Source.begin()->URI).fullstr();
      }
      if ((*it).Target.size() != 0) {
        std::list<DataTargetType>::const_iterator it3;
        it3 = ((*it).Target).begin();
        XMLNode target = datastaging.NewChild("Target");
        if (trim(((*it3).URI).fullstr()) != "")
          target.NewChild("URI") = ((*it3).URI).fullstr();
      }
      if (it->IsExecutable || it->Name == job.Application.Executable.Name)
        datastaging.NewChild("IsExecutable") = "true";
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
