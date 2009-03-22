// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/JobDescription.h>

#include "JSDLParser.h"

namespace Arc {

  JSDLParser::JSDLParser()
    : JobDescriptionParser() {}

  JSDLParser::~JSDLParser() {}

  // Optional name attribute checker.
  // When the node has "isoptional" attribute and it is "true",
  // then add this node name to the list of OptionalElement.
  void Optional_attribute_check(XMLNode node, std::string path,
                                JobDescription& job,
                                OptionalElementType& optional_element,
                                std::string value = "") {
    if (lower((std::string)node.Attribute("isoptional")) == "true") {
      optional_element.Name = node.Name();
      optional_element.Path = path;
      optional_element.Value = value;
      job.OptionalElement.push_back(optional_element);
    }
  }

  JobDescription JSDLParser::Parse(const std::string& source) const {
    //Source checking
    XMLNode node(source);
    if (node.Size() == 0) {
      logger.msg(DEBUG, "[JSDLParser] Wrong XML structure! ");
      return JobDescription();
    }

    // The source parsing start now.
    NS nsList;
    nsList.insert(std::pair<std::string, std::string>("jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl"));
    //TODO: wath is the new GIN namespace  (it is now temporary namespace)
    //nsList.insert(std::pair<std::string, std::string>("gin","http://"));

    //Meta
    XMLNode meta;
    OptionalElementType optional;

    JobDescription job;

    meta = node["Meta"]["Author"];
    if (bool(meta) && (std::string)(meta) != "") {
      Optional_attribute_check(meta, "//Meta/Author", job, optional);
      job.Author = (std::string)(meta);
    }
    meta.Destroy();

    meta = node["Meta"]["DocumentExpiration"];
    if (bool(meta) && (std::string)(meta) != "") {
      Optional_attribute_check(meta, "//Meta/DocumentExpiration", job, optional);
      Time time((std::string)(meta));
      job.DocumentExpiration = time;
    }
    meta.Destroy();

    meta = node["Meta"]["Rank"];
    if (bool(meta) && (std::string)(meta) != "") {
      Optional_attribute_check(meta, "//Meta/Rank", job, optional);
      job.Rank = (std::string)(meta);
    }
    meta.Destroy();

    meta = node["Meta"]["FuzzyRank"];
    if (bool(meta) && (std::string)(meta) != "") {
      Optional_attribute_check(meta, "//Meta/FuzzyRank", job, optional);
      if ((std::string)(meta) == "true")
        job.FuzzyRank = true;
      else if ((std::string)(meta) == "false")
        job.FuzzyRank = false;
      else {
        logger.msg(DEBUG, "Invalid \"/Meta/FuzzyRank\" value: %s", (std::string)(meta));
        return JobDescription();
      }
    }
    meta.Destroy();

    //Application
    XMLNode application;
    application = node["JobDescription"]["Application"]["Executable"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/Executable", job, optional);
      job.Executable = (std::string)(application);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["LogDir"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/LogDir", job, optional);
      job.LogDir = (std::string)(application);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Argument"];
    for (int i = 0; bool(application[i]) && (std::string)(application[i]) != ""; i++) {
      Optional_attribute_check(application[i], "//JobDescription/Application/Argument", job, optional, (std::string)(application[i]));
      job.Argument.push_back((std::string)(application[i]));
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Input"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/Input", job, optional);
      job.Input = (std::string)(application);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Output"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/Output", job, optional);
      job.Output = (std::string)(application);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Error"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/Error", job, optional);
      job.Error = (std::string)(application);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["RemoteLogging"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/RemoteLogging", job, optional);
      URL url((std::string)(application));
      job.RemoteLogging = url;
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Environment"];
    for (int i = 0; bool(application[i]) && (std::string)(application[i]) != ""; i++) {
      Optional_attribute_check(application[i], "//JobDescription/Application/Environment", job, optional, (std::string)(application[i]));
      EnvironmentType env;
      env.name_attribute = (std::string)(application[i].Attribute("name"));
      env.value = (std::string)(application[i]);
      job.Environment.push_back(env);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["LRMSReRun"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/LRMSReRun", job, optional);
      job.LRMSReRun = stringtoi(application);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Prologue"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/Prologue", job, optional);
      std::string source_prologue = (std::string)(application);
      std::vector<std::string> parts;
      tokenize(source_prologue, parts);
      if (!parts.empty()) {
        XLogueType _prologue;
        _prologue.Name = parts[0];
        for (std::vector<std::string>::const_iterator it = ++parts.begin(); it != parts.end(); it++)
          _prologue.Arguments.push_back(*it);
        job.Prologue = _prologue;
      }
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Epilogue"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/Epilogue", job, optional);
      std::string source_epilogue = (std::string)(application);
      std::vector<std::string> parts;
      tokenize(source_epilogue, parts);
      if (!parts.empty()) {
        XLogueType _epilogue;
        _epilogue.Name = parts[0];
        for (std::vector<std::string>::const_iterator it = ++parts.begin(); it != parts.end(); it++)
          _epilogue.Arguments.push_back(*it);
        job.Epilogue = _epilogue;
      }
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["SessionLifeTime"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/SessionLifeTime", job, optional);
      Period time((std::string)(application));
      job.SessionLifeTime = time;
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["AccessControl"];
    if (bool(application)) {
      Optional_attribute_check(application, "//JobDescription/Application/AccessControl", job, optional);
      ((XMLNode)(application)).Child(0).New(job.AccessControl);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["ProcessingStartTime"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/ProcessingStartTime", job, optional);
      Time stime((std::string)application);
      job.ProcessingStartTime = stime;
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Notification"];
    for (int i = 0; bool(application[i]); i++) {
      Optional_attribute_check(application[i], "//JobDescription/Application/Notification", job, optional);
      NotificationType ntype;
      XMLNodeList address = (application[i]).XPathLookup((std::string)"//Address", nsList);
      for (std::list<XMLNode>::iterator it_addr = address.begin(); it_addr != address.end(); it_addr++)
        ntype.Address.push_back((std::string)(*it_addr));
      XMLNodeList state = (application[i]).XPathLookup((std::string)"//State", nsList);
      for (std::list<XMLNode>::iterator it_state = state.begin(); it_state != state.end(); it_state++)
        ntype.State.push_back((std::string)(*it_state));
      job.Notification.push_back(ntype);
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["CredentialService"];
    if (bool(application) && (std::string)(application) != "") {
      Optional_attribute_check(application, "//JobDescription/Application/CredentialService", job, optional);
      URL curl((std::string)application);
      job.CredentialService = curl;
    }
    application.Destroy();

    application = node["JobDescription"]["Application"]["Join"];
    if (bool(application) && upper((std::string)application) == "TRUE") {
      Optional_attribute_check(application, "//JobDescription/Application/Join", job, optional);
      job.Join = true;
    }
    application.Destroy();

    //JobIdentification
    XMLNode jobidentification;
    jobidentification = node["JobDescription"]["JobIdentification"]["JobName"];
    if (bool(jobidentification) && (std::string)(jobidentification) != "") {
      Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobName", job, optional);
      job.JobName = (std::string)(jobidentification);
    }
    jobidentification.Destroy();

    jobidentification = node["JobDescription"]["JobIdentification"]["Description"];
    if (bool(jobidentification) && (std::string)(jobidentification) != "") {
      Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/Description", job, optional);
      job.Description = (std::string)(jobidentification);
    }
    jobidentification.Destroy();

    jobidentification = node["JobDescription"]["JobIdentification"]["JobProject"];
    if (bool(jobidentification) && (std::string)(jobidentification) != "") {
      Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobProject", job, optional);
      job.JobProject = (std::string)(jobidentification);
    }
    jobidentification.Destroy();

    jobidentification = node["JobDescription"]["JobIdentification"]["JobType"];
    if (bool(jobidentification) && (std::string)(jobidentification) != "") {
      Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobType", job, optional);
      job.JobType = (std::string)(jobidentification);
    }
    jobidentification.Destroy();

    jobidentification = node["JobDescription"]["JobIdentification"]["JobCategory"];
    if (bool(jobidentification) && (std::string)(jobidentification) != "") {
      Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobCategory", job, optional);
      job.JobCategory = (std::string)(jobidentification);
    }
    jobidentification.Destroy();

    jobidentification = node["JobDescription"]["JobIdentification"]["UserTag"];
    for (int i = 0; bool(jobidentification[i]) && (std::string)(jobidentification[i]) != ""; i++) {
      Optional_attribute_check(jobidentification[i], "//JobDescription/JobIdentification/UserTag", job, optional, (std::string)jobidentification[i]);
      job.UserTag.push_back((std::string)jobidentification[i]);
    }
    jobidentification.Destroy();

    //Resource
    XMLNode resource;
    resource = node["JobDescription"]["Resource"]["TotalCPUTime"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/TotalCPUTime", job, optional);
      Period time((std::string)(resource));
      job.TotalCPUTime = time;
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["IndividualCPUTime"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/IndividualCPUTime", job, optional);
      Period time((std::string)(resource));
      job.IndividualCPUTime = time;
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["TotalWallTime"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/TotalWallTime", job, optional);
      Period _walltime((std::string)(resource));
      job.TotalWallTime = _walltime;
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["IndividualWallTime"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/IndividualWallTime", job, optional);
      Period _indwalltime((std::string)(resource));
      job.IndividualWallTime = _indwalltime;
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["ReferenceTime"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/ReferenceTime", job, optional);
      ReferenceTimeType _reference;
      _reference.time = (std::string)(resource);
      _reference.benchmark = (std::string)resource.Attribute("benchmark");
      _reference.value = stringtod((std::string)resource.Attribute("value"));
      job.ReferenceTime.push_back(_reference);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["ExclusiveExecution"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/ExclusiveExecution", job, optional);
      ReferenceTimeType _reference;
      if (upper((std::string)(resource)) == "TRUE")
        job.ExclusiveExecution = true;
      else if (upper((std::string)(resource)) == "FALSE")
        job.ExclusiveExecution = false;
      else {
        logger.msg(DEBUG, "Invalid \"/JobDescription/Resource/ExclusiveExecution\" value: %s", (std::string)(resource));
        return JobDescription();
      }
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["NetworkInfo"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/NetworkInfo", job, optional);
      job.NetworkInfo = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["OSFamily"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/OSFamily", job, optional);
      job.OSFamily = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["OSName"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/OSName", job, optional);
      job.OSName = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["OSVersion"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/OSVersion", job, optional);
      job.OSVersion = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Platform"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Platform", job, optional);
      job.Platform = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["IndividualPhysicalMemory"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/IndividualPhysicalMemory", job, optional);
      job.IndividualPhysicalMemory = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["IndividualVirtualMemory"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/IndividualVirtualMemory", job, optional);
      job.IndividualVirtualMemory = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["DiskSpace"]["IndividualDiskSpace"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace/IndividualDiskSpace", job, optional);
      job.IndividualDiskSpace = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["DiskSpace"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace", job, optional);
      job.DiskSpace = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["DiskSpace"]["CacheDiskSpace"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace/CacheDiskSpace", job, optional);
      job.CacheDiskSpace = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["DiskSpace"]["SessionDiskSpace"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace/SessionDiskSpace", job, optional);
      job.SessionDiskSpace = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["CandidateTarget"]["Alias"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/CandidateTarget/Alias", job, optional);
      job.Alias = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["CandidateTarget"]["EndPointURL"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/CandidateTarget/EndPointURL", job, optional);
      URL url((std::string)(resource));
      job.EndPointURL = url;
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["CandidateTarget"]["QueueName"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/CandidateTarget/QueueName", job, optional);
      job.QueueName = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Location"]["Country"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Location/Country", job, optional);
      job.Country = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Location"]["Place"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Location/Place", job, optional);
      job.Place = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Location"]["PostCode"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Location/PostCode", job, optional);
      job.PostCode = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Location"]["Latitude"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Location/Latitude", job, optional);
      job.Latitude = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Location"]["Longitude"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Location/Longitude", job, optional);
      job.Longitude = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["CEType"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/CEType", job, optional);
      job.CEType = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Slots"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Slots", job, optional);
      job.Slots = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Slots"]["ProcessPerHost"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Slots/ProcessPerHost", job, optional);
      job.ProcessPerHost = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Slots"]["NumberOfProcesses"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Slots/NumberOfProcesses", job, optional);
      job.NumberOfProcesses = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Slots"]["ThreadPerProcesses"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Slots/ThreadPerProcesses", job, optional);
      job.ThreadPerProcesses = stringtoi(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Slots"]["SPMDVariation"];
    if (bool(resource) && (std::string)(resource) != "") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Slots/SPMDVariation", job, optional);
      job.SPMDVariation = (std::string)(resource);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["RunTimeEnvironment"];
    for (int i = 0; bool(resource[i]); i++) {
      //            Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/RunTimeEnvironment", job,optional, (std::string)(*it));
      RunTimeEnvironmentType _runtimeenv;
      if ((bool)(resource[i]["Name"])) {
        _runtimeenv.Name = (std::string)resource[i]["Name"];
        Optional_attribute_check(resource[i]["Name"], "//JobDescription/Resource/RunTimeEnvironment/Name", job, optional, (std::string)(resource[i]["Name"]));
      }
      else {
        logger.msg(DEBUG, "Not found  \"/JobDescription/Resource/RunTimeEnvironment/Name\" element");
        return JobDescription();
      }
      XMLNodeList version = resource[i].XPathLookup((std::string)"//Version", nsList);
      for (std::list<XMLNode>::const_iterator it_v = version.begin(); it_v != version.end(); it_v++) {
        Optional_attribute_check(*it_v, "//JobDescription/Resource/RunTimeEnvironment/Version", job, optional, (std::string)(*it_v));
        _runtimeenv.Version.push_back((std::string)(*it_v));
      }
      job.RunTimeEnvironment.push_back(_runtimeenv);
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["Homogeneous"];
    if (bool(resource) && (std::string)(resource) == "true") {
      Optional_attribute_check(resource, "//JobDescription/Resource/Homogeneous", job, optional);
      job.Homogeneous = true;
    }
    else {
      Optional_attribute_check(resource, "//JobDescription/Resource/Homogeneous", job, optional);
      job.Homogeneous = false;
    }
    resource.Destroy();

    resource = node["JobDescription"]["Resource"]["NodeAccess"];
    if ((bool)((XMLNode)(resource)["InBound"]) &&
        (std::string)((XMLNode)(resource)["InBound"]) == "true")
      job.InBound = true;
    if ((bool)((XMLNode)(resource)["OutBound"]) &&
        (std::string)((XMLNode)(resource)["OutBound"]) == "true")
      job.OutBound = true;
    resource.Destroy();


    //DataStaging
    XMLNode datastaging;
    datastaging = node["JobDescription"]["DataStaging"]["File"];
    for (int i = 0; bool(datastaging[i]); i++) {
      //           Optional_attribute_check(datastaging[i], "//JobDescription/DataStaging/File", job,optional);
      FileType _file;
      if ((bool)(datastaging[i]["Name"]))
        _file.Name = (std::string)datastaging[i]["Name"];
      XMLNodeList source = datastaging[i].XPathLookup((std::string)"//Source", nsList);
      for (std::list<XMLNode>::const_iterator it_source = source.begin(); it_source != source.end(); it_source++) {
        SourceType _source;
        if ((bool)((*it_source)["URI"]))
          _source.URI = (std::string)(*it_source)["URI"];
        if ((bool)((*it_source)["Threads"]))
          _source.Threads = stringtoi((std::string)(*it_source)["Threads"]);
        else
          _source.Threads = -1;
        _file.Source.push_back(_source);
      }
      XMLNodeList target = datastaging[i].XPathLookup((std::string)"//Target", nsList);
      for (std::list<XMLNode>::const_iterator it_target = target.begin(); it_target != target.end(); it_target++) {
        TargetType _target;
        if ((bool)((*it_target)["URI"]))
          _target.URI = (std::string)(*it_target)["URI"];
        if ((bool)((*it_target)["Threads"]))
          _target.Threads = stringtoi((std::string)(*it_target)["Threads"]);
        else
          _target.Threads = -1;
        if ((bool)((*it_target)["Mandatory"])) {
          if ((std::string)(*it_target)["Mandatory"] == "true")
            _target.Mandatory = true;
          else if ((std::string)(*it_target)["Mandatory"] == "false")
            _target.Mandatory = false;
          else
            logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/Target/Mandatory\" value: %s", (std::string)(*it_target)["Mandatory"]);
        }
        else
          _target.Mandatory = false;
        if ((bool)((*it_target)["NeededReplicas"]))
          _target.NeededReplicas = stringtoi((std::string)(*it_target)["NeededReplicas"]);
        else
          _target.NeededReplicas = -1;
        _file.Target.push_back(_target);
      }
      if ((bool)(datastaging[i]["KeepData"])) {
        if ((std::string)datastaging[i]["KeepData"] == "true")
          _file.KeepData = true;
        else if ((std::string)datastaging[i]["KeepData"] == "false")
          _file.KeepData = false;
        else
          logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/KeepData\" value: %s", (std::string)datastaging[i]["KeepData"]);
      }
      else
        _file.KeepData = false;
      if ((bool)(datastaging[i]["IsExecutable"])) {
        if ((std::string)datastaging[i]["IsExecutable"] == "true")
          _file.IsExecutable = true;
        else if ((std::string)datastaging[i]["IsExecutable"] == "false")
          _file.IsExecutable = false;
        else
          logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/IsExecutable\" value: %s", (std::string)datastaging[i]["IsExecutable"]);
      }
      else
        _file.IsExecutable = false;
      if ((bool)(datastaging[i]["DataIndexingService"])) {
        URL uri((std::string)(datastaging[i]["DataIndexingService"]));
        _file.DataIndexingService = uri;
      }
      if ((bool)(datastaging[i]["DownloadToCache"])) {
        if ((std::string)datastaging[i]["DownloadToCache"] == "true")
          _file.DownloadToCache = true;
        else if ((std::string)datastaging[i]["DownloadToCache"] == "false")
          _file.DownloadToCache = false;
        else
          logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/DownloadToCache\" value: %s", (std::string)datastaging[i]["DownloadToCache"]);
      }
      else
        _file.DownloadToCache = false;
      job.File.push_back(_file);
    }
    datastaging.Destroy();

    datastaging = node["JobDescription"]["DataStaging"]["Directory"];
    for (int i = 0; bool(datastaging[i]); i++) {
      //           Optional_attribute_check(datastaging[i], "//JobDescription/DataStaging/Directory", job,optional);
      DirectoryType _directory;
      if ((bool)(datastaging[i]["Name"]))
        _directory.Name = (std::string)datastaging[i]["Name"];
      XMLNodeList source = datastaging[i].XPathLookup((std::string)"//Source", nsList);
      for (std::list<XMLNode>::const_iterator it_source = source.begin(); it_source != source.end(); it_source++) {
        SourceType _source;
        if ((bool)((*it_source)["URI"]))
          _source.URI = (std::string)(*it_source)["URI"];
        if ((bool)((*it_source)["Threads"]))
          _source.Threads = stringtoi((std::string)(*it_source)["Threads"]);
        else
          _source.Threads = -1;
        _directory.Source.push_back(_source);
      }
      XMLNodeList target = datastaging[i].XPathLookup((std::string)"//Target", nsList);
      for (std::list<XMLNode>::const_iterator it_target = target.begin(); it_target != target.end(); it_target++) {
        TargetType _target;
        if ((bool)((*it_target)["URI"]))
          _target.URI = (std::string)(*it_target)["URI"];
        if ((bool)((*it_target)["Threads"]))
          _target.Threads = stringtoi((std::string)(*it_target)["Threads"]);
        else
          _target.Threads = -1;
        if ((bool)((*it_target)["Mandatory"])) {
          if ((std::string)(*it_target)["Mandatory"] == "true")
            _target.Mandatory = true;
          else if ((std::string)(*it_target)["Mandatory"] == "false")
            _target.Mandatory = false;
          else
            logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/Target/Mandatory\" value: %s", (std::string)(*it_target)["Mandatory"]);
        }
        else
          _target.Mandatory = false;
        if ((bool)((*it_target)["NeededReplicas"]))
          _target.NeededReplicas = stringtoi((std::string)(*it_target)["NeededReplicas"]);
        else
          _target.NeededReplicas = -1;
        _directory.Target.push_back(_target);
      }
      if ((bool)(datastaging[i]["KeepData"])) {
        if ((std::string)datastaging[i]["KeepData"] == "true")
          _directory.KeepData = true;
        else if ((std::string)datastaging[i]["KeepData"] == "false")
          _directory.KeepData = false;
        else
          logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/KeepData\" value: %s", (std::string)datastaging[i]["KeepData"]);
      }
      else
        _directory.KeepData = false;
      if ((bool)(datastaging[i]["IsExecutable"])) {
        if ((std::string)datastaging[i]["IsExecutable"] == "true")
          _directory.IsExecutable = true;
        else if ((std::string)datastaging[i]["IsExecutable"] == "false")
          _directory.IsExecutable = false;
        else
          logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/IsExecutable\" value: %s", (std::string)datastaging[i]["IsExecutable"]);
      }
      else
        _directory.IsExecutable = false;
      if ((bool)(datastaging[i]["DataIndexingService"])) {
        URL uri((std::string)(datastaging[i]["DataIndexingService"]));
        _directory.DataIndexingService = uri;
      }
      if ((bool)(datastaging[i]["DownloadToCache"])) {
        if ((std::string)datastaging[i]["DownloadToCache"] == "true")
          _directory.DownloadToCache = true;
        else if ((std::string)datastaging[i]["DownloadToCache"] == "false")
          _directory.DownloadToCache = false;
        else
          logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/DownloadToCache\" value: %s", (std::string)datastaging[i]["DownloadToCache"]);
      }
      else
        _directory.DownloadToCache = false;
      job.Directory.push_back(_directory);
    }
    datastaging.Destroy();

    return job;
  }

  std::string JSDLParser::UnParse(const JobDescription& job) const {

    NS gin_namespaces;
    gin_namespaces["jsdl-gin"] = "http://schemas.ogf.org/gin-profile/2008/11/jsdl";
    gin_namespaces["targetNamespace"] = "http://schemas.ogf.org/gin-profile/2008/11/jsdl";
    gin_namespaces["elementFormDefault"] = "qualified";
    XMLNode jobTree(gin_namespaces, "JobDefinition");

    //obligatory elements: Executable, LogDir
    if (!bool(jobTree["JobDescription"]))
      jobTree.NewChild("JobDescription");
    if (!bool(jobTree["JobDescription"]["Application"]))
      jobTree["JobDescription"].NewChild("Application");
    if (!bool(jobTree["JobDescription"]["Application"]["Executable"]))
      jobTree["JobDescription"]["Application"].NewChild("Executable") = job.Executable;
    if (!bool(jobTree["JobDescription"]["Application"]["LogDir"]))
      jobTree["JobDescription"]["Application"].NewChild("LogDir") = job.LogDir;

    //optional elements
    //JobIdentification
    if (!job.JobName.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]))
        jobTree["JobDescription"].NewChild("JobIdentification");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]["JobName"]))
        jobTree["JobDescription"]["JobIdentification"].NewChild("JobName") = job.JobName;
    }
    if (!job.Description.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]))
        jobTree["JobDescription"].NewChild("JobIdentification");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]["Description"]))
        jobTree["JobDescription"]["JobIdentification"].NewChild("Description") = job.Description;
    }
    if (!job.JobProject.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]))
        jobTree["JobDescription"].NewChild("JobIdentification");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]["JobProject"]))
        jobTree["JobDescription"]["JobIdentification"].NewChild("JobProject") = job.JobProject;
    }
    if (!job.JobType.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]))
        jobTree["JobDescription"].NewChild("JobIdentification");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]["JobType"]))
        jobTree["JobDescription"]["JobIdentification"].NewChild("JobType") = job.JobType;
    }
    if (!job.JobCategory.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]))
        jobTree["JobDescription"].NewChild("JobIdentification");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]["JobCategory"]))
        jobTree["JobDescription"]["JobIdentification"].NewChild("JobCategory") = job.JobCategory;
    }
    if (!job.UserTag.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["JobIdentification"]))
        jobTree["JobDescription"].NewChild("JobIdentification");
      for (std::list<std::string>::const_iterator it = job.UserTag.begin();
           it != job.UserTag.end(); it++) {
        XMLNode attribute = jobTree["JobDescription"]["JobIdentification"].NewChild("UserTag");
        attribute = (*it);
      }
    }

    //Meta
    if (!job.Author.empty()) {
      if (!bool(jobTree["Meta"]))
        jobTree.NewChild("Meta");
      if (!bool(jobTree["Meta"]["Author"]))
        jobTree["Meta"].NewChild("Author") = job.Author;
    }
    if (job.DocumentExpiration != -1) {
      if (!bool(jobTree["Meta"]))
        jobTree.NewChild("Meta");
      if (!bool(jobTree["Meta"]["DocumentExpiration"]))
        jobTree["Meta"].NewChild("DocumentExpiration") = (std::string)job.DocumentExpiration;
    }

    if (!job.Rank.empty()) {
      if (!bool(jobTree["Meta"]))
        jobTree.NewChild("Meta");
      if (!bool(jobTree["JobDescription"]["Meta"]["Rank"]))
        jobTree["Meta"].NewChild("Rank") = job.Rank;
    }
    if (job.FuzzyRank) {
      if (!bool(jobTree["Meta"]))
        jobTree.NewChild("Meta");
      if (!bool(jobTree["Meta"]["FuzzyRank"]))
        jobTree["Meta"].NewChild("FuzzyRank") = "true";
    }

    //Application
    if (!job.Argument.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      for (std::list<std::string>::const_iterator it = job.Argument.begin();
           it != job.Argument.end(); it++) {
        XMLNode attribute = jobTree["JobDescription"]["Application"].NewChild("Argument");
        attribute = (*it);
      }
    }
    if (!job.Input.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["Input"]))
        jobTree["JobDescription"]["Application"].NewChild("Input") = job.Input;
    }
    if (!job.Output.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["Output"]))
        jobTree["JobDescription"]["Application"].NewChild("Output") = job.Output;
    }
    if (!job.Error.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["Error"]))
        jobTree["JobDescription"]["Application"].NewChild("Error") = job.Error;
    }
    if (job.RemoteLogging) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["RemoteLogging"]))
        jobTree["JobDescription"]["Application"].NewChild("RemoteLogging") =
          job.RemoteLogging.fullstr();
    }
    if (!job.Environment.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      for (std::list<EnvironmentType>::const_iterator it = job.Environment.begin();
           it != job.Environment.end(); it++) {
        XMLNode attribute = jobTree["JobDescription"]["Application"].NewChild("Environment");
        attribute = (*it).value;
        attribute.NewAttribute("name") = (*it).name_attribute;
      }
    }
    if (job.LRMSReRun != -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["LRMSReRun"]))
        jobTree["JobDescription"]["Application"].NewChild("LRMSReRun");
      jobTree["JobDescription"]["Application"]["LRMSReRun"] = tostring(job.LRMSReRun);
    }
    if (!job.Prologue.Name.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["Prologue"])) {
        std::string prologue;
        prologue += job.Prologue.Name;
        std::list<std::string>::const_iterator iter;
        for (iter = job.Prologue.Arguments.begin();
             iter != job.Prologue.Arguments.end(); iter++) {
          prologue += " ";
          prologue += *iter;
        }
        jobTree["JobDescription"]["Application"].NewChild("Prologue") = prologue;
      }
    }
    if (!job.Epilogue.Name.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["Epilogue"])) {
        std::string epilogue;
        epilogue += job.Epilogue.Name;
        std::list<std::string>::const_iterator iter;
        for (iter = job.Epilogue.Arguments.begin();
             iter != job.Epilogue.Arguments.end(); iter++) {
          epilogue += " ";
          epilogue += *iter;
        }
        jobTree["JobDescription"]["Application"].NewChild("Epilogue") = epilogue;
      }
    }
    if (job.SessionLifeTime != -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["SessionLifeTime"]))
        jobTree["JobDescription"]["Application"].NewChild("SessionLifeTime") = job.SessionLifeTime;
    }
    if (job.AccessControl) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["AccessControl"]))
        jobTree["JobDescription"]["Application"].NewChild("AccessControl");
      jobTree["JobDescription"]["Application"]["AccessControl"].NewChild(job.AccessControl);
    }
    if (job.ProcessingStartTime != -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["ProcessingStartTime"]))
        jobTree["JobDescription"]["Application"].NewChild("ProcessingStartTime") =
          job.ProcessingStartTime;
    }
    if (!job.Notification.empty()) {
      if (!jobTree["JobDescription"])
        jobTree.NewChild("JobDescription");
      if (!jobTree["JobDescription"]["Application"])
        jobTree["JobDescription"].NewChild("Application");
      for (std::list<NotificationType>::const_iterator it = job.Notification.begin();
           it != job.Notification.end(); it++) {
        XMLNode attribute = jobTree["JobDescription"]["Application"].NewChild("Notification");
        for (std::list<std::string>::const_iterator iter = (*it).Address.begin();
             iter != (*it).Address.end(); iter++)
          attribute.NewChild("Address") = (*iter);
        for (std::list<std::string>::const_iterator iter = (*it).State.begin();
             iter != (*it).State.end(); iter++)
          attribute.NewChild("State") = (*iter);
      }
    }
    if (job.CredentialService) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["CredentialService"]))
        jobTree["JobDescription"]["Application"].NewChild("CredentialService") =
          job.CredentialService.fullstr();
    }
    if (job.Join) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Application"]))
        jobTree["JobDescription"].NewChild("Application");
      if (!bool(jobTree["JobDescription"]["Application"]["Join"]))
        jobTree["JobDescription"]["Application"].NewChild("Join") = "true";
    }

    // Resource
    if (job.TotalCPUTime != -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["TotalCPUTime"]))
        jobTree["JobDescription"]["Resource"].NewChild("TotalCPUTime") = (std::string)job.TotalCPUTime;
    }
    if (job.IndividualCPUTime != -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["IndividualCPUTime"]))
        jobTree["JobDescription"]["Resource"].NewChild("IndividualCPUTime") = (std::string)job.IndividualCPUTime;
    }
    if (job.TotalWallTime != -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["TotalWallTime"]))
        jobTree["JobDescription"]["Resource"].NewChild("TotalWallTime") = (std::string)job.TotalWallTime;
    }
    if (job.IndividualWallTime != -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["IndividualWallTime"]))
        jobTree["JobDescription"]["Resource"].NewChild("IndividualWallTime") = (std::string)job.IndividualWallTime;
    }
    if (!job.ReferenceTime.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      XMLNode attribute;
      if (!bool(jobTree["JobDescription"]["Resource"]["ReferenceTime"]))
        attribute = jobTree["JobDescription"]["Resource"].NewChild("ReferenceTime");
      attribute = (std::string)job.ReferenceTime.begin()->time;
      attribute.NewAttribute("benchmark") = job.ReferenceTime.begin()->benchmark;
      attribute.NewAttribute("value") = tostring(job.ReferenceTime.begin()->value);
    }
    if (job.ExclusiveExecution) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["ExclusiveExecution"]))
        jobTree["JobDescription"]["Resource"].NewChild("ExclusiveExecution") = "true";
    }
    if (!job.NetworkInfo.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["NetworkInfo"]))
        jobTree["JobDescription"]["Resource"].NewChild("NetworkInfo") = job.NetworkInfo;
    }
    if (!job.OSFamily.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["OSFamily"]))
        jobTree["JobDescription"]["Resource"].NewChild("OSFamily") = job.OSFamily;
    }
    if (!job.OSName.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["OSName"]))
        jobTree["JobDescription"]["Resource"].NewChild("OSName") = job.OSName;
    }
    if (!job.OSVersion.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["OSVersion"]))
        jobTree["JobDescription"]["Resource"].NewChild("OSVersion") = job.OSVersion;
    }
    if (!job.Platform.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Platform"]))
        jobTree["JobDescription"]["Resource"].NewChild("Platform") = job.Platform;
    }
    if (job.IndividualPhysicalMemory > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["IndividualPhysicalMemory"]))
        jobTree["JobDescription"]["Resource"].NewChild("IndividualPhysicalMemory");
      jobTree["JobDescription"]["Resource"]["IndividualPhysicalMemory"] = tostring(job.IndividualPhysicalMemory);
    }
    if (job.IndividualVirtualMemory > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["IndividualVirtualMemory"]))
        jobTree["JobDescription"]["Resource"].NewChild("IndividualVirtualMemory");
      jobTree["JobDescription"]["Resource"]["IndividualVirtualMemory"] = tostring(job.IndividualVirtualMemory);
    }
    if (job.DiskSpace > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["DiskSpace"]))
        jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
      jobTree["JobDescription"]["Resource"]["DiskSpace"] = tostring(job.DiskSpace);
    }
    if (job.CacheDiskSpace > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["DiskSpace"]))
        jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
      if (!bool(jobTree["JobDescription"]["Resource"]["DiskSpace"]["CacheDiskSpace"]))
        jobTree["JobDescription"]["Resource"]["DiskSpace"].NewChild("CacheDiskSpace");
      jobTree["JobDescription"]["Resource"]["DiskSpace"]["CacheDiskSpace"] = tostring(job.CacheDiskSpace);
    }
    if (job.SessionDiskSpace > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["DiskSpace"]))
        jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
      if (!bool(jobTree["JobDescription"]["Resource"]["DiskSpace"]["SessionDiskSpace"]))
        jobTree["JobDescription"]["Resource"]["DiskSpace"].NewChild("SessionDiskSpace");
      jobTree["JobDescription"]["Resource"]["DiskSpace"]["SessionDiskSpace"] = tostring(job.SessionDiskSpace);
    }
    if (job.IndividualDiskSpace > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["DiskSpace"]))
        jobTree["JobDescription"]["Resource"].NewChild("DiskSpace");
      if (!bool(jobTree["JobDescription"]["Resource"]["DiskSpace"]["IndividualDiskSpace"]))
        jobTree["JobDescription"]["Resource"]["DiskSpace"].NewChild("IndividualDiskSpace");
      jobTree["JobDescription"]["Resource"]["DiskSpace"]["IndividualDiskSpace"] = tostring(job.IndividualDiskSpace);
    }
    if (!job.Alias.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["CandidateTarget"]))
        jobTree["JobDescription"]["Resource"].NewChild("CandidateTarget");
      if (!bool(jobTree["JobDescription"]["Resource"]["CandidateTarget"]["Alias"]))
        jobTree["JobDescription"]["Resource"]["CandidateTarget"].NewChild("Alias") =
          job.Alias;
    }
    if (job.EndPointURL) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["CandidateTarget"]))
        jobTree["JobDescription"]["Resource"].NewChild("CandidateTarget");
      if (!bool(jobTree["JobDescription"]["Resource"]["CandidateTarget"]["EndPointURL"]))
        jobTree["JobDescription"]["Resource"]["CandidateTarget"].NewChild("EndPointURL") =
          job.EndPointURL.str();
    }
    if (!job.QueueName.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["CandidateTarget"]))
        jobTree["JobDescription"]["Resource"].NewChild("CandidateTarget");
      if (!bool(jobTree["JobDescription"]["Resource"]["CandidateTarget"]["QueueName"]))
        jobTree["JobDescription"]["Resource"]["CandidateTarget"].NewChild("QueueName") =
          job.QueueName;
    }
    if (!job.Country.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]))
        jobTree["JobDescription"]["Resource"].NewChild("Location");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]["Country"]))
        jobTree["JobDescription"]["Resource"]["Location"].NewChild("Country") =
          job.Country;
    }
    if (!job.Place.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]))
        jobTree["JobDescription"]["Resource"].NewChild("Location");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]["Place"]))
        jobTree["JobDescription"]["Resource"]["Location"].NewChild("Place") =
          job.Place;
    }
    if (!job.PostCode.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]))
        jobTree["JobDescription"]["Resource"].NewChild("Location");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]["PostCode"]))
        jobTree["JobDescription"]["Resource"]["Location"].NewChild("PostCode") =
          job.PostCode;
    }
    if (!job.Latitude.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]))
        jobTree["JobDescription"]["Resource"].NewChild("Location");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]["Latitude"]))
        jobTree["JobDescription"]["Resource"]["Location"].NewChild("Latitude") =
          job.Latitude;
    }
    if (!job.Longitude.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]))
        jobTree["JobDescription"]["Resource"].NewChild("Location");
      if (!bool(jobTree["JobDescription"]["Resource"]["Location"]["Longitude"]))
        jobTree["JobDescription"]["Resource"]["Location"].NewChild("Longitude") =
          job.Longitude;
    }
    if (!job.CEType.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["CEType"]))
        jobTree["JobDescription"]["Resource"].NewChild("CEType") = job.CEType;
    }
    if (job.Slots > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]))
        jobTree["JobDescription"]["Resource"].NewChild("Slots");
      jobTree["JobDescription"]["Resource"]["Slots"] = tostring(job.Slots);
    }
    if (job.NumberOfProcesses > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]))
        jobTree["JobDescription"]["Resource"].NewChild("Slots");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]["NumberOfProcesses"]))
        jobTree["JobDescription"]["Resource"]["Slots"].NewChild("NumberOfProcesses");
      jobTree["JobDescription"]["Resource"]["Slots"]["NumberOfProcesses"] = tostring(job.NumberOfProcesses);
    }
    if (job.ProcessPerHost > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]))
        jobTree["JobDescription"]["Resource"].NewChild("Slots");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]["ProcessPerHost"]))
        jobTree["JobDescription"]["Resource"]["Slots"].NewChild("ProcessPerHost");
      jobTree["JobDescription"]["Resource"]["Slots"]["ProcessPerHost"] = tostring(job.ProcessPerHost);
    }
    if (job.ThreadPerProcesses > -1) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]))
        jobTree["JobDescription"]["Resource"].NewChild("Slots");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]["ThreadPerProcesses"]))
        jobTree["JobDescription"]["Resource"]["Slots"].NewChild("ThreadPerProcesses");
      jobTree["JobDescription"]["Resource"]["Slots"]["ThreadPerProcesses"] = tostring(job.ThreadPerProcesses);
    }
    if (!job.SPMDVariation.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]))
        jobTree["JobDescription"]["Resource"].NewChild("Slots");
      if (!bool(jobTree["JobDescription"]["Resource"]["Slots"]["SPMDVariation"]))
        jobTree["JobDescription"]["Resource"]["Slots"].NewChild("SPMDVariation") =
          job.SPMDVariation;
    }
    if (!job.RunTimeEnvironment.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      std::list<RunTimeEnvironmentType>::const_iterator iter;
      for (iter = job.RunTimeEnvironment.begin();
           iter != job.RunTimeEnvironment.end(); iter++) {
        XMLNode attribute = jobTree["JobDescription"]["Resource"].NewChild("RunTimeEnvironment");
        attribute.NewChild("Name") = (*iter).Name;
        std::list<std::string>::const_iterator it;
        for (it = (*iter).Version.begin(); it != (*iter).Version.end(); it++)
          attribute.NewChild("Version") = (*it);
      }
    }
    if (job.Homogeneous) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["Homogeneous"]))
        jobTree["JobDescription"]["Resource"].NewChild("Homogeneous") = "true";
    }
    if (job.InBound) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["NodeAccess"]))
        jobTree["JobDescription"]["Resource"].NewChild("NodeAccess");
      if (!bool(jobTree["JobDescription"]["Resource"]["NodeAccess"]["InBound"]))
        jobTree["JobDescription"]["Resource"]["NodeAccess"].NewChild("InBound") = "true";
    }
    if (job.OutBound) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["Resource"]))
        jobTree["JobDescription"].NewChild("Resource");
      if (!bool(jobTree["JobDescription"]["Resource"]["NodeAccess"]))
        jobTree["JobDescription"]["Resource"].NewChild("NodeAccess");
      if (!bool(jobTree["JobDescription"]["Resource"]["NodeAccess"]["OutBound"]))
        jobTree["JobDescription"]["Resource"]["NodeAccess"].NewChild("OutBound") = "true";
    }

    //DataStaging
    if (!job.File.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["DataStaging"]))
        jobTree["JobDescription"].NewChild("DataStaging");
      std::list<FileType>::const_iterator iter;
      for (iter = job.File.begin(); iter != job.File.end(); iter++) {
        XMLNode attribute = jobTree["JobDescription"]["DataStaging"].NewChild("File");
        attribute.NewChild("Name") = (*iter).Name;
        std::list<SourceType>::const_iterator it_source;
        for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++) {
          XMLNode attribute_source = attribute.NewChild("Source");
          attribute_source.NewChild("URI") = (*it_source).URI.fullstr();
          if ((*it_source).Threads > -1)
            attribute_source.NewChild("Threads") = tostring((*it_source).Threads);
        }
        std::list<TargetType>::const_iterator it_target;
        for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++) {
          XMLNode attribute_target = attribute.NewChild("Target");
          attribute_target.NewChild("URI") = (*it_target).URI.fullstr();
          if ((*it_target).Threads > -1)
            attribute_target.NewChild("Threads") = tostring((*it_target).Threads);
          if ((*it_target).Mandatory)
            attribute_target.NewChild("Mandatory") = "true";
          if ((*it_target).NeededReplicas > -1)
            attribute_target.NewChild("NeededReplicas") = tostring((*it_target).NeededReplicas);
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
    if (!job.Directory.empty()) {
      if (!bool(jobTree["JobDescription"]))
        jobTree.NewChild("JobDescription");
      if (!bool(jobTree["JobDescription"]["DataStaging"]))
        jobTree["JobDescription"].NewChild("DataStaging");
      std::list<DirectoryType>::const_iterator iter;
      for (iter = job.Directory.begin(); iter != job.Directory.end(); iter++) {
        XMLNode attribute = jobTree["JobDescription"]["DataStaging"].NewChild("Directory");
        attribute.NewChild("Name") = (*iter).Name;
        std::list<SourceType>::const_iterator it_source;
        for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++) {
          XMLNode attribute_source = attribute.NewChild("Source");
          attribute_source.NewChild("URI") = (*it_source).URI.fullstr();
          if ((*it_source).Threads > -1)
            attribute_source.NewChild("Threads") = tostring((*it_source).Threads);
        }
        std::list<TargetType>::const_iterator it_target;
        for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++) {
          XMLNode attribute_target = attribute.NewChild("Target");
          attribute_target.NewChild("URI") = (*it_target).URI.fullstr();
          if ((*it_target).Threads > -1)
            attribute_target.NewChild("Threads") = tostring((*it_target).Threads);
          if ((*it_target).Mandatory)
            attribute_target.NewChild("Mandatory") = "true";
          if ((*it_target).NeededReplicas > -1)
            attribute_target.NewChild("NeededReplicas") = tostring((*it_target).NeededReplicas);
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
    //set the "isoptional" attributes
    if (!job.OptionalElement.empty())
      for (std::list<OptionalElementType>::const_iterator it = job.OptionalElement.begin();
           it != job.OptionalElement.end(); it++) {
        XMLNodeList attribute = jobTree.XPathLookup((*it).Path, gin_namespaces);
        for (std::list<XMLNode>::iterator it_node = attribute.begin(); it_node != attribute.end(); it_node++)
          if ((std::string)(*it_node) == (*it).Value || (*it).Value == "")
            (*it_node).NewAttribute("isoptional") = "true";
      }

    std::string product;
    jobTree.GetDoc(product, true);
    return product;
  }

} // namespace Arc
