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
#include <arc/compute/JobDescription.h>

#include "XMLNodeRecover.h"
#include "ARCJSDLParser.h"

namespace Arc {

  #define JSDL_NAMESPACE "http://schemas.ggf.org/jsdl/2005/11/jsdl"
  #define JSDL_POSIX_NAMESPACE "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"
  #define JSDL_ARC_NAMESPACE "http://www.nordugrid.org/ws/schemas/jsdl-arc"
  #define JSDL_HPCPA_NAMESPACE "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa"

  ARCJSDLParser::ARCJSDLParser(PluginArgument* parg)
    : JobDescriptionParserPlugin(parg) {
    supportedLanguages.push_back("nordugrid:jsdl");
  }

  ARCJSDLParser::~ARCJSDLParser() {}

  Plugin* ARCJSDLParser::Instance(PluginArgument *arg) {
    return new ARCJSDLParser(arg);
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

      XMLNode xmlSoftware = arcJSDL.NewChild("arc-jsdl:Software");
      if (!itSW->getFamily().empty()) xmlSoftware.NewChild("arc-jsdl:Family") = itSW->getFamily();
      xmlSoftware.NewChild("arc-jsdl:Name") = itSW->getName();
      if (!itSW->getVersion().empty()) {
        XMLNode xmlVersion = xmlSoftware.NewChild("arc-jsdl:Version");
        xmlVersion = itSW->getVersion();
        if (*itCO != &Software::operator ==)
          xmlVersion.NewAttribute("require") = Software::toString(*itCO);
      }
    }
  }

  template<typename T>
  bool ARCJSDLParser::parseRange(XMLNode xmlRange, Range<T>& range) const {
    std::map<std::string, XMLNodeList> types;
    types["Exact"] = xmlRange.Path("Exact");
    types["UpperBoundedRange"] = xmlRange.Path("UpperBoundedRange");
    types["LowerBoundedRange"] = xmlRange.Path("LowerBoundedRange");
    types["Range"] = xmlRange.Path("Range");
    types["Range/LowerBound"] = xmlRange.Path("Range/LowerBound");
    types["Range/UpperBound"] = xmlRange.Path("Range/UpperBound");
    types["Min"] = xmlRange.Path("Min");
    types["Max"] = xmlRange.Path("Max");

    for (std::map<std::string, XMLNodeList>::const_iterator it = types.begin();
         it != types.end(); ++it) {
      if (it->second.size() > 1) {
        logger.msg(VERBOSE, "Multiple '%s' elements are not supported.", it->first);
        return false;
      }
    }

    XMLNodeList xmlMin, xmlMax;

    if(types["LowerBoundedRange"].size()) xmlMin.push_back(types["LowerBoundedRange"].front());
    if(types["Range/LowerBound"].size()) xmlMin.push_back(types["Range/LowerBound"].front());

    if(types["UpperBoundedRange"].size()) xmlMax.push_back(types["UpperBoundedRange"].front());
    if(types["Range/UpperBound"].size()) xmlMax.push_back(types["Range/UpperBound"].front());

    for(XMLNodeList::iterator xel = xmlMin.begin(); xel != xmlMin.end(); ++xel) {
      if (bool(xel->Attribute("exclusiveBound"))) {
        logger.msg(VERBOSE, "The 'exclusiveBound' attribute to the '%s' element is not supported.", xel->Name());
        return false;
      }
    }

    for(XMLNodeList::iterator xel = xmlMax.begin(); xel != xmlMax.end(); ++xel) {
      if (bool(xel->Attribute("exclusiveBound"))) {
        logger.msg(VERBOSE, "The 'exclusiveBound' attribute to the '%s' element is not supported.", xel->Name());
        return false;
      }
    }

    if (types["Exact"].size() == 1) {
      if (bool(types["Exact"].front().Attribute("epsilon"))) {
        logger.msg(VERBOSE, "The 'epsilon' attribute to the 'Exact' element is not supported.");
        return false;
      }
      xmlMax.push_back(types["Exact"].front());
    }

    if(types["Min"].size()) xmlMin.push_back(types["Min"].front());
    if(types["Max"].size()) xmlMax.push_back(types["Max"].front());

    return parseMinMax(xmlMin, xmlMax, range);
  }

  static std::string namesToString(XMLNodeList xlist) {
    std::string str;
    for(XMLNodeList::iterator xel = xlist.begin(); xel != xlist.end(); ++xel) {
      if(!str.empty()) str += ", ";
      str += xel->Name();
    }
    return str;
  }

  template<typename T>
  bool ARCJSDLParser::parseMinMax(XMLNodeList xmlMin, XMLNodeList xmlMax, Range<T>& range) const {
    std::pair<bool, double> min(false, .0), max(false, .0);

    for(XMLNodeList::iterator xel = xmlMax.begin(); xel != xmlMax.end(); ++xel) {
      double value;
      if(!stringto<double>((std::string)*xel, value)) {
        logger.msg(VERBOSE, "Parsing error: Value of %s element can't be parsed as number", xel->Name());
        return false;
      }
      if(max.first) {
        if(max.second != value) {
          logger.msg(VERBOSE, "Parsing error: Elements (%s) representing upper range have different values", namesToString(xmlMax));
          return false;
        }
      } else {
        max.second = value;
        max.first = true;
      }
    }

    for(XMLNodeList::iterator xel = xmlMin.begin(); xel != xmlMin.end(); ++xel) {
      double value;
      if(!stringto<double>((std::string)*xel, value)) {
        logger.msg(VERBOSE, "Parsing error: Value of %s element can't be parsed as number", xel->Name());
        return false;
      }
      if(min.first) {
        if(max.second != value) {
          logger.msg(VERBOSE, "Parsing error: Elements (%s) representing lower range have different values", namesToString(xmlMax));
        }
      } else {
        min.second = value;
        min.first = true;
      }
    }

    if (min.first && max.first && (min.second > max.second)) {
      logger.msg(VERBOSE, "Parsing error: Value of lower range (%s) is greater than value of upper range (%s)", namesToString(xmlMin), namesToString(xmlMax));
      return false;
    }

    if (min.first) range.min = static_cast<T>(min.second);
    if (max.first) range.max = static_cast<T>(max.second);

    return true;
  }

  template<typename T>
  void ARCJSDLParser::outputARCJSDLRange(const Range<T>& range, XMLNode& arcJSDL, const T& undefValue) const {
    if (range.min != undefValue) {
      const std::string min = tostring(range.min);
      if (!min.empty()) arcJSDL.NewChild("arc-jsdl:Min") = min;
    }

    if (range.max != undefValue) {
      const std::string max = tostring(range.max);
      if (!max.empty()) arcJSDL.NewChild("arc-jsdl:Max") = max;
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
      benchmark = std::pair<std::string, int>((std::string)xmlBenchmark["BenchmarkType"], value);
  }

  void ARCJSDLParser::outputBenchmark(const std::pair<std::string, double>& benchmark, XMLNode& arcJSDL) const {
    if (!benchmark.first.empty()) {
      arcJSDL.NewChild("arc-jsdl:BenchmarkType") = benchmark.first;
      arcJSDL.NewChild("arc-jsdl:BenchmarkValue") = tostring(benchmark.second);
    }
  }

  JobDescriptionParserPluginResult ARCJSDLParser::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) const {
    JobDescriptionParserPluginResult result(JobDescriptionParserPluginResult::WrongLanguage);
    if (language != "" && !IsLanguageSupported(language)) {
      return result;
    }

    XMLNodeRecover node(source);
    if (node.HasErrors()) {
      if (node.Name() != "JobDefinition") {
        return result;
      }
      else {
        result.SetFailure();
        for (std::list<xmlErrorPtr>::const_iterator itErr = node.GetErrors().begin();
             itErr != node.GetErrors().end(); ++itErr) {
          JobDescriptionParsingError err;
          err.message = (**itErr).message;
          if (err.message[err.message.length()-1] == '\n') {
            err.message = err.message.substr(0, err.message.length()-1);
          }
          if ((**itErr).line > 0 && (**itErr).int2 > 0) {
            err.line_pos = std::pair<int, int>((**itErr).line, (**itErr).int2);
          }
          result.AddError(err);
        }
        return result;
      }
    }

    JobDescription parsed_jobdescription;

    // The source parsing start now.
    XMLNode jobdescription = node["JobDescription"];

    // Check if it is JSDL
    if((!jobdescription) || (!MatchXMLNamespace(jobdescription,JSDL_NAMESPACE))) {
      logger.msg(VERBOSE, "[ARCJSDLParser] Not a JSDL - missing JobDescription element");
      return false;
    }

    // JobIdentification
    XMLNode jobidentification = node["JobDescription"]["JobIdentification"];

    // std::string JobName;
    if (bool(jobidentification["JobName"]))
      parsed_jobdescription.Identification.JobName = (std::string)jobidentification["JobName"];

    // std::string Description;
    if (bool(jobidentification["Description"]))
      parsed_jobdescription.Identification.Description = (std::string)jobidentification["Description"];

    // JSDL compliance
    if (bool(jobidentification["JobProject"]))
      parsed_jobdescription.OtherAttributes["nordugrid:jsdl;Identification/JobProject"] = (std::string)jobidentification["JobProject"];

    // std::list<std::string> Annotation;
    for (int i = 0; (bool)(jobidentification["UserTag"][i]); i++)
      parsed_jobdescription.Identification.Annotation.push_back((std::string)jobidentification["UserTag"][i]);

    // std::list<std::string> ActivityOldID;
    for (int i = 0; (bool)(jobidentification["ActivityOldId"][i]); i++)
      parsed_jobdescription.Identification.ActivityOldID.push_back((std::string)jobidentification["ActivityOldId"][i]);
    // end of JobIdentification


    // Application
    XMLNode xmlApplication = node["JobDescription"]["Application"];

    // Look for extended application element. First look for POSIX and then HPCProfile.
    XMLNode xmlXApplication = xmlApplication["POSIXApplication"];
    if (!xmlXApplication)
      xmlXApplication = xmlApplication["HPCProfileApplication"];


    // ExecutableType Executable;
    if (bool(xmlApplication["Executable"]["Path"])) {
      parsed_jobdescription.Application.Executable.Path = (std::string)xmlApplication["Executable"]["Path"];
      for (int i = 0; (bool)(xmlApplication["Executable"]["Argument"][i]); i++)
        parsed_jobdescription.Application.Executable.Argument.push_back((std::string)xmlApplication["Executable"]["Argument"]);
    }
    else if (bool(xmlXApplication["Executable"])) {
      parsed_jobdescription.Application.Executable.Path = (std::string)xmlXApplication["Executable"];
      for (int i = 0; (bool)(xmlXApplication["Argument"][i]); i++)
        parsed_jobdescription.Application.Executable.Argument.push_back((std::string)xmlXApplication["Argument"][i]);
    }

    // std::string Input;
    if (bool(xmlApplication["Input"]))
      parsed_jobdescription.Application.Input = (std::string)xmlApplication["Input"];
    else if (bool(xmlXApplication["Input"]))
      parsed_jobdescription.Application.Input = (std::string)xmlXApplication["Input"];

    // std::string Output;
    if (bool(xmlApplication["Output"]))
      parsed_jobdescription.Application.Output = (std::string)xmlApplication["Output"];
    else if (bool(xmlXApplication["Output"]))
      parsed_jobdescription.Application.Output = (std::string)xmlXApplication["Output"];

    // std::string Error;
    if (bool(xmlApplication["Error"]))
      parsed_jobdescription.Application.Error = (std::string)xmlApplication["Error"];
    else if (bool(xmlXApplication["Error"]))
      parsed_jobdescription.Application.Error = (std::string)xmlXApplication["Error"];

    // std::list< std::pair<std::string, std::string> > Environment;
    if (bool(xmlApplication["Environment"]["Name"])) {
      for (int i = 0; (bool)(xmlApplication["Environment"][i]); i++) {
        if (!((std::string)xmlApplication["Environment"][i]["Name"]).empty() && bool(xmlApplication["Environment"][i]["Value"]))
          parsed_jobdescription.Application.Environment.push_back(std::pair<std::string, std::string>((std::string)xmlApplication["Environment"][i]["Name"],
                                                                                    (std::string)xmlApplication["Environment"][i]["Value"]));
      }
    }
    else if (bool(xmlXApplication["Environment"])) {
      for (int i = 0; (bool)(xmlXApplication["Environment"][i]); i++) {
        XMLNode env = xmlXApplication["Environment"][i];
        XMLNode name = env.Attribute("name");
        if (!name || ((std::string)name).empty()) {
          logger.msg(VERBOSE, "[ARCJSDLParser] Error during the parsing: missed the name attributes of the \"%s\" Environment", (std::string)env);
          return false;
        }
        parsed_jobdescription.Application.Environment.push_back(std::pair<std::string, std::string>(name, env));
      }
    }

    // std::list<ExecutableType> PreExecutable;
    if (bool(xmlApplication["Prologue"]["Path"])) {
      if (parsed_jobdescription.Application.PreExecutable.empty()) {
        parsed_jobdescription.Application.PreExecutable.push_back(ExecutableType());
      }
      parsed_jobdescription.Application.PreExecutable.front().Path = (std::string)xmlApplication["Prologue"]["Path"];
    }
    if (bool(xmlApplication["Prologue"]["Argument"])) {
      if (parsed_jobdescription.Application.PreExecutable.empty()) {
        parsed_jobdescription.Application.PreExecutable.push_back(ExecutableType());
      }
      for (int i = 0; (bool)(xmlApplication["Prologue"]["Argument"][i]); i++) {
        parsed_jobdescription.Application.PreExecutable.front().Argument.push_back((std::string)xmlApplication["Prologue"]["Argument"][i]);
      }
    }

    // std::list<ExecutableType> PostExecutable;
    if (bool(xmlApplication["Epilogue"]["Path"])) {
      if (parsed_jobdescription.Application.PostExecutable.empty()) {
        parsed_jobdescription.Application.PostExecutable.push_back(ExecutableType());
      }
      parsed_jobdescription.Application.PostExecutable.front().Path = (std::string)xmlApplication["Epilogue"]["Path"];
    }
    if (bool(xmlApplication["Epilogue"]["Argument"])) {
      if (parsed_jobdescription.Application.PostExecutable.empty()) {
        parsed_jobdescription.Application.PostExecutable.push_back(ExecutableType());
      }
      for (int i = 0; (bool)(xmlApplication["Epilogue"]["Argument"][i]); i++) {
        parsed_jobdescription.Application.PostExecutable.front().Argument.push_back((std::string)xmlApplication["Epilogue"]["Argument"][i]);
      }
    }

    // std::string LogDir;
    if (bool(xmlApplication["LogDir"]))
      parsed_jobdescription.Application.LogDir = (std::string)xmlApplication["LogDir"];

    // std::list<URL> RemoteLogging;
    for (int i = 0; (bool)(xmlApplication["RemoteLogging"][i]); i++) {
      URL url((std::string)xmlApplication["RemoteLogging"][i]);
      if (!url) {
        logger.msg(VERBOSE, "[ARCJSDLParser] RemoteLogging URL is wrongly formatted.");
        return false;
      }
      parsed_jobdescription.Application.RemoteLogging.push_back(RemoteLoggingType());
      parsed_jobdescription.Application.RemoteLogging.back().ServiceType = "SGAS";
      parsed_jobdescription.Application.RemoteLogging.back().Location = url;
    }

    // int Rerun;
    if (bool(xmlApplication["Rerun"]))
      parsed_jobdescription.Application.Rerun = stringtoi((std::string)xmlApplication["Rerun"]);

    // int Priority
    if (bool(xmlApplication["Priority"])) {
      parsed_jobdescription.Application.Priority = stringtoi((std::string)xmlApplication["Priority"]);
      if (parsed_jobdescription.Application.Priority > 100) {
        logger.msg(VERBOSE, "[ARCJSDLParser] priority is too large - using max value 100");
        parsed_jobdescription.Application.Priority = 100;
      }
    }

    // Time ExpirationTime;
    if (bool(xmlApplication["ExpiryTime"]))
      parsed_jobdescription.Application.ExpirationTime = Time((std::string)xmlApplication["ExpiryTime"]);

    // Time ProcessingStartTime;
    if (bool(xmlApplication["ProcessingStartTime"]))
      parsed_jobdescription.Application.ProcessingStartTime = Time((std::string)xmlApplication["ProcessingStartTime"]);

    // XMLNode Notification;
    for (XMLNode n = xmlApplication["Notification"]; (bool)n; ++n) {
      // Accepting only supported notification types
      if(((bool)n["Type"]) && (n["Type"] != "Email")) continue;
      NotificationType notification;
      notification.Email = (std::string)n["Endpoint"];
      for (int j = 0; bool(n["State"][j]); j++) {
        notification.States.push_back((std::string)n["State"][j]);
      }
      parsed_jobdescription.Application.Notification.push_back(notification);
    }

    // std::list<URL> CredentialService;
    for (int i = 0; (bool)(xmlApplication["CredentialService"][i]); i++)
      parsed_jobdescription.Application.CredentialService.push_back(URL((std::string)xmlApplication["CredentialService"][i]));

    // XMLNode AccessControl;
    if (bool(xmlApplication["AccessControl"]))
      xmlApplication["AccessControl"].Child().New(parsed_jobdescription.Application.AccessControl);

    if (bool(xmlApplication["DryRun"]) && lower((std::string)xmlApplication["DryRun"]) == "yes") {
      parsed_jobdescription.Application.DryRun = true;
    }

    // End of Application

    // Resources
    XMLNode resource = node["JobDescription"]["Resources"];

    // SoftwareRequirement OperatingSystem;
    if (bool(resource["OperatingSystem"])) {
      if (!parseSoftware(resource["OperatingSystem"], parsed_jobdescription.Resources.OperatingSystem))
        return false;
      if (!resource["OperatingSystem"]["Software"] &&
          resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"] &&
          resource["OperatingSystem"]["OperatingSystemVersion"])
        parsed_jobdescription.Resources.OperatingSystem = Software((std::string)resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"],
                                                 (std::string)resource["OperatingSystem"]["OperatingSystemVersion"]);
    }

    // std::string Platform;
    if (bool(resource["Platform"]))
      parsed_jobdescription.Resources.Platform = (std::string)resource["Platform"];
    else if (bool(resource["CPUArchitecture"]["CPUArchitectureName"]))
      parsed_jobdescription.Resources.Platform = (std::string)resource["CPUArchitecture"]["CPUArchitectureName"];

    // std::string NetworkInfo;
    if (bool(resource["NetworkInfo"]))
      parsed_jobdescription.Resources.NetworkInfo = (std::string)resource["NetworkInfo"];
    else if (bool(resource["IndividualNetworkBandwidth"])) {
      Range<long long> bits_per_sec;
      if (!parseRange<long long>(resource["IndividualNetworkBandwidth"], bits_per_sec)) {
        return false;
      }
      const long long network = 1024 * 1024;
      if (bits_per_sec < 100 * network)
        parsed_jobdescription.Resources.NetworkInfo = "100megabitethernet";
      else if (bits_per_sec < 1024 * network)
        parsed_jobdescription.Resources.NetworkInfo = "gigabitethernet";
      else if (bits_per_sec < 10 * 1024 * network)
        parsed_jobdescription.Resources.NetworkInfo = "myrinet";
      else
        parsed_jobdescription.Resources.NetworkInfo = "infiniband";
    }

    // Range<int> IndividualPhysicalMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualPhysicalMemory"])) {
      if (!parseRange<int>(resource["IndividualPhysicalMemory"], parsed_jobdescription.Resources.IndividualPhysicalMemory)) {
        return false;
      }
    }
    else if (bool(xmlXApplication["MemoryLimit"])) {
      long long jsdlMemoryLimit = -1;
      if (stringto<long long>((std::string)xmlXApplication["MemoryLimit"], jsdlMemoryLimit)) {
        parsed_jobdescription.Resources.IndividualPhysicalMemory.max = (int)(jsdlMemoryLimit/(1024*1024));
      }
      else {
        parsed_jobdescription.Resources.IndividualPhysicalMemory = Range<int>(-1);
      }

    }

    // Range<int> IndividualVirtualMemory;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["IndividualVirtualMemory"])) {
      if (!parseRange<int>(resource["IndividualVirtualMemory"], parsed_jobdescription.Resources.IndividualVirtualMemory)) {
        return false;
      }
    }
    else if (bool(xmlXApplication["VirtualMemoryLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["VirtualMemoryLimit"], parsed_jobdescription.Resources.IndividualVirtualMemory.max))
        parsed_jobdescription.Resources.IndividualVirtualMemory = Range<int>(-1);
    }

    // Range<int> IndividualCPUTime;
    if (bool(resource["IndividualCPUTime"]["Value"])) {
      if (!parseRange<int>(resource["IndividualCPUTime"]["Value"], parsed_jobdescription.Resources.IndividualCPUTime.range)) {
        return false;
      }
      parseBenchmark(resource["IndividualCPUTime"], parsed_jobdescription.Resources.IndividualCPUTime.benchmark);
    }
    else if (bool(resource["IndividualCPUTime"])) { // JSDL compliance...
      if (!parseRange<int>(resource["IndividualCPUTime"], parsed_jobdescription.Resources.IndividualCPUTime.range)) {
        return false;
      }
    }

    // Range<int> TotalCPUTime;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["TotalCPUTime"]["Value"])) {
      if (!parseRange<int>(resource["TotalCPUTime"]["Value"], parsed_jobdescription.Resources.TotalCPUTime.range)) {
        return false;
      }
      parseBenchmark(resource["TotalCPUTime"], parsed_jobdescription.Resources.TotalCPUTime.benchmark);
    }
    else if (bool(resource["TotalCPUTime"])) { // JSDL compliance...
      if (!parseRange<int>(resource["TotalCPUTime"], parsed_jobdescription.Resources.TotalCPUTime.range)) {
        return false;
      }
    }
    else if (bool(xmlXApplication["CPUTimeLimit"])) { // POSIX compliance...
      if (!stringto<int>((std::string)xmlXApplication["CPUTimeLimit"], parsed_jobdescription.Resources.TotalCPUTime.range.max))
        parsed_jobdescription.Resources.TotalCPUTime.range = Range<int>(-1);
    }

    // Range<int> IndividualWallTime;
    if (bool(resource["IndividualWallTime"]["Value"])) {
      if (!parseRange<int>(resource["IndividualWallTime"]["Value"], parsed_jobdescription.Resources.IndividualWallTime.range)) {
        return false;
      }
      parseBenchmark(resource["IndividualCPUTime"], parsed_jobdescription.Resources.IndividualWallTime.benchmark);
    }

    // Range<int> TotalWallTime;
    // If the consolidated element exist parse it, else try to parse the POSIX one.
    if (bool(resource["TotalWallTime"]["Value"])) {
      if (!parseRange<int>(resource["TotalWallTime"]["Value"], parsed_jobdescription.Resources.TotalWallTime.range)) {
        return false;
      }
      parseBenchmark(resource["TotalWallTime"], parsed_jobdescription.Resources.TotalWallTime.benchmark);
    }
    else if (bool(xmlXApplication["WallTimeLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["WallTimeLimit"], parsed_jobdescription.Resources.TotalWallTime.range.max))
        parsed_jobdescription.Resources.TotalWallTime.range = Range<int>(-1);
    }

    // Range<int> DiskSpace;
    // If the consolidated element exist parse it, else try to parse the JSDL one.
    if (bool(resource["DiskSpaceRequirement"]["DiskSpace"])) {
      Range<long long int> diskspace = -1;
      if (!parseRange<long long int>(resource["DiskSpaceRequirement"]["DiskSpace"], diskspace)) {
        return false;
      }
      if (diskspace > -1) {
        parsed_jobdescription.Resources.DiskSpaceRequirement.DiskSpace.max = diskspace.max/(1024*1024);
        parsed_jobdescription.Resources.DiskSpaceRequirement.DiskSpace.min = diskspace.min/(1024*1024);
      }
    }
    else if (bool(resource["FileSystem"]["DiskSpace"])) {
      Range<long long int> diskspace = -1;
      if (!parseRange<long long int>(resource["FileSystem"]["DiskSpace"], diskspace)) {
        return false;
      }
      if (diskspace.max > -1) {
        parsed_jobdescription.Resources.DiskSpaceRequirement.DiskSpace.max = diskspace.max/(1024*1024);
      }
      if (diskspace.min > -1) {
        parsed_jobdescription.Resources.DiskSpaceRequirement.DiskSpace.min = diskspace.min/(1024*1024);
      }
    }

    // int CacheDiskSpace;
    if (bool(resource["DiskSpaceRequirement"]["CacheDiskSpace"])) {
      long long int cachediskspace = -1;
      if (!stringto<long long int>((std::string)resource["DiskSpaceRequirement"]["CacheDiskSpace"], cachediskspace)) {
         parsed_jobdescription.Resources.DiskSpaceRequirement.CacheDiskSpace = -1;
      }
      else if (cachediskspace > -1) {
        parsed_jobdescription.Resources.DiskSpaceRequirement.CacheDiskSpace = cachediskspace/(1024*1024);
      }
    }

    // int SessionDiskSpace;
    if (bool(resource["DiskSpaceRequirement"]["SessionDiskSpace"])) {
      long long int sessiondiskspace = -1;
      if (!stringto<long long int>((std::string)resource["DiskSpaceRequirement"]["SessionDiskSpace"], sessiondiskspace)) {
        parsed_jobdescription.Resources.DiskSpaceRequirement.SessionDiskSpace = -1;
      }
     else if (sessiondiskspace > -1) {
        parsed_jobdescription.Resources.DiskSpaceRequirement.SessionDiskSpace = sessiondiskspace/(1024*1024);
      }
    }

    // Period SessionLifeTime;
    if (bool(resource["SessionLifeTime"]))
      parsed_jobdescription.Resources.SessionLifeTime = Period((std::string)resource["SessionLifeTime"]);

    // SoftwareRequirement CEType;
    if (bool(resource["CEType"])) {
      if (!parseSoftware(resource["CEType"], parsed_jobdescription.Resources.CEType)) {
        return false;
      }
    }

    // NodeAccessType NodeAccess;
    if (lower((std::string)resource["NodeAccess"]) == "inbound")
      parsed_jobdescription.Resources.NodeAccess = NAT_INBOUND;
    else if (lower((std::string)resource["NodeAccess"]) == "outbound")
      parsed_jobdescription.Resources.NodeAccess = NAT_OUTBOUND;
    else if (lower((std::string)resource["NodeAccess"]) == "inoutbound")
      parsed_jobdescription.Resources.NodeAccess = NAT_INOUTBOUND;

    // SlotRequirementType ExclusiveExecution
    if (bool(resource["ExclusiveExecution"])) {
       if (lower((std::string)resource["ExclusiveExecution"]) == "true")
         parsed_jobdescription.Resources.SlotRequirement.ExclusiveExecution = SlotRequirementType::EE_TRUE;
       else if (lower((std::string)resource["ExclusiveExecution"]) == "false")
         parsed_jobdescription.Resources.SlotRequirement.ExclusiveExecution = SlotRequirementType::EE_FALSE;
       else
         parsed_jobdescription.Resources.SlotRequirement.ExclusiveExecution = SlotRequirementType::EE_DEFAULT;
    }

    // ResourceSlotType Slots;
    if (bool(resource["SlotRequirement"]["NumberOfSlots"])) {
      if (!stringto<int>(resource["SlotRequirement"]["NumberOfSlots"], parsed_jobdescription.Resources.SlotRequirement.NumberOfSlots)) {
        parsed_jobdescription.Resources.SlotRequirement.NumberOfSlots = -1;
      }
    }
    else if (bool(xmlXApplication["ProcessCountLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["ProcessCountLimit"], parsed_jobdescription.Resources.SlotRequirement.NumberOfSlots)) {
        parsed_jobdescription.Resources.SlotRequirement.NumberOfSlots = -1;
      }
    }
    if (bool(resource["SlotRequirement"]["ThreadsPerProcesses"])) {
      if (!stringto<int>(resource["SlotRequirement"]["ThreadsPerProcesses"], parsed_jobdescription.Resources.ParallelEnvironment.ThreadsPerProcess)) {
        parsed_jobdescription.Resources.ParallelEnvironment.ThreadsPerProcess = -1;
      }
    }
    else if (bool(xmlXApplication["ThreadCountLimit"])) {
      if (!stringto<int>((std::string)xmlXApplication["ThreadCountLimit"], parsed_jobdescription.Resources.ParallelEnvironment.ThreadsPerProcess)) {
        parsed_jobdescription.Resources.ParallelEnvironment.ThreadsPerProcess = -1;
      }
    }
    if (bool(resource["SlotRequirement"]["ProcessPerHost"])) {
      if (!stringto<int>(resource["SlotRequirement"]["ProcessPerHost"], parsed_jobdescription.Resources.SlotRequirement.SlotsPerHost)) {
        parsed_jobdescription.Resources.SlotRequirement.SlotsPerHost = -1;
      }
    }
    else if (bool(resource["TotalCPUCount"])) {
      Range<int> cpuCount;
      if (!parseRange(resource["TotalCPUCount"], cpuCount)) {
        return false;
      }
      if (cpuCount.min > 0) {
        logger.msg(VERBOSE, "Lower bounded range is not supported for the 'TotalCPUCount' element.");
        return false;
      }
      parsed_jobdescription.Resources.SlotRequirement.SlotsPerHost = cpuCount.max;
    }

    // std::string SPMDVariation;
    //if (bool(resource["SlotRequirement"]["SPMDVariation"]))
    //  parsed_jobdescription.Resources.SlotRequirement.SPMDVariation = (std::string)resource["Slots"]["SPMDVariation"];


    // std::string QueueName;
    if (bool(resource["QueueName"]) ||
        bool(resource["CandidateTarget"]["QueueName"]) // Be backwards compatible
        ) {
      XMLNode xmlQueue = (bool(resource["QueueName"]) ? resource["QueueName"] : resource["CandidateTarget"]["QueueName"]);
      std::string useQueue = (std::string)xmlQueue.Attribute("require");
      if (!useQueue.empty() && useQueue != "eq" && useQueue != "=" && useQueue != "==" && useQueue != "ne" && useQueue != "!=") {
        logger.msg(ERROR, "Parsing the \"require\" attribute of the \"QueueName\" nordugrid-JSDL element failed. An invalid comparison operator was used, only \"ne\" or \"eq\" are allowed.");
        return false;
      }

      if (useQueue == "ne" || useQueue == "!=") {
        parsed_jobdescription.OtherAttributes["nordugrid:broker;reject_queue"] = (std::string)xmlQueue;
      }
      else {
        parsed_jobdescription.Resources.QueueName = (std::string)xmlQueue;
      }
    }

    // SoftwareRequirement RunTimeEnvironment;
    if (bool(resource["RunTimeEnvironment"])) {
      if (!parseSoftware(resource["RunTimeEnvironment"], parsed_jobdescription.Resources.RunTimeEnvironment)) {
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
            return false;
          }
          // add any URL options
          XMLNode option = source["URIOption"];
          for (; (bool)option; ++option) {
            if (!source_url.AddOption((std::string)option, true)) {
              return false;
            }
          }
          // add URL Locations, which may have their own options
          XMLNode location = source["Location"];
          for (; (bool)location; ++location) {
            XMLNode location_uri = location["URI"];
            if (!location_uri) {
              logger.msg(ERROR, "No URI element found in Location for file %s", file.Name);
              return false;
            }
            URLLocation location_url((std::string)location_uri);
            if (!location_url || location_url.Protocol() == "file") {
              logger.msg(ERROR, "Location URI for file %s is invalid", file.Name);
              return false;
            }
            XMLNode loc_option = location["URIOption"];
            for (; (bool)loc_option; ++loc_option) {
              if (!location_url.AddOption((std::string)loc_option, true)) {
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
        parsed_jobdescription.DataStaging.InputFiles.push_back(file);
      }

      if ((bool)filenameNode && bool(target) && bool(target_uri)) {
        OutputFileType file;
        file.Name = (std::string)filenameNode;
        URL target_url((std::string)target_uri);
        if (!target_url) {
          return false;
        }
        // add any URL options
        XMLNode option = target["URIOption"];
        for (; (bool)option; ++option) {
          if (!target_url.AddOption((std::string)option, true)) {
            return false;
          }
        }
        // add URL Locations, which may have their own options
        XMLNode location = target["Location"];
        for (; (bool)location; ++location) {
          XMLNode location_uri = location["URI"];
          if (!location_uri) {
            logger.msg(ERROR, "No URI element found in Location for file %s", file.Name);
            return false;
          }
          URLLocation location_url((std::string)location_uri);
          if (!location_url || location_url.Protocol() == "file") {
            logger.msg(ERROR, "Location URI for file %s is invalid", file.Name);
            return false;
          }
          XMLNode loc_option = location["URIOption"];
          for (; (bool)loc_option; ++loc_option) {
            if (!location_url.AddOption((std::string)loc_option, true)) {
              return false;
            }
          }
          target_url.AddLocation(location_url);
        }
        file.Targets.push_back(target_url);
        parsed_jobdescription.DataStaging.OutputFiles.push_back(file);
      }

      bool deleteOnTermination = (ds["DeleteOnTermination"] && lower(((std::string)ds["DeleteOnTermination"])) == "false");
      if ((bool)filenameNode && (deleteOnTermination || ((bool)target && !target_uri))) {
        // It is allowed by schema for uri not to be present. It is
        // probably safer to assume user wants that file.
        OutputFileType file;
        file.Name = (std::string)filenameNode;
        parsed_jobdescription.DataStaging.OutputFiles.push_back(file);
      }
    }
    // end of Datastaging

    SourceLanguage(parsed_jobdescription) = (!language.empty() ? language : supportedLanguages.front());
    logger.msg(VERBOSE, "String successfully parsed as %s.", parsed_jobdescription.GetSourceLanguage());

    jobdescs.push_back(parsed_jobdescription);
    return true;
  }

  JobDescriptionParserPluginResult ARCJSDLParser::Assemble(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect) const {
    if (!IsLanguageSupported(language)) {
        error = "Language is not supported";
      return false;
    }

    if (job.Application.Executable.Path.empty()) {
      error = "The path of the application's executable is empty.";
      return false;
    }

    NS ns;
    ns[""] = JSDL_NAMESPACE;
    ns["posix-jsdl"] = JSDL_POSIX_NAMESPACE;
    ns["hpcpa-jsdl"] = JSDL_HPCPA_NAMESPACE;
    ns["arc-jsdl"] = JSDL_ARC_NAMESPACE;
    XMLNode jobdefinition(ns, "JobDefinition");

    XMLNode jobdescription = jobdefinition.NewChild("JobDescription");

    // JobIdentification

    // std::string JobName;
    XMLNode xmlIdentification(ns,"JobIdentification");
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
      xmlIdentification.NewChild("arc-jsdl:UserTag") = *it;

    // std::list<std::string> ActivityOldID;
    for (std::list<std::string>::const_iterator it = job.Identification.ActivityOldID.begin();
         it != job.Identification.ActivityOldID.end(); it++)
      xmlIdentification.NewChild("arc-jsdl:ActivityOldId") = *it;

    if (xmlIdentification.Size() > 0)
      jobdescription.NewChild(xmlIdentification);
    // end of JobIdentification

    // Application
    XMLNode xmlApplication(ns,"Application");
    XMLNode xmlPApplication(NS("posix-jsdl", JSDL_POSIX_NAMESPACE), "posix-jsdl:POSIXApplication");
    XMLNode xmlHApplication(NS("hpcpa-jsdl", JSDL_HPCPA_NAMESPACE), "hpcpa-jsdl:HPCProfileApplication");

    // ExecutableType Executable;
    if (!job.Application.Executable.Path.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Executable") = job.Application.Executable.Path;
      xmlHApplication.NewChild("hpcpa-jsdl:Executable") = job.Application.Executable.Path;
      for (std::list<std::string>::const_iterator it = job.Application.Executable.Argument.begin();
           it != job.Application.Executable.Argument.end(); it++) {
        xmlPApplication.NewChild("posix-jsdl:Argument") = *it;
        xmlHApplication.NewChild("hpcpa-jsdl:Argument") = *it;
      }
    }

    // std::string Input;
    if (!job.Application.Input.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Input") = job.Application.Input;
      xmlHApplication.NewChild("hpcpa-jsdl:Input") = job.Application.Input;
    }

    // std::string Output;
    if (!job.Application.Output.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Output") = job.Application.Output;
      xmlHApplication.NewChild("hpcpa-jsdl:Output") = job.Application.Output;
    }

    // std::string Error;
    if (!job.Application.Error.empty()) {
      xmlPApplication.NewChild("posix-jsdl:Error") = job.Application.Error;
      xmlHApplication.NewChild("hpcpa-jsdl:Error") = job.Application.Error;
    }

    // std::list< std::pair<std::string, std::string> > Environment;
    for (std::list< std::pair<std::string, std::string> >::const_iterator it = job.Application.Environment.begin();
         it != job.Application.Environment.end(); it++) {
      XMLNode pEnvironment = xmlPApplication.NewChild("posix-jsdl:Environment");
      XMLNode hEnvironment = xmlHApplication.NewChild("hpcpa-jsdl:Environment");
      pEnvironment.NewAttribute("name") = it->first;
      pEnvironment = it->second;
      hEnvironment.NewAttribute("name") = it->first;
      hEnvironment = it->second;
    }

    // std::list<ExecutableType> PreExecutable;
    if (!job.Application.PreExecutable.empty() && !job.Application.PreExecutable.front().Path.empty()) {
      XMLNode prologue = xmlApplication.NewChild("arc-jsdl:Prologue");
      prologue.NewChild("arc-jsdl:Path") = job.Application.PreExecutable.front().Path;
      for (std::list<std::string>::const_iterator it = job.Application.PreExecutable.front().Argument.begin();
           it != job.Application.PreExecutable.front().Argument.end(); ++it) {
        prologue.NewChild("arc-jsdl:Argument") = *it;
      }
    }

    // std::list<ExecutableType> PostExecutable;
    if (!job.Application.PostExecutable.empty() && !job.Application.PostExecutable.front().Path.empty()) {
      XMLNode epilogue = xmlApplication.NewChild("arc-jsdl:Epilogue");
      epilogue.NewChild("arc-jsdl:Path") = job.Application.PostExecutable.front().Path;
      for (std::list<std::string>::const_iterator it = job.Application.PostExecutable.front().Argument.begin();
           it != job.Application.PostExecutable.front().Argument.end(); ++it)
        epilogue.NewChild("arc-jsdl:Argument") = *it;
    }

    // std::string LogDir;
    if (!job.Application.LogDir.empty())
      xmlApplication.NewChild("arc-jsdl:LogDir") = job.Application.LogDir;

    // std::list<URL> RemoteLogging;
    for (std::list<RemoteLoggingType>::const_iterator it = job.Application.RemoteLogging.begin();
         it != job.Application.RemoteLogging.end(); it++) {
      if (it->ServiceType == "SGAS") {
        xmlApplication.NewChild("arc-jsdl:RemoteLogging") = it->Location.str();
      }
    }

    // int Rerun;
    if (job.Application.Rerun > -1)
      xmlApplication.NewChild("arc-jsdl:Rerun") = tostring(job.Application.Rerun);

    // int Priority;
    if (job.Application.Priority > -1)
      xmlApplication.NewChild("arc-jsdl:Priority") = tostring(job.Application.Priority);

    // Time ExpirationTime;
    if (job.Application.ExpirationTime > -1)
      xmlApplication.NewChild("arc-jsdl:ExpiryTime") = job.Application.ExpirationTime.str();


    // Time ProcessingStartTime;
    if (job.Application.ProcessingStartTime > -1)
      xmlApplication.NewChild("arc-jsdl:ProcessingStartTime") = job.Application.ProcessingStartTime.str();

    // XMLNode Notification;
    for (std::list<NotificationType>::const_iterator it = job.Application.Notification.begin();
         it != job.Application.Notification.end(); it++) {
      XMLNode n = xmlApplication.NewChild("arc-jsdl:Notification");
      n.NewChild("arc-jsdl:Type") = "Email";
      n.NewChild("arc-jsdl:Endpoint") = it->Email;
      for (std::list<std::string>::const_iterator s = it->States.begin();
                  s != it->States.end(); s++) {
        n.NewChild("arc-jsdl:State") = *s;
      }
    }

    // XMLNode AccessControl;
    if (bool(job.Application.AccessControl))
      xmlApplication.NewChild("arc-jsdl:AccessControl").NewChild(job.Application.AccessControl);

    // std::list<URL> CredentialService;
    for (std::list<URL>::const_iterator it = job.Application.CredentialService.begin();
         it != job.Application.CredentialService.end(); it++)
      xmlApplication.NewChild("arc-jsdl:CredentialService") = it->fullstr();

    if (job.Application.DryRun) {
      xmlApplication.NewChild("arc-jsdl:DryRun") = "yes";
    }

    // POSIX compliance...
    if (job.Resources.TotalWallTime.range.max != -1)
      xmlPApplication.NewChild("posix-jsdl:WallTimeLimit") = tostring(job.Resources.TotalWallTime.range.max);
    if (job.Resources.IndividualPhysicalMemory.max != -1) {
      xmlPApplication.NewChild("posix-jsdl:MemoryLimit") = tostring(job.Resources.IndividualPhysicalMemory.max*1024*1024);
    }
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
    XMLNode xmlResources(ns,"Resources");

    // SoftwareRequirement OperatingSystem
    if (!job.Resources.OperatingSystem.empty()) {
      XMLNode xmlOS = xmlResources.NewChild("OperatingSystem");

      outputSoftware(job.Resources.OperatingSystem, xmlOS);

      // JSDL compliance. Only the first element in the OperatingSystem object is printed.
      xmlOS.NewChild("OperatingSystemType").NewChild("OperatingSystemName") = job.Resources.OperatingSystem.getSoftwareList().front().getName();
      if (!job.Resources.OperatingSystem.getSoftwareList().front().getVersion().empty())
        xmlOS.NewChild("OperatingSystemVersion") = job.Resources.OperatingSystem.getSoftwareList().front().getVersion();
    }

    // std::string Platform;
    if (!job.Resources.Platform.empty()) {
      xmlResources.NewChild("arc-jsdl:Platform") = job.Resources.Platform;
      // JSDL compliance
      xmlResources.NewChild("CPUArchitecture").NewChild("CPUArchitectureName") = job.Resources.Platform;
    }

    // std::string NetworkInfo;
    if (!job.Resources.NetworkInfo.empty()) {
      xmlResources.NewChild("arc-jsdl:NetworkInfo") = job.Resources.NetworkInfo;

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
      xmlResources.NewChild("arc-jsdl:NodeAccess") = "inbound";
      break;
    case NAT_OUTBOUND:
      xmlResources.NewChild("arc-jsdl:NodeAccess") = "outbound";
      break;
    case NAT_INOUTBOUND:
      xmlResources.NewChild("arc-jsdl:NodeAccess") = "inoutbound";
      break;
    }

    // Range<int> IndividualPhysicalMemory;
    {
      XMLNode xmlIPM(ns,"IndividualPhysicalMemory");
      outputARCJSDLRange(job.Resources.IndividualPhysicalMemory, xmlIPM, (int)-1);
      // JSDL compliance...
      outputJSDLRange(job.Resources.IndividualPhysicalMemory, xmlIPM, (int)-1);
      if (xmlIPM.Size() > 0)
        xmlResources.NewChild(xmlIPM);
    }

    // Range<int> IndividualVirtualMemory;
    {
      XMLNode xmlIVM(ns,"IndividualVirtualMemory");
      outputARCJSDLRange(job.Resources.IndividualVirtualMemory, xmlIVM, (int)-1);
      outputJSDLRange(job.Resources.IndividualVirtualMemory, xmlIVM, (int)-1);
      if (xmlIVM.Size() > 0)
        xmlResources.NewChild(xmlIVM);
    }

    {
      // Range<int> DiskSpace;
      XMLNode xmlDiskSpace(ns,"arc-jsdl:DiskSpace");
      XMLNode xmlFileSystem(ns,"DiskSpace"); // JDSL compliance...
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
        XMLNode dsr = xmlResources.NewChild("arc-jsdl:DiskSpaceRequirement");
        dsr.NewChild(xmlDiskSpace);

        // int CacheDiskSpace;
        if (job.Resources.DiskSpaceRequirement.CacheDiskSpace > -1) {
          dsr.NewChild("arc-jsdl:CacheDiskSpace") = tostring(job.Resources.DiskSpaceRequirement.CacheDiskSpace*1024*1024);
        }

        // int SessionDiskSpace;
        if (job.Resources.DiskSpaceRequirement.SessionDiskSpace > -1) {
          dsr.NewChild("arc-jsdl:SessionDiskSpace") = tostring(job.Resources.DiskSpaceRequirement.SessionDiskSpace*1024*1024);
        }
      }

      // JSDL Compliance...
      if (xmlFileSystem.Size() > 0) {
        xmlResources.NewChild("FileSystem").NewChild(xmlFileSystem);
      }
    }

    // Period SessionLifeTime;
    if (job.Resources.SessionLifeTime > -1)
      xmlResources.NewChild("arc-jsdl:SessionLifeTime") = tostring(job.Resources.SessionLifeTime);

    // ScalableTime<int> IndividualCPUTime;
    {
      XMLNode xmlICPUT(ns,"IndividualCPUTime");
      XMLNode xmlValue = xmlICPUT.NewChild("arc-jsdl:Value");
      outputARCJSDLRange(job.Resources.IndividualCPUTime.range, xmlValue, -1);
      if (xmlValue.Size() > 0) {
        outputBenchmark(job.Resources.IndividualCPUTime.benchmark, xmlICPUT);
        // JSDL compliance...
        outputJSDLRange(job.Resources.IndividualCPUTime.range, xmlICPUT, -1);

        xmlResources.NewChild(xmlICPUT);
      }
    }

    // ScalableTime<int> TotalCPUTime;
    {
      XMLNode xmlTCPUT(ns,"TotalCPUTime");
      XMLNode xmlValue = xmlTCPUT.NewChild("arc-jsdl:Value");
      outputARCJSDLRange(job.Resources.TotalCPUTime.range, xmlValue, -1);
      if (xmlValue.Size() > 0) {
        outputBenchmark(job.Resources.TotalCPUTime.benchmark, xmlTCPUT);
        // JSDL compliance...
        outputJSDLRange(job.Resources.TotalCPUTime.range, xmlTCPUT, -1);

        xmlResources.NewChild(xmlTCPUT);
      }
    }

    // ScalableTime<int> IndividualWallTime;
    {
      XMLNode xmlIWT(ns,"arc-jsdl:IndividualWallTime");
      XMLNode xmlValue = xmlIWT.NewChild("arc-jsdl:Value");
      outputARCJSDLRange(job.Resources.IndividualWallTime.range, xmlValue, -1);
      if (xmlValue.Size() > 0) {
        outputBenchmark(job.Resources.IndividualWallTime.benchmark, xmlIWT);
        xmlResources.NewChild(xmlIWT);
      }
    }

    // ScalableTime<int> TotalWallTime;
    {
      XMLNode xmlTWT("arc-jsdl:TotalWallTime");
      XMLNode xmlValue = xmlTWT.NewChild("arc-jsdl:Value");
      outputARCJSDLRange(job.Resources.TotalWallTime.range, xmlValue, -1);
      if (xmlValue.Size() > 0) {
        outputBenchmark(job.Resources.TotalWallTime.benchmark, xmlTWT);
        xmlResources.NewChild(xmlTWT);
      }
    }

    // SoftwareRequirement CEType;
    if (!job.Resources.CEType.empty()) {
      XMLNode xmlCEType = xmlResources.NewChild("arc-jsdl:CEType");
      outputSoftware(job.Resources.CEType, xmlCEType);
    }

    // ResourceSlotType Slots;
    {
      XMLNode xmlSlotRequirement(ns,"arc-jsdl:SlotRequirement");

      // Range<int> NumberOfSlots;
      if (job.Resources.SlotRequirement.NumberOfSlots > -1) {
        xmlSlotRequirement.NewChild("arc-jsdl:NumberOfSlots") = tostring(job.Resources.SlotRequirement.NumberOfSlots);
      }

      // int ProcessPerHost;
      if (job.Resources.SlotRequirement.SlotsPerHost > -1) {
        xmlSlotRequirement.NewChild("arc-jsdl:ProcessPerHost") = tostring(job.Resources.SlotRequirement.SlotsPerHost);
        xmlResources.NewChild("TotalCPUCount") = tostring(job.Resources.SlotRequirement.SlotsPerHost);
      }

      // int ThreadsPerProcess;
      if (job.Resources.ParallelEnvironment.ThreadsPerProcess > -1) {
        xmlSlotRequirement.NewChild("arc-jsdl:ThreadsPerProcesses") = tostring(job.Resources.ParallelEnvironment.ThreadsPerProcess);
      }

      if (!job.Resources.ParallelEnvironment.Type.empty()) {
        xmlSlotRequirement.NewChild("arc-jsdl:SPMDVariation") = job.Resources.ParallelEnvironment.Type;
      }

      if (job.Resources.SlotRequirement.ExclusiveExecution != SlotRequirementType::EE_DEFAULT) {
         if (job.Resources.SlotRequirement.ExclusiveExecution == SlotRequirementType::EE_TRUE) {
           xmlSlotRequirement.NewChild("arc-jsdl:ExclusiveExecution") = "true";
           xmlResources.NewChild("ExclusiveExecution") = "true"; // JSDL
         }
         else if (job.Resources.SlotRequirement.ExclusiveExecution == SlotRequirementType::EE_FALSE) {
           xmlSlotRequirement.NewChild("arc-jsdl:ExclusiveExecution") = "false";
           xmlResources.NewChild("ExclusiveExecution") = "false";
         }
      }

      if (xmlSlotRequirement.Size() > 0) {
        xmlResources.NewChild(xmlSlotRequirement);
      }
    }

    // std::string QueueName;
    if (!job.Resources.QueueName.empty()) {;
      //logger.msg(INFO, "job.Resources.QueueName = %s", job.Resources.QueueName);
      xmlResources.NewChild("arc-jsdl:QueueName") = job.Resources.QueueName;

      // Be backwards compatible with NOX versions of A-REX.
      xmlResources.NewChild("arc-jsdl:CandidateTarget").NewChild("arc-jsdl:QueueName") = job.Resources.QueueName;
    }

    // SoftwareRequirement RunTimeEnvironment;
    if (!job.Resources.RunTimeEnvironment.empty()) {
      XMLNode xmlRTE = xmlResources.NewChild("arc-jsdl:RunTimeEnvironment");
      outputSoftware(job.Resources.RunTimeEnvironment, xmlRTE);
    }

    if (xmlResources.Size() > 0)
      jobdescription.NewChild(xmlResources);

    // end of Resources

    // DataStaging
    for (std::list<InputFileType>::const_iterator it = job.DataStaging.InputFiles.begin();
         it != job.DataStaging.InputFiles.end(); ++it) {
      if (it->Name.empty()) {
        error = "The name of the input file is empty.";
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
            xs.NewChild("arc-jsdl:URIOption") = itOpt->first + "=" + itOpt->second;
          }
          // add URL Locations, which may have their own options
          for (std::list<URLLocation>::const_iterator itLoc = it->Sources.front().Locations().begin();
               itLoc != it->Sources.front().Locations().end(); ++itLoc) {
            XMLNode xloc = xs.NewChild("arc-jsdl:Location");
            xloc.NewChild("URI") = itLoc->str();

            for (std::multimap<std::string, std::string>::const_iterator itOpt = itLoc->Options().begin();
                 itOpt != itLoc->Options().end(); ++itOpt) {
              xloc.NewChild("arc-jsdl:URIOption") = itOpt->first + "=" + itOpt->second;
            }
          }
        }
      }
      if (it->IsExecutable) {
        datastaging.NewChild("arc-jsdl:IsExecutable") = "true";
      }
      if (it->FileSize > -1) {
        datastaging.NewChild("arc-jsdl:FileSize") = tostring(it->FileSize);
      }
      if (!it->Checksum.empty()) {
        datastaging.NewChild("arc-jsdl:Checksum") = it->Checksum;
      }
    }

    for (std::list<OutputFileType>::const_iterator it = job.DataStaging.OutputFiles.begin();
         it != job.DataStaging.OutputFiles.end(); ++it) {
      if (it->Name.empty()) {
        error = "The name of the output file is empty.";
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
          xs.NewChild("arc-jsdl:URIOption") = itOpt->first + "=" + itOpt->second;
        }
        // add URL Locations, which may have their own options
        for (std::list<URLLocation>::const_iterator itLoc = it->Targets.front().Locations().begin();
             itLoc != it->Targets.front().Locations().end(); ++itLoc) {
          XMLNode xloc = xs.NewChild("arc-jsdl:Location");
          xloc.NewChild("URI") = itLoc->str();

          for (std::multimap<std::string, std::string>::const_iterator itOpt = itLoc->Options().begin();
               itOpt != itLoc->Options().end(); ++itOpt) {
            xloc.NewChild("arc-jsdl:URIOption") = itOpt->first + "=" + itOpt->second;
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
