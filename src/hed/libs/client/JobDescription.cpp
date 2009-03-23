// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobDescription.h"

#include "XRSLParser.h"
#include "JSDLParser.h"
#include "PosixJSDLParser.h"
#include "JDLParser.h"

namespace Arc {

  Logger JobDescription::logger(Logger::getRootLogger(), "JobDescription");

  JobDescription::JobDescription()
    : FuzzyRank(false),
      DocumentExpiration(-1),
      LRMSReRun(-1),
      SessionLifeTime(-1),
      ProcessingStartTime(-1),
      Join(false),
      TotalCPUTime(-1),
      IndividualCPUTime(-1),
      TotalWallTime(-1),
      IndividualWallTime(-1),
      ExclusiveExecution(false),
      IndividualPhysicalMemory(-1),
      IndividualVirtualMemory(-1),
      DiskSpace(-1),
      CacheDiskSpace(-1),
      SessionDiskSpace(-1),
      IndividualDiskSpace(-1),
      Slots(-1),
      NumberOfProcesses(-1),
      ProcessPerHost(-1),
      ThreadPerProcesses(-1),
      Homogeneous(false),
      InBound(false),
      OutBound(false),
      cached(false) {}

  JobDescription::~JobDescription() {}

  void JobDescription::Print(bool longlist) const {

    std::cout << IString(" Executable: %s", Executable) << std::endl;
    std::cout << IString(" Grid Manager Log Directory: %s", LogDir) << std::endl;
    if (!JobName.empty())
      std::cout << IString(" JobName: %s", JobName) << std::endl;
    if (!Description.empty())
      std::cout << IString(" Description: %s", Description) << std::endl;
    if (!JobProject.empty())
      std::cout << IString(" JobProject: %s", JobProject) << std::endl;
    if (!JobType.empty())
      std::cout << IString(" JobType: %s", JobType) << std::endl;
    if (!JobCategory.empty())
      std::cout << IString(" JobCategory: %s", JobCategory) << std::endl;

    if (longlist) {
      if (!UserTag.empty()) {
        std::list<std::string>::const_iterator iter;
        for (iter = UserTag.begin(); iter != UserTag.end(); iter++)
          std::cout << IString(" UserTag: %s", *iter) << std::endl;
      }

      if (!OldJobIDs.empty()) {
        std::list<URL>::const_iterator m_iter;
        int i = 1;
        for (m_iter = OldJobIDs.begin(); m_iter != OldJobIDs.end(); m_iter++, i++)
          std::cout << IString(" %d. Old Job EPR: %s", i, m_iter->str()) << std::endl;
      }

      if (!OptionalElement.empty()) {
        std::list<OptionalElementType>::const_iterator iter;
        for (iter = OptionalElement.begin(); iter != OptionalElement.end(); iter++) {
          std::cout << IString(" OptionalElement: %s, path: %s", (*iter).Name, (*iter).Path);
          if ((*iter).Value != "")
            std::cout << IString(", value: %s", (*iter).Value);
          std::cout << std::endl;
        }
      }
      if (!Author.empty())
        std::cout << IString(" Author: %s", Author) << std::endl;
      if (DocumentExpiration != -1)
        std::cout << IString(" DocumentExpiration: %s",
                             (std::string)DocumentExpiration) << std::endl;
      if (!Argument.empty()) {
        std::list<std::string>::const_iterator iter;
        for (iter = Argument.begin(); iter != Argument.end(); iter++)
          std::cout << IString(" Argument: %s", *iter) << std::endl;
      }
      if (!Input.empty())
        std::cout << IString(" Input: %s", Input) << std::endl;
      if (!Output.empty())
        std::cout << IString(" Output: %s", Output) << std::endl;
      if (!Error.empty())
        std::cout << IString(" Error: %s", Error) << std::endl;
      if (bool(RemoteLogging))
        std::cout << IString(" RemoteLogging: %s", RemoteLogging.fullstr()) << std::endl;
      if (!Environment.empty()) {
        std::list<EnvironmentType>::const_iterator iter;
        for (iter = Environment.begin(); iter != Environment.end(); iter++) {
          std::cout << IString(" Environment.name: %s", (*iter).name_attribute) << std::endl;
          std::cout << IString(" Environment: %s", (*iter).value) << std::endl;
        }
      }
      if (LRMSReRun > -1)
        std::cout << IString(" LRMSReRun: %d", LRMSReRun) << std::endl;
      if (!Prologue.Name.empty())
        std::cout << IString(" Prologue: %s", Prologue.Name) << std::endl;
      if (!Prologue.Arguments.empty()) {
        std::list<std::string>::const_iterator iter;
        for (iter = Prologue.Arguments.begin(); iter != Prologue.Arguments.end(); iter++)
          std::cout << IString(" Prologue.Arguments: %s", *iter) << std::endl;
      }
      if (!Epilogue.Name.empty())
        std::cout << IString(" Epilogue: %s", Epilogue.Name) << std::endl;
      if (!Epilogue.Arguments.empty()) {
        std::list<std::string>::const_iterator iter;
        for (iter = Epilogue.Arguments.begin(); iter != Epilogue.Arguments.end(); iter++)
          std::cout << IString(" Epilogue.Arguments: %s", *iter) << std::endl;
      }
      if (SessionLifeTime != -1)
        std::cout << IString(" SessionLifeTime: %s",
                             (std::string)SessionLifeTime) << std::endl;
      if (bool(AccessControl)) {
        std::string str;
        AccessControl.GetXML(str, true);
        std::cout << IString(" AccessControl: %s", str) << std::endl;
      }
      if (ProcessingStartTime != -1)
        std::cout << IString(" ProcessingStartTime: %s",
                             (std::string)ProcessingStartTime.str()) << std::endl;
      if (!Notification.empty())
        for (std::list<NotificationType>::const_iterator it = Notification.begin(); it != Notification.end(); it++) {
          std::cout << IString(" Notification element: ") << std::endl;
          for (std::list<std::string>::const_iterator iter = (*it).Address.begin();
               iter != (*it).Address.end(); iter++)
            std::cout << IString("    Address: %s", (*iter)) << std::endl;
          for (std::list<std::string>::const_iterator iter = (*it).State.begin();
               iter != (*it).State.end(); iter++)
            std::cout << IString("    State: %s", (*iter)) << std::endl;
        }
      if (bool(CredentialService))
        std::cout << IString(" CredentialService: %s", CredentialService.str()) << std::endl;
      if (Join)
        std::cout << " Join: true" << std::endl;
      if (TotalCPUTime != -1)
        std::cout << IString(" Total CPU Time: %s",
                             (std::string)TotalCPUTime) << std::endl;
      if (IndividualCPUTime != -1)
        std::cout << IString(" Individual CPU Time: %s",
                             (std::string)IndividualCPUTime) << std::endl;
      if (TotalWallTime != -1)
        std::cout << IString(" Total Wall Time: %s",
                             (std::string)TotalWallTime) << std::endl;
      if (IndividualWallTime != -1)
        std::cout << IString(" Individual Wall Time: %s",
                             (std::string)IndividualWallTime) << std::endl;
      for (std::list<ReferenceTimeType>::const_iterator it = ReferenceTime.begin();
           it != ReferenceTime.end(); it++) {
        std::cout << IString(" Benchmark: %s", it->benchmark) << std::endl;
        std::cout << IString("  value: %d", it->value) << std::endl;
        std::cout << IString("  time: %s", (std::string)it->time) << std::endl;
      }
      if (ExclusiveExecution)
        std::cout << " ExclusiveExecution: true" << std::endl;
      if (!NetworkInfo.empty())
        std::cout << IString(" NetworkInfo: %s", NetworkInfo) << std::endl;
      if (!OSFamily.empty())
        std::cout << IString(" OSFamily: %s", OSFamily) << std::endl;
      if (!OSName.empty())
        std::cout << IString(" OSName: %s", OSName) << std::endl;
      if (!OSVersion.empty())
        std::cout << IString(" OSVersion: %s", OSVersion) << std::endl;
      if (!Platform.empty())
        std::cout << IString(" Platform: %s", Platform) << std::endl;
      if (IndividualPhysicalMemory > -1)
        std::cout << IString(" IndividualPhysicalMemory: %d", IndividualPhysicalMemory) << std::endl;
      if (IndividualVirtualMemory > -1)
        std::cout << IString(" IndividualVirtualMemory: %d", IndividualVirtualMemory) << std::endl;
      if (DiskSpace > -1)
        std::cout << IString(" DiskSpace: %d", DiskSpace) << std::endl;
      if (CacheDiskSpace > -1)
        std::cout << IString(" CacheDiskSpace: %d", CacheDiskSpace) << std::endl;
      if (SessionDiskSpace > -1)
        std::cout << IString(" SessionDiskSpace: %d", SessionDiskSpace) << std::endl;
      if (IndividualDiskSpace > -1)
        std::cout << IString(" IndividualDiskSpace: %d", IndividualDiskSpace) << std::endl;
      if (!Alias.empty())
        std::cout << IString(" Alias: %s", Alias) << std::endl;
      if (bool(EndPointURL))
        std::cout << IString(" EndPointURL: %s", EndPointURL.str()) << std::endl;
      if (!QueueName.empty())
        std::cout << IString(" QueueName: %s", QueueName) << std::endl;
      if (!Country.empty())
        std::cout << IString(" Country: %s", Country) << std::endl;
      if (!Place.empty())
        std::cout << IString(" Place: %s", Place) << std::endl;
      if (!PostCode.empty())
        std::cout << IString(" PostCode: %s", PostCode) << std::endl;
      if (!Latitude.empty())
        std::cout << IString(" Latitude: %s", Latitude) << std::endl;
      if (!Longitude.empty())
        std::cout << IString(" Longitude: %s", Longitude) << std::endl;
      if (!CEType.empty())
        std::cout << IString(" CEType: %s", CEType) << std::endl;
      if (Slots > -1)
        std::cout << IString(" Slots: %d", Slots) << std::endl;
      if (NumberOfProcesses > -1)
        std::cout << IString(" NumberOfProcesses: %d", NumberOfProcesses) << std::endl;
      if (ProcessPerHost > -1)
        std::cout << IString(" ProcessPerHost: %d", ProcessPerHost) << std::endl;
      if (ThreadPerProcesses > -1)
        std::cout << IString(" ThreadPerProcesses: %d", ThreadPerProcesses) << std::endl;
      if (!SPMDVariation.empty())
        std::cout << IString(" SPMDVariation: %s", SPMDVariation) << std::endl;
      if (!RunTimeEnvironment.empty()) {
        std::list<RunTimeEnvironmentType>::const_iterator iter;
        for (iter = RunTimeEnvironment.begin(); iter != RunTimeEnvironment.end(); iter++) {
          std::cout << IString(" RunTimeEnvironment.Name: %s", (*iter).Name) << std::endl;
          std::list<std::string>::const_iterator it;
          for (it = (*iter).Version.begin(); it != (*iter).Version.end(); it++)
            std::cout << IString("    RunTimeEnvironment.Version: %s", (*it)) << std::endl;
        }
      }
      if (Homogeneous)
        std::cout << IString(" Homogeneous: true") << std::endl;
      if (InBound)
        std::cout << IString(" InBound: true") << std::endl;
      if (OutBound)
        std::cout << IString(" OutBound: true") << std::endl;
      if (!File.empty()) {
        std::list<FileType>::const_iterator iter;
        for (iter = File.begin(); iter != File.end(); iter++) {
          std::cout << IString(" File element:") << std::endl;
          std::cout << IString("     Name: %s", (*iter).Name) << std::endl;
          std::list<SourceType>::const_iterator it_source;
          for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++) {
            std::cout << IString("     Source.URI: %s", (*it_source).URI.fullstr()) << std::endl;
            if ((*it_source).Threads > -1)
              std::cout << IString("     Source.Threads: %d", (*it_source).Threads) << std::endl;
          }
          std::list<TargetType>::const_iterator it_target;
          for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++) {
            std::cout << IString("     Target.URI: %s", (*it_target).URI.fullstr()) << std::endl;
            if ((*it_target).Threads > -1)
              std::cout << IString("     Target.Threads: %d", (*it_target).Threads) << std::endl;
            if ((*it_target).Mandatory)
              std::cout << IString("     Target.Mandatory: true") << std::endl;
            if ((*it_target).NeededReplicas > -1)
              std::cout << IString("     Target.NeededReplicas: %d", (*it_target).NeededReplicas) << std::endl;
          }
          if ((*iter).KeepData)
            std::cout << IString("     KeepData: true") << std::endl;
          if ((*iter).IsExecutable)
            std::cout << IString("     IsExecutable: true") << std::endl;
          if (bool((*iter).DataIndexingService))
            std::cout << IString("     DataIndexingService: %s", (*iter).DataIndexingService.fullstr()) << std::endl;
          if ((*iter).DownloadToCache)
            std::cout << IString("     DownloadToCache: true") << std::endl;
        }
      }
      if (!Directory.empty()) {
        std::list<DirectoryType>::const_iterator iter;
        for (iter = Directory.begin(); iter != Directory.end(); iter++)
          std::cout << IString(" Directory elemet:") << std::endl;
        std::cout << IString("     Name: %s", (*iter).Name) << std::endl;
        std::list<SourceType>::const_iterator it_source;
        for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++) {
          std::cout << IString("     Source.URI: %s", (*it_source).URI.fullstr()) << std::endl;
          if ((*it_source).Threads > -1)
            std::cout << IString("     Source.Threads: %d", (*it_source).Threads) << std::endl;
        }
        std::list<TargetType>::const_iterator it_target;
        for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++) {
          std::cout << IString("     Target.URI: %s", (*it_target).URI.fullstr()) << std::endl;
          if ((*it_target).Threads > -1)
            std::cout << IString("     Target.Threads: %d", (*it_target).Threads) << std::endl;
          if ((*it_target).Mandatory)
            std::cout << IString("     Target.Mandatory: true") << std::endl;
          if ((*it_target).NeededReplicas > -1)
            std::cout << IString("     Target.NeededReplicas: %d", (*it_target).NeededReplicas) << std::endl;
        }
        if ((*iter).KeepData)
          std::cout << IString("     KeepData: true") << std::endl;
        if ((*iter).IsExecutable)
          std::cout << IString("     IsExecutable: true") << std::endl;
        if (bool((*iter).DataIndexingService))
          std::cout << IString("     DataIndexingService: %s", (*iter).DataIndexingService.fullstr()) << std::endl;
        if ((*iter).DownloadToCache)
          std::cout << IString("     DownloadToCache: true") << std::endl;
      }
    }

    if (!XRSL_elements.empty()) {
      std::map<std::string, std::string>::const_iterator it;
      for (it = XRSL_elements.begin(); it != XRSL_elements.end(); it++)
        std::cout << IString(" XRSL_elements: [%s], %s", it->first, it->second) << std::endl;
    }

    if (!JDL_elements.empty()) {
      std::map<std::string, std::string>::const_iterator it;
      for (it = JDL_elements.begin(); it != JDL_elements.end(); it++)
        std::cout << IString(" JDL_elements: [%s], %s", it->first, it->second) << std::endl;
    }

    std::cout << std::endl;
  } // end of Print

  bool JobDescription::Parse(const std::string& source) {
    if (source.empty()) {
      logger.msg(ERROR, "Empty job description source string");
      return false;
    }

    logger.msg(DEBUG, "Try to parse as XRSL");
    XRSLParser parser1;
    *this = parser1.Parse(source);
    if (*this) {
      sourceFormat = "xrsl";
      return true;
    }

    logger.msg(DEBUG, "Try to parse as POSIX JSDL");
    PosixJSDLParser parser2;
    *this = parser2.Parse(source);
    if (*this) {
      sourceFormat = "posixjsdl";
      return true;
    }

    logger.msg(DEBUG, "Try to parse as JSDL");
    JSDLParser parser3;
    *this = parser3.Parse(source);
    if (*this) {
      sourceFormat = "jsdl";
      return true;
    }

    logger.msg(DEBUG, "Try to parse as JDL");
    JDLParser parser4;
    *this = parser4.Parse(source);
    if (*this) {
      sourceFormat = "jdl";
      return true;
    }

    logger.msg(ERROR, "The parsing of the job description was unsuccessful");
    return false;
  }

  // Generate the output in the requested format
  std::string JobDescription::UnParse(const std::string& format) const {
    std::string product;

    // Generate the output text with the right parser class
    if (!*this) {
      logger.msg(DEBUG, "There is no successfully parsed source");
      return false;
    }

    if (lower(format) == "jdl") {
      logger.msg(DEBUG, "Generate JDL output");
      JDLParser parser;
      product = parser.UnParse(*this);
      if (product.empty())
        logger.msg(ERROR, "Generating %s output was unsuccessful", format);
    }
    else if (lower(format) == "xrsl") {
      logger.msg(DEBUG, "Generate XRSL output");
      XRSLParser parser;
      product = parser.UnParse(*this);
      if (product.empty())
        logger.msg(ERROR, "Generating %s output was unsuccessful", format);
    }
    else if (lower(format) == "posixjsdl") {
      logger.msg(DEBUG, "Generate POSIX JSDL output");
      PosixJSDLParser parser;
      product = parser.UnParse(*this);
      if (product.empty())
        logger.msg(ERROR, "Generating %s output was unsuccessful", format);
    }
    else if (lower(format) == "jsdl") {
      logger.msg(DEBUG, "Generate JSDL output");
      JSDLParser parser;
      product = parser.UnParse(*this);
      if (product.empty())
        logger.msg(ERROR, "Generating %s output was unsuccessful", format);
    }
    else
      logger.msg(ERROR, "Unknown output format: %s", format);

    return product;
  }

  bool JobDescription::getSourceFormat(std::string& _sourceFormat) const {
    if (!*this) {
      logger.msg(DEBUG, "There is no input defined yet or it's format can be determinized.");
      return false;
    }
    else {
      _sourceFormat = sourceFormat;
      return true;
    }
  }

  bool JobDescription::getUploadableFiles(std::list<std::pair<std::string,
                                                              std::string> >& sourceFiles) const {
    if (!*this) {
      logger.msg(DEBUG, "There is no input defined yet or it's format can be determinized.");
      return false;
    }
    // Get the URI's from the DataStaging/File/Source files
    for (std::list<FileType>::const_iterator it = File.begin();
         it != File.end(); it++)
      if (!it->Source.empty()) {
        const URL& uri = it->Source.begin()->URI;
        const std::string& filename = it->Name;
        if (uri.Protocol() == "file") {
          std::pair<std::string, std::string> item(filename, uri.Path());
          sourceFiles.push_back(item);
        }
      }
    return true;
  }

  bool JobDescription::addOldJobID(const URL& oldjobid) {
    OldJobIDs.push_back(oldjobid);
    return true;
  }

} // namespace Arc
