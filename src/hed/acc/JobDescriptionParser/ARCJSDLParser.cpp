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

  #define JSDL_NAMESPACE "http://schemas.ggf.org/jsdl/2005/11/jsdl"
  #define JSDL_POSIX_NAMESPACE "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"
  #define JSDL_ARC_NAMESPACE "http://www.nordugrid.org/ws/schemas/jsdl-arc"
  #define JSDL_HPCPA_NAMESPACE "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa"

  ARCJSDLParser::ARCJSDLParser()
    : JobDescriptionParser() {
    supportedLanguages.push_back("nordugrid:jsdl");
  }

  ARCJSDLParser::~ARCJSDLParser() {}

  Plugin* ARCJSDLParser::Instance(PluginArgument *arg) {
    return new ARCJSDLParser();
  }

  bool ARCJSDLParser::parseSoftware(XMLNode xmlSoftware, SoftwareRequirement& sr) const {
    for (int i = 0; (bool)(xmlSoftware["Software"][i]); i++) {
      Software::ComparisonOperator comOp = &Software::operator==;
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
    std::list<Software::ComparisonOperator>::const_iterator itCO = sr.getComparisonOperatorList().begin();
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
  }

  template<typename T>
  void ARCJSDLParser::parseRange(XMLNode xmlRange, Range<T>& range, const T& undefValue) const {
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
      if (!stringto<T>((std::string)xmlRange["UpperBoundedRange"], range.max))
        range.max = undefValue;
    }
  }

  template<typename T>
  Range<T> ARCJSDLParser::parseRange(XMLNode xmlRange, const T& undefValue) const {
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

  void ARCJSDLParser::parseBenchmark(XMLNode xmlBenchmark, std::pair<std::string, double>& benchmark) const {
    int value;
    if (bool(xmlBenchmark["BenchmarkType"]) &&
        bool(xmlBenchmark["BenchmarkValue"]) &&
        stringto(xmlBenchmark["BenchmarkValue"], value))
      benchmark = std::make_pair<std::string, int>((std::string)xmlBenchmark["BenchmarkType"], value);
  }

  void ARCJSDLParser::outputBenchmark(const std::pair<std::string, double>& benchmark, XMLNode& arcJSDL) const {
    if (!benchmark.first.empty()) {
      arcJSDL.NewChild("BenchmarkType") = benchmark.first;
      arcJSDL.NewChild("BenchmarkValue") = tostring(benchmark.second);
    }
  }

  JobDescriptionParserResult ARCJSDLParser::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) const {
    if (language != "" && !IsLanguageSupported(language)) {
      return false;
    }

    logger.msg(VERBOSE, "Parsing string using ARCJSDLParser");

    jobdescs.clear();

    jobdescs.push_back(JobDescription());
    JobDescription& job = jobdescs.back();

    XMLNode node(source);
    if (!node) {
        logger.msg(VERBOSE, "[ARCJSDLParser] XML parsing error: %s\n", (xmlGetLastError())->message);
    }

    if (node.Size() == 0) {
      logger.msg(VERBOSE, "[ARCJSDLParser] Wrong XML structure! ");
      jobdescs.clear();
      return false;
    }

    // The source parsing start now.
    XMLNode jobdescription = node["JobDescription"];

    // Check if it is JSDL
    if((!jobdescription) || (!MatchXMLNamespace(jobdescription,JSDL_NAMESPACE))) {
      logger.msg(VERBOSE, "[ARCJSDLParser] Not a JSDL - missing JobDescription element");
      jobdescs.clear();
      return false;
    }

    // JobIdentification
    XMLNode jobidentification = node["JobDescription"]["JobIdentification"];

    // std::string JobName;
    if (bool(jobidentification["JobName"]))
      job.Identification.JobName = (std::string)jobidentification["JobName"];

    // std::string Description;
    if (bool(jobidentification["Description"]))
      job.Identification.Description = (std::string)jobidentification["Description"];

    // JSDL compliance
    if (bool(jobidentification["JobProject"]))
      job.OtherAttributes["nordugrid:jsdl;Identification/JobProject"] = (std::string)jobidentification["JobProject"];

    // std::list<std::string> Annotation;
    for (int i = 0; (bool)(jobidentification["UserTag"][i]); i++)
      job.Identification.Annotation.push_back((std::string)jobidentification["UserTag"][i]);

    // std::list<std::string> ActivityOldID;
    for (int i = 0; (bool)(jobidentification["ActivityOldId"][i]); i++)
      job.Identification.ActivityOldID.push_back((std::string)jobidentification["ActivityOldId"][i]);
    // end of JobIdentification


    // Application
    XMLNode xmlApplication = node["JobDescription"]["Application"];

    // Look for extended application element. First look for POSIX and then HPCProfile.
    XMLNode xmlXApplication = xmlApplication["POSIXApplication"];
    if (!xmlXApplication)
      xmlXApplication = xmlApplication["HPCProfileApplication"];


    // ExecutableType Executable;
    if (bool(xmlApplication["Executable"]["Path"])) {
      job.Application.Executable.Path = (std::string)xmlApplication["Executable"]["Path"];
      for (int i = 0; (bool)(xmlApplication["Executable"]["Argument"][i]); i++)
        job.Application.Executable.Argument.push_back((std::string)xmlApplication["Executable"]["Argument"]);
    }
    else if (bool(xmlXApplication["Executable"])) {
      job.Application.Executable.Path = (std::string)xmlXApplication["Executable"];
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
          logger.msg(VERBOSE, "[ARCJSDLParser] Error during the parsing: missed the name attributes of the \"%s\" Environment", (std::string)env);
          jobdescs.clear();
          return false;
        }
        job.Application.Environment.push_back(std::pair<std::string, std::string>(name, env));
      }
    }

    // std::list<ExecutableType> PreExecutable;
    if (bool(xmlApplication["Prologue"]["Path"])) {
      if (job.Application.PreExecutable.empty()) {
        job.Application.PreExecutable.push_back(ExecutableType());
      }
      job.Application.PreExecutable.front().Path = (std::string)xmlApplication["Prologue"]["Path"];
    }
    if (bool(xmlApplication["Prologue"]["Argument"])) {
      if (job.Application.PreExecutable.empty()) {
        job.Application.PreExecutable.push_back(ExecutableType());
      }
      for (int i = 0; (bool)(xmlApplication["Prologue"]["Argument"][i]); i++) {
        job.Application.PreExecutable.front().Argument.push_back((std::string)xmlApplication["Prologue"]["Argument"][i]);
      }
    }

    // std::list<ExecutableType> PostExecutable;
    if (bool(xmlApplication["Epilogue"]["Path"])) {
      if (job.Application.PostExecutable.empty()) {
        job.Application.PostExecutable.push_back(ExecutableType());
      }
      job.Application.PostExecutable.front().Path = (std::string)xmlApplication["Epilogue"]["Path"];
    }
    if (bool(xmlApplication["Epilogue"]["Argument"])) {
      if (job.Application.PostExecutable.empty()) {
        job.Application.PostExecutable.push_back(ExecutableType());
      }
      for (int i = 0; (bool)(xmlApplication["Epilogue"]["Argument"][i]); i++) {
        job.Application.PostExecutable.front().Argument.push_back((std::string)xmlApplication["Epilogue"]["Argument"][i]);
      }
    }

    // std::string LogDir;
    if (bool(xmlApplication["LogDir"]))
      job.Application.LogDir = (std::string)xmlApplication["LogDir"];

    // std::list<URL> RemoteLogging;
    for (int i = 0; (bool)(xmlApplication["RemoteLogging"][i]); i++) {
      URL url((std::string)xmlApplication["RemoteLogging"][i]);
      if (!url) {
        logger.msg(VERBOSE, "[ARCJSDLParser] RemoteLogging URL is wrongly formatted.");
        jobdescs.clear();
        return false;
      }
      job.Application.RemoteLogging.push_back(RemoteLoggingType());
      job.Application.RemoteLogging.back().ServiceType = "SGAS";
      job.Application.RemoteLogging.back().Location = url;
    }

    // int Rerun;
    if (bool(xmlApplication["Rerun"]))
      job.Application.Rerun = stringtoi((std::string)xmlApplication["Rerun"]);

    // int Priority
    if (bool(xmlApplication["Priority"])) {
      job.Application.Priority = stringtoi((std::string)xmlApplication["Priority"]);
      if (job.Application.Priority > 100) {
        logger.msg(VERBOSE, "[ARCJSDLParser] priority is too large - using max value 100");
        job.Application.Priority = 100;
      }
    }

    // Time ExpiryTime;
    if (bool(xmlApplication["ExpiryTime"]))
      job.Application.ExpiryTime = Time((std::string)xmlApplication["ExpiryTime"]);

    // Time ProcessingStartTime;
    if (bool(xmlApplication["ProcessingStartTime"]))
      job.Application.ProcessingStartTime = Time((std::string)xmlApplication["ProcessingStartTime"]);

    // XMLNode Notification;
    for (XMLNode n = xmlApplication["Notification"]; (bool)n; ++n) {
      // Accepting only supported notification types
      if(((bool)n["Type"]) && (n["Type"] != "Email")) continue;
      NotificationType notification;
      notification.Email = (std::string)n["Endpoint"];
      for (int j = 0; bool(n["State"][j]); j++) {
        notification.States.push_back((std::string)n["State"][j]);
      }
      job.Application.Notification.push_back(notification);
    }

    // std::list<URL> CredentialService;
    for (int i = 0; (bool)(xmlApplication["CredentialService"][i]); i++)
      job.Application.CredentialService.push_back(URL((std::string)xmlApplication["CredentialService"][i]));

    // XMLNode AccessControl;
    if (bool(xmlApplication["AccessControl"]))
      xmlApplication["AccessControl"][0].New(job.Application.AccessControl);

    if (bool(xmlApplication["DryRun"]) && lower((std::string)xmlApplication["DryRun"]) == "yes") {
      job.Application.DryRun = true;
    }

    // End of Application

    // Resources
    XMLNode resource = node["JobDescription"]["Resources"];

    // SoftwareRequirement OperatingSystem;
    if (bool(resource["OperatingSystem"])) {
      if (!parseSoftware(resource["OperatingSystem"], job.Resources.OperatingSystem))
        return false;
      if (!resource["OperatingSystem"]["Software"] &&
          resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"] &&
          resource["OperatingSystem"]["OperatingSystemVersion"])
        job.Resources.OperatingSystem = Software((std::string)resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"],
                                                 (std::string)resource["OperatingSystem"]["OperatingSystemVersion"]);
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
      Range<long long> bits_per_sec;
      parseRange<long long>(resource["IndividualNetworkBandwidth"], bits_per_sec, -1);
      const long long network = 1024 * 1024;
      if (bits_per_sec < 100 * network)
        job.Resources.NetworkInfo = "100megabitethernet";
      else if (bits_per_sec < 1024 * network)
        job.Resources.NetworkInfo = "gigabitethernet";
      else if (bits_per_sec < 10 * 1024 * network)
        job.Resources.NetworkInfo = "myrinet";
      else
        job.Resources.NetworkInfo = "infiniband";
    }

    // Range<int> IndividualPhysicalMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualPhysicalMemory"]))
      parseRange<int>(resource["IndividualPhysicalMemory"], job.Resources.IndividualPhysicalMemory, -1);
    else if (bool(xmlXApplication["MemoryLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["MemoryLimit"], job.Resources.IndividualPhysicalMemory.max))
        job.Resources.IndividualPhysicalMemory = Range<int>(-1);
    }

    // Range<int> IndividualVirtualMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualVirtualMemory"])) {
      parseRange<int>(resource["IndividualVirtualMemory"], job.Resources.IndividualVirtualMemory, -1);
    }
    else if (bool(xmlXApplication["VirtualMemoryLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["VirtualMemoryLimit"], job.Resources.IndividualVirtualMemory.max))
        job.Resources.IndividualVirtualMemory = Range<int>(-1);
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

    // Range<int> DiskSpace;
    // If the consolidated element exist parse it, else try to parse the JSDL one.
    if (bool(resource["DiskSpaceRequirement"]["DiskSpace"])) {
      Range<long long int> diskspace = -1;
      parseRange<long long int>(resource["DiskSpaceRequirement"]["DiskSpace"], diskspace, -1);
      if (diskspace > -1) {
        job.Resources.DiskSpaceRequirement.DiskSpace.max = diskspace.max/(1024*1024);
        job.Resources.DiskSpaceRequirement.DiskSpace.min = diskspace.min/(1024*1024);
      }
    }
    else if (bool(resource["FileSystem"]["DiskSpace"])) {
      Range<long long int> diskspace = -1;
      parseRange<long long int>(resource["FileSystem"]["DiskSpace"], diskspace, -1);
      if (diskspace > -1) {
        job.Resources.DiskSpaceRequirement.DiskSpace.max = diskspace.max/(1024*1024);
        job.Resources.DiskSpaceRequirement.DiskSpace.min = diskspace.min/(1024*1024);
      }
    }

    // int CacheDiskSpace;
    if (bool(resource["DiskSpaceRequirement"]["CacheDiskSpace"])) {
      long long int cachediskspace = -1;
      if (!stringto<long long int>((std::string)resource["DiskSpaceRequirement"]["CacheDiskSpace"], cachediskspace)) {
         job.Resources.DiskSpaceRequirement.CacheDiskSpace = -1;
      }
      else if (cachediskspace > -1) {
        job.Resources.DiskSpaceRequirement.CacheDiskSpace = cachediskspace/(1024*1024);
      }
    }

    // int SessionDiskSpace;
    if (bool(resource["DiskSpaceRequirement"]["SessionDiskSpace"])) {
      long long int sessiondiskspace = -1;
      if (!stringto<long long int>((std::string)resource["DiskSpaceRequirement"]["SessionDiskSpace"], sessiondiskspace)) {
        job.Resources.DiskSpaceRequirement.SessionDiskSpace = -1;
      }
     else if (sessiondiskspace > -1) {
        job.Resources.DiskSpaceRequirement.SessionDiskSpace = sessiondiskspace/(1024*1024);
      }
    }

    // Period SessionLifeTime;
    if (bool(resource["SessionLifeTime"]))
      job.Resources.SessionLifeTime = Period((std::string)resource["SessionLifeTime"]);

    // SoftwareRequirement CEType;
    if (bool(resource["CEType"])) {
      if (!parseSoftware(resource["CEType"], job.Resources.CEType)) {
        jobdescs.clear();
        return false;
      }
    }

    // NodeAccessType NodeAccess;
    if (lower((std::string)resource["NodeAccess"]) == "inbound")
      job.Resources.NodeAccess = NAT_INBOUND;
    else if (lower((std::string)resource["NodeAccess"]) == "outbound")
      job.Resources.NodeAccess = NAT_OUTBOUND;
    else if (lower((std::string)resource["NodeAccess"]) == "inoutbound")
      job.Resources.NodeAccess = NAT_INOUTBOUND;

    // ResourceSlotType Slots;
    if (bool(resource["SlotRequirement"]["NumberOfSlots"])) {
      if (!stringto<int>(resource["SlotRequirement"]["NumberOfSlots"], job.Resources.SlotRequirement.NumberOfSlots)) {
        job.Resources.SlotRequirement.NumberOfSlots = -1;
      }
    }
    else if (bool(xmlXApplication["ProcessCountLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["ProcessCountLimit"], job.Resources.SlotRequirement.NumberOfSlots)) {
        job.Resources.SlotRequirement.NumberOfSlots = -1;
      }
    }
    if (bool(resource["SlotRequirement"]["ThreadsPerProcesses"])) {
      if (!stringto<int>(resource["SlotRequirement"]["ThreadsPerProcesses"], job.Resources.ParallelEnvironment.ThreadsPerProcess)) {
        job.Resources.ParallelEnvironment.ThreadsPerProcess = -1;
      }
    }
    else if (bool(xmlXApplication["ThreadCountLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["ThreadCountLimit"], job.Resources.ParallelEnvironment.ThreadsPerProcess)) {
        job.Resources.ParallelEnvironment.ThreadsPerProcess = -1;
      }
    }
    if (bool(resource["SlotRequirement"]["ProcessPerHost"])) {
      if (!stringto<int>(resource["SlotRequirement"]["ProcessPerHost"], job.Resources.SlotRequirement.SlotsPerHost)) {
        job.Resources.SlotRequirement.SlotsPerHost = -1;
      }
    }
    else if (bool(resource["TotalCPUCount"])) {
      if (!stringto<int>(resource["TotalCPUCount"], job.Resources.SlotRequirement.SlotsPerHost)) {
        job.Resources.SlotRequirement.SlotsPerHost = -1;
      }
    }

    // std::string SPMDVariation;
    //if (bool(resource["SlotRequirement"]["SPMDVariation"]))
    //  job.Resources.SlotRequirement.SPMDVariation = (std::string)resource["Slots"]["SPMDVariation"];


    // std::string QueueName;
    if (bool(resource["QueueName"]) ||
        bool(resource["CandidateTarget"]["QueueName"]) // Be backwards compatible
        ) {
      XMLNode xmlQueue = (bool(resource["QueueName"]) ? resource["QueueName"] : resource["CandidateTarget"]["QueueName"]);
      std::string useQueue = (std::string)xmlQueue.Attribute("require");
      if (!useQueue.empty() && useQueue != "eq" && useQueue != "=" && useQueue != "==" && useQueue != "ne" && useQueue != "!=") {
        logger.msg(ERROR, "Parsing the \"require\" attribute of the \"QueueName\" nordugrid-JSDL element failed. An invalid comparison operator was used, only \"ne\" or \"eq\" are allowed.");
        jobdescs.clear();
        return false;
      }

      if (useQueue == "ne" || useQueue == "!=") {
        job.OtherAttributes["nordugrid:broker;reject_queue"] = (std::string)xmlQueue;
      }
      else {
        job.Resources.QueueName = (std::string)xmlQueue;
      }
    }

    // SoftwareRequirement RunTimeEnvironment;
    if (bool(resource["RunTimeEnvironment"])) {
      if (!parseSoftware(resource["RunTimeEnvironment"], job.Resources.RunTimeEnvironment)) {
        jobdescs.clear();
        return false;
      }
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

      if ((bool)filenameNode && (bool)source) {
        InputFileType file;
        file.Name = (std::string)filenameNode;
        if (bool(source_uri)) {
          URL source_url((std::string)source_uri);
          if (!source_url) {
            jobdescs.clear();
            return false;
          }
          // add any URL options
          XMLNode option = source["URIOption"];
          for (; (bool)option; ++option) {
            if (!source_url.AddOption((std::string)option, true)) {
              jobdescs.clear();
              return false;
            }
          }
          // add URL Locations, which may have their own options
          XMLNode location = source["Location"];
          for (; (bool)location; ++location) {
            XMLNode location_uri = location["URI"];
            if (!location_uri) {
              logger.msg(ERROR, "No URI element found in Location for file %s", file.Name);
              jobdescs.clear();
              return false;
            }
            URLLocation location_url((std::string)location_uri);
            if (!location_url || location_url.Protocol() == "file") {
              logger.msg(ERROR, "Location URI for file %s is invalid", file.Name);
              jobdescs.clear();
              return false;
            }
            XMLNode loc_option = location["URIOption"];
            for (; (bool)loc_option; ++loc_option) {
              if (!location_url.AddOption((std::string)loc_option, true)) {
                jobdescs.clear();
                return false;
              }
            }
            source_url.AddLocation(location_url);
          }
          file.Sources.push_back(source_url);
        }
        else {
          file.Sources.push_back(URL(file.Name));
        }
        if (ds["FileSize"]) {
          stringto<long>((std::string)ds["FileSize"], file.FileSize);
        }
        if (ds["Checksum"]) {
          file.Checksum = (std::string)ds["Checksum"];
        }
        file.IsExecutable = (ds["IsExecutable"] && lower(((std::string)ds["IsExecutable"])) == "true");
        // DownloadToCache does not make sense for output files.
        if (ds["DownloadToCache"] && !file.Sources.empty()) {
          file.Sources.back().AddOption("cache", (std::string)ds["DownloadToCache"]);
        }
        job.DataStaging.InputFiles.push_back(file);
      }

      if ((bool)filenameNode && bool(target) && bool(target_uri)) {
        OutputFileType file;
        file.Name = (std::string)filenameNode;
        URL target_url((std::string)target_uri);
        if (!target_url) {
          jobdescs.clear();
          return false;
        }
        // add any URL options
        XMLNode option = target["URIOption"];
        for (; (bool)option; ++option) {
          if (!target_url.AddOption((std::string)option, true)) {
            jobdescs.clear();
            return false;
          }
        }
        // add URL Locations, which may have their own options
        XMLNode location = target["Location"];
        for (; (bool)location; ++location) {
          XMLNode location_uri = location["URI"];
          if (!location_uri) {
            logger.msg(ERROR, "No URI element found in Location for file %s", file.Name);
            jobdescs.clear();
            return false;
          }
          URLLocation location_url((std::string)location_uri);
          if (!location_url || location_url.Protocol() == "file") {
            logger.msg(ERROR, "Location URI for file %s is invalid", file.Name);
            jobdescs.clear();
            return false;
          }
          XMLNode loc_option = location["URIOption"];
          for (; (bool)loc_option; ++loc_option) {
            if (!location_url.AddOption((std::string)loc_option, true)) {
              jobdescs.clear();
              return false;
            }
          }
          target_url.AddLocation(location_url);
        }
        file.Targets.push_back(target_url);
        job.DataStaging.OutputFiles.push_back(file);
      }

      bool deleteOnTermination = (ds["DeleteOnTermination"] && lower(((std::string)ds["DeleteOnTermination"])) == "false");
      if ((bool)filenameNode && deleteOnTermination) {
        OutputFileType file;
        file.Name = (std::string)filenameNode;
        job.DataStaging.OutputFiles.push_back(file);
      }
    }
    // end of Datastaging

    SourceLanguage(job) = (!language.empty() ? language : supportedLanguages.front());
    logger.msg(INFO, "String successfully parsed as %s.", job.GetSourceLanguage());
    return true;
  }

  JobDescriptionParserResult ARCJSDLParser::UnParse(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect) const {
    if (!IsLanguageSupported(language)) {
      return false;
    }

    if (job.Application.Executable.Path.empty()) {
      return false;
    }

    XMLNode jobdefinition(NS("", JSDL_NAMESPACE), "JobDefinition");

    XMLNode jobdescription = jobdefinition.NewChild("JobDescription");

    // JobIdentification

    // std::string JobName;
    XMLNode xmlIdentification("<JobIdentification/>");
    if (!job.Identification.JobName.empty())
      xmlIdentification.NewChild("JobName") = job.Identification.JobName;

    // std::string Description;
    if (!job.Identification.Description.empty())
      xmlIdentification.NewChild("Description") = job.Identification.Description;

    // JSDL compliance
    if (!job.OtherAttributes.empty()) {
      std::map<std::string, std::string>::const_iterator jrIt = job.OtherAttributes.find("nordugrid:jsdl;Identification/JobProject");
      if (jrIt != job.OtherAttributes.end()) {
        xmlIdentification.NewChild("JobProject") = jrIt->second;
      }
    }

    // std::list<std::string> Annotation;
    for (std::list<std::string>::const_iterator it = job.Identification.Annotation.begin();
         it != job.Identification.Annotation.end(); it++)
      xmlIdentification.NewChild("UserTag") = *it;

    // std::list<std::string> ActivityOldID;
    for (std::list<std::string>::const_iterator it = job.Identification.ActivityOldID.begin();
         it != job.Identification.ActivityOldID.end(); it++)
      xmlIdentification.NewChild("ActivityOldId") = *it;

    if (xmlIdentification.Size() > 0)
      jobdescription.NewChild(xmlIdentification);
    // end of JobIdentification

    // Application
    XMLNode xmlApplication("<Application/>");
    XMLNode xmlPApplication(NS("posix-jsdl", JSDL_POSIX_NAMESPACE), "posix-jsdl:POSIXApplication");
    XMLNode xmlHApplication(NS("hpcp-jsdl", JSDL_HPCPA_NAMESPACE), "hpcp-jsdl:HPCProfileApplication");

    // ExecutableType Executable;
    if (!job.Application.Executable.Path.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Executable") = job.Application.Executable.Path;
      xmlHApplication.NewChild("hpcp-jsdl:Executable") = job.Application.Executable.Path;
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

    // std::list<ExecutableType> PreExecutable;
    if (!job.Application.PreExecutable.empty() && !job.Application.PreExecutable.front().Path.empty()) {
      xmlApplication.NewChild("Prologue");
      xmlApplication["Prologue"].NewChild("Path") = job.Application.PreExecutable.front().Path;
      for (std::list<std::string>::const_iterator it = job.Application.PreExecutable.front().Argument.begin();
           it != job.Application.PreExecutable.front().Argument.end(); ++it) {
        xmlApplication["Prologue"].NewChild("Argument") = *it;
      }
    }

    // std::list<ExecutableType> PostExecutable;
    if (!job.Application.PostExecutable.empty() && !job.Application.PostExecutable.front().Path.empty()) {
      xmlApplication.NewChild("Epilogue");
      xmlApplication["Epilogue"].NewChild("Path") = job.Application.PostExecutable.front().Path;
      for (std::list<std::string>::const_iterator it = job.Application.PostExecutable.front().Argument.begin();
           it != job.Application.PostExecutable.front().Argument.end(); ++it)
        xmlApplication["Epilogue"].NewChild("Argument") = *it;
    }

    // std::string LogDir;
    if (!job.Application.LogDir.empty())
      xmlApplication.NewChild("LogDir") = job.Application.LogDir;

    // std::list<URL> RemoteLogging;
    for (std::list<RemoteLoggingType>::const_iterator it = job.Application.RemoteLogging.begin();
         it != job.Application.RemoteLogging.end(); it++) {
      if (it->ServiceType == "SGAS") {
        xmlApplication.NewChild("RemoteLogging") = it->Location.str();
      }
    }

    // int Rerun;
    if (job.Application.Rerun > -1)
      xmlApplication.NewChild("Rerun") = tostring(job.Application.Rerun);

    // int Priority;
    if (job.Application.Priority > -1)
      xmlApplication.NewChild("Priority") = tostring(job.Application.Priority);

    // Time ExpiryTime;
    if (job.Application.ExpiryTime > -1)
      xmlApplication.NewChild("ExpiryTime") = job.Application.ExpiryTime.str();


    // Time ProcessingStartTime;
    if (job.Application.ProcessingStartTime > -1)
      xmlApplication.NewChild("ProcessingStartTime") = job.Application.ProcessingStartTime.str();

    // XMLNode Notification;
    for (std::list<NotificationType>::const_iterator it = job.Application.Notification.begin();
         it != job.Application.Notification.end(); it++) {
      XMLNode n = xmlApplication.NewChild("Notification");
      n.NewChild("Type") = "Email";
      n.NewChild("Endpoint") = it->Email;
      for (std::list<std::string>::const_iterator s = it->States.begin();
                  s != it->States.end(); s++) {
        n.NewChild("State") = *s;
      }
    }

    // XMLNode AccessControl;
    if (bool(job.Application.AccessControl))
      xmlApplication.NewChild("AccessControl").NewChild(job.Application.AccessControl);

    // std::list<URL> CredentialService;
    for (std::list<URL>::const_iterator it = job.Application.CredentialService.begin();
         it != job.Application.CredentialService.end(); it++)
      xmlApplication.NewChild("CredentialService") = it->fullstr();

    if (job.Application.DryRun) {
      xmlApplication.NewChild("DryRun") = "yes";
    }

    // POSIX compliance...
    if (job.Resources.TotalWallTime.range.max != -1)
      xmlPApplication.NewChild("posix-jsdl:WallTimeLimit") = tostring(job.Resources.TotalWallTime.range.max);
    if (job.Resources.IndividualPhysicalMemory.max != -1)
      xmlPApplication.NewChild("posix-jsdl:MemoryLimit") = tostring(job.Resources.IndividualPhysicalMemory.max);
    if (job.Resources.TotalCPUTime.range.max != -1)
      xmlPApplication.NewChild("posix-jsdl:CPUTimeLimit") = tostring(job.Resources.TotalCPUTime.range.max);
    if (job.Resources.SlotRequirement.NumberOfSlots != -1)
      xmlPApplication.NewChild("posix-jsdl:ProcessCountLimit") = tostring(job.Resources.SlotRequirement.NumberOfSlots);
    if (job.Resources.IndividualVirtualMemory.max != -1)
      xmlPApplication.NewChild("posix-jsdl:VirtualMemoryLimit") = tostring(job.Resources.IndividualVirtualMemory.max);
    if (job.Resources.ParallelEnvironment.ThreadsPerProcess != -1)
      xmlPApplication.NewChild("posix-jsdl:ThreadCountLimit") = tostring(job.Resources.ParallelEnvironment.ThreadsPerProcess);

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
      xmlResources.NewChild("NetworkInfo") = job.Resources.NetworkInfo;

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
    case NAT_NONE:
      break;
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

    // Range<int> IndividualPhysicalMemory;
    {
      XMLNode xmlIPM("<IndividualPhysicalMemory/>");
      outputARCJSDLRange(job.Resources.IndividualPhysicalMemory, xmlIPM, (int)-1);
      // JSDL compliance...
      outputJSDLRange(job.Resources.IndividualPhysicalMemory, xmlIPM, (int)-1);
      if (xmlIPM.Size() > 0)
        xmlResources.NewChild(xmlIPM);
    }

    // Range<int> IndividualVirtualMemory;
    {
      XMLNode xmlIVM("<IndividualVirtualMemory/>");
      outputARCJSDLRange(job.Resources.IndividualVirtualMemory, xmlIVM, (int)-1);
      outputJSDLRange(job.Resources.IndividualVirtualMemory, xmlIVM, (int)-1);
      if (xmlIVM.Size() > 0)
        xmlResources.NewChild(xmlIVM);
    }

    {
      // Range<int> DiskSpace;
      XMLNode xmlDiskSpace("<DiskSpace/>");
      XMLNode xmlFileSystem("<FileSystem/>"); // JDSL compliance...
      if (job.Resources.DiskSpaceRequirement.DiskSpace.max != -1 || job.Resources.DiskSpaceRequirement.DiskSpace.min != -1) {
        Range<long long int> diskspace;
        if (job.Resources.DiskSpaceRequirement.DiskSpace.max != -1) {
          diskspace.max = job.Resources.DiskSpaceRequirement.DiskSpace.max*1024*1024;
        }
        if (job.Resources.DiskSpaceRequirement.DiskSpace.min != -1) {
          diskspace.min = job.Resources.DiskSpaceRequirement.DiskSpace.min*1024*1024;
        }
        outputARCJSDLRange(diskspace, xmlDiskSpace, (long long int)-1);

        // JSDL compliance...
        outputJSDLRange(diskspace, xmlFileSystem, (long long int)-1);
      }

      if (xmlDiskSpace.Size() > 0) {
        xmlResources.NewChild("DiskSpaceRequirement").NewChild(xmlDiskSpace);

        // int CacheDiskSpace;
        if (job.Resources.DiskSpaceRequirement.CacheDiskSpace > -1) {
          xmlResources["DiskSpaceRequirement"].NewChild("CacheDiskSpace") = tostring(job.Resources.DiskSpaceRequirement.CacheDiskSpace*1024*1024);
        }

        // int SessionDiskSpace;
        if (job.Resources.DiskSpaceRequirement.SessionDiskSpace > -1) {
          xmlResources["DiskSpaceRequirement"].NewChild("SessionDiskSpace") = tostring(job.Resources.DiskSpaceRequirement.SessionDiskSpace*1024*1024);
        }
      }

      // JSDL Compliance...
      if (xmlFileSystem.Size() > 0) {
        xmlResources.NewChild("FileSystem").NewChild(xmlFileSystem);
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

      // Range<int> NumberOfSlots;
      if (job.Resources.SlotRequirement.NumberOfSlots > -1) {
        xmlSlotRequirement.NewChild("NumberOfSlots") = tostring(job.Resources.SlotRequirement.NumberOfSlots);
      }

      // int ProcessPerHost;
      if (job.Resources.SlotRequirement.SlotsPerHost > -1) {
        xmlSlotRequirement.NewChild("ProcessPerHost") = tostring(job.Resources.SlotRequirement.SlotsPerHost);
        xmlResources.NewChild("TotalCPUCount") = tostring(job.Resources.SlotRequirement.SlotsPerHost);
      }

      // int ThreadsPerProcess;
      if (job.Resources.ParallelEnvironment.ThreadsPerProcess > -1) {
        xmlSlotRequirement.NewChild("ThreadsPerProcesses") = tostring(job.Resources.ParallelEnvironment.ThreadsPerProcess);
      }

      if (!job.Resources.ParallelEnvironment.Type.empty()) {
        xmlSlotRequirement.NewChild("SPMDVariation") = job.Resources.ParallelEnvironment.Type;
      }

      if (xmlSlotRequirement.Size() > 0) {
        xmlResources.NewChild(xmlSlotRequirement);
      }
    }

    // std::string QueueName;
    if (!job.Resources.QueueName.empty()) {;
      logger.msg(INFO, "job.Resources.QueueName = %s", job.Resources.QueueName);
      xmlResources.NewChild("QueueName") = job.Resources.QueueName;

      // Be backwards compatible with NOX versions of A-REX.
      xmlResources.NewChild("CandidateTarget").NewChild("QueueName") = job.Resources.QueueName;
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
    for (std::list<InputFileType>::const_iterator it = job.DataStaging.InputFiles.begin();
         it != job.DataStaging.InputFiles.end(); ++it) {
      if (it->Name.empty()) {
        return false;
      }
      XMLNode datastaging = jobdescription.NewChild("DataStaging");
      datastaging.NewChild("FileName") = it->Name;
      if (!it->Sources.empty() && it->Sources.front()) {
        if (it->Sources.front().Protocol() == "file") {
          datastaging.NewChild("Source");
        }
        else {
          XMLNode xs = datastaging.NewChild("Source");
          xs.NewChild("URI") = it->Sources.front().str();
          // add any URL options
          for (std::multimap<std::string, std::string>::const_iterator itOpt = it->Sources.front().Options().begin();
               itOpt != it->Sources.front().Options().end(); ++itOpt) {
            xs.NewChild("URIOption") = itOpt->first + "=" + itOpt->second;
          }
          // add URL Locations, which may have their own options
          for (std::list<URLLocation>::const_iterator itLoc = it->Sources.front().Locations().begin();
               itLoc != it->Sources.front().Locations().end(); ++itLoc) {
            XMLNode xloc = xs.NewChild("Location");
            xloc.NewChild("URI") = itLoc->str();

            for (std::multimap<std::string, std::string>::const_iterator itOpt = itLoc->Options().begin();
                 itOpt != itLoc->Options().end(); ++itOpt) {
              xloc.NewChild("URIOption") = itOpt->first + "=" + itOpt->second;
            }
          }
        }
      }
      if (it->IsExecutable) {
        datastaging.NewChild("IsExecutable") = "true";
      }
      if (it->FileSize > -1) {
        datastaging.NewChild("FileSize") = tostring(it->FileSize);
      }
      if (!it->Checksum.empty()) {
        datastaging.NewChild("Checksum") = it->Checksum;
      }
    }

    for (std::list<OutputFileType>::const_iterator it = job.DataStaging.OutputFiles.begin();
         it != job.DataStaging.OutputFiles.end(); ++it) {
      if (it->Name.empty()) {
        return false;
      }
      XMLNode datastaging = jobdescription.NewChild("DataStaging");
      datastaging.NewChild("FileName") = it->Name;
      if (!it->Targets.empty() && it->Targets.front()) {
        XMLNode xs = datastaging.NewChild("Target");
        xs.NewChild("URI") = it->Targets.front().str();
        // add any URL options
        for (std::multimap<std::string, std::string>::const_iterator itOpt = it->Targets.front().Options().begin();
             itOpt != it->Targets.front().Options().end(); ++itOpt) {
          xs.NewChild("URIOption") = itOpt->first + "=" + itOpt->second;
        }
        // add URL Locations, which may have their own options
        for (std::list<URLLocation>::const_iterator itLoc = it->Targets.front().Locations().begin();
             itLoc != it->Targets.front().Locations().end(); ++itLoc) {
          XMLNode xloc = xs.NewChild("Location");
          xloc.NewChild("URI") = itLoc->str();

          for (std::multimap<std::string, std::string>::const_iterator itOpt = itLoc->Options().begin();
               itOpt != itLoc->Options().end(); ++itOpt) {
            xloc.NewChild("URIOption") = itOpt->first + "=" + itOpt->second;
          }
        }
      }
      else {
        datastaging.NewChild("DeleteOnTermination") = "false";
      }
    }
    // End of DataStaging

    jobdefinition.GetDoc(product, true);

    return true;
  }

} // namespace Arc
