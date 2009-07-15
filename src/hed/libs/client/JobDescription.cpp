// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobDescription.h"

#include "XRSLParser.h"
#include "JDLParser.h"
#include "ARCJSDLParser.h"


#define INTPRINT(X, Y) if ((X) > -1) \
  std::cout << IString(#Y ": %d", X) << std::endl;
#define STRPRINT(X, Y) if (!(X).empty()) \
  std::cout << IString(#Y ": %s", X) << std::endl;


namespace Arc {

  Logger JobDescription::logger(Logger::getRootLogger(), "JobDescription");

  JobDescription::JobDescription(const long int& ptraddr) { *this = *((JobDescription*)ptraddr); }

  void JobDescription::Print(bool longlist) const {

    STRPRINT(Application.Executable.Name, Executable);
    STRPRINT(Application.LogDir, Log Directory)
    STRPRINT(Identification.JobName, JobName)
    STRPRINT(Identification.Description, Description)
    STRPRINT(Identification.JobVOName, Virtual Organization)

/** Skou: Unclear...
    if (!Identification.JobType.empty())
      std::cout << IString(" JobType: %s", JobType) << std::endl;
*/

    if (longlist) {
      if (!Identification.UserTag.empty()) {
        std::list<std::string>::const_iterator iter = Identification.UserTag.begin();
        for (; iter != Identification.UserTag.end(); iter++)
          std::cout << IString(" UserTag: %s", *iter) << std::endl;
      }

      if (!Identification.ActivityOldId.empty()) {
        std::list<std::string>::const_iterator iter = Identification.ActivityOldId.begin();
        for (; iter != Identification.ActivityOldId.end(); iter++)
          std::cout << IString(" Activity Old Id: %s", *iter) << std::endl;
      }

      STRPRINT(JobMeta.Author, Author)
      if (JobMeta.DocumentExpiration.GetTime() > 0)
        std::cout << IString(" DocumentExpiration: %s", JobMeta.DocumentExpiration.str()) << std::endl;

      if (!Application.Executable.Argument.empty()) {
        std::list<std::string>::const_iterator iter = Application.Executable.Argument.begin();
        for (; iter != Application.Executable.Argument.end(); iter++)
          std::cout << IString(" Argument: %s", *iter) << std::endl;
      }

      STRPRINT(Application.Input, Input)
      STRPRINT(Application.Output, Output)
      STRPRINT(Application.Error, Error)

      if (!Application.RemoteLogging.empty()) {
        std::list<URL>::const_iterator iter = Application.RemoteLogging.begin();
        for (; iter != Application.RemoteLogging.end(); iter++) {
          std::cout << IString(" RemoteLogging: %s", iter->fullstr()) << std::endl;
        }
      }

      if (!Application.Environment.empty()) {
        std::list< std::pair<std::string, std::string> >::const_iterator iter = Application.Environment.begin();
        for (; iter != Application.Environment.end(); iter++) {
          std::cout << IString(" Environment.name: %s", iter->first) << std::endl;
          std::cout << IString(" Environment: %s", iter->second) << std::endl;
        }
      }

      INTPRINT(Application.Rerun, Rerun)

      STRPRINT(Application.Prologue.Name, Prologue)
      if (!Application.Prologue.Argument.empty()) {
        std::list<std::string>::const_iterator iter = Application.Prologue.Argument.begin();
        for (; iter != Application.Prologue.Argument.end(); iter++)
          std::cout << IString(" Prologue.Arguments: %s", *iter) << std::endl;
      }

      STRPRINT(Application.Epilogue.Name, Epilogue)
      if (!Application.Epilogue.Argument.empty()) {
        std::list<std::string>::const_iterator iter = Application.Epilogue.Argument.begin();
        for (; iter != Application.Epilogue.Argument.end(); iter++)
          std::cout << IString(" Epilogue.Arguments: %s", *iter) << std::endl;
      }

      INTPRINT(Application.SessionLifeTime.GetPeriod(), SessionLifeTime)

      if (bool(Application.AccessControl)) {
        std::string str;
        Application.AccessControl.GetXML(str, true);
        std::cout << IString(" AccessControl: %s", str) << std::endl;
      }

      if (Application.ProcessingStartTime.GetTime() > 0)
        std::cout << IString(" ProcessingStartTime: %s", Application.ProcessingStartTime.str()) << std::endl;

/** Skou: Currently not supported...
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
*/
      if (!Application.CredentialService.empty()) {
        std::list<URL>::const_iterator iter = Application.CredentialService.begin();
        for (; iter != Application.CredentialService.end(); iter++)
          std::cout << IString(" CredentialService: %s", iter->str()) << std::endl;
      }

      if (Application.Join)
        std::cout << " Join: true" << std::endl;

      INTPRINT(Resources.TotalCPUTime.current, TotalCPUTime)
      INTPRINT(Resources.IndividualCPUTime.current, IndividualCPUTime)
      INTPRINT(Resources.TotalWallTime.current, TotalWallTime)
      INTPRINT(Resources.IndividualWallTime.current, IndividualWallTime)

/** Skou: Currently not supported...
      for (std::list<ReferenceTimeType>::const_iterator it = ReferenceTime.begin();
           it != ReferenceTime.end(); it++) {
        std::cout << IString(" Benchmark: %s", it->benchmark) << std::endl;
        std::cout << IString("  value: %d", it->value) << std::endl;
        std::cout << IString("  time: %s", (std::string)it->time) << std::endl;
      }
*/

      STRPRINT(Resources.NetworkInfo, NetworkInfo)
      STRPRINT(Resources.OSFamily, OSFamily)
      STRPRINT(Resources.OSName, OSName)
      STRPRINT(Resources.OSVersion, OSVersion)
      STRPRINT(Resources.Platform, Platform)
      INTPRINT(Resources.IndividualPhysicalMemory.current, IndividualPhysicalMemory)
      INTPRINT(Resources.IndividualVirtualMemory.current, IndividualVirtualMemory)
      INTPRINT(Resources.IndividualDiskSpace, IndividualDiskSpace)
      INTPRINT(Resources.CacheDiskSpace, CacheDiskSpace)
      INTPRINT(Resources.SessionDiskSpace, SessionDiskSpace)
      INTPRINT(Resources.IndividualDiskSpace, IndividualDiskSpace)

      for (std::list<ResourceTargetType>::const_iterator it = Resources.CandidateTarget.begin();
           it != Resources.CandidateTarget.end(); it++) {
        if (it->EndPointURL)
          std::cout << IString(" EndPointURL: %s", it->EndPointURL.str()) << std::endl;
        if (!it->QueueName.empty())
          std::cout << IString(" QueueName: %s", it->QueueName) << std::endl;
      }

/** Skou: Currently not supported...
      if (!CEType.empty())
        std::cout << IString(" CEType: %s", CEType) << std::endl;
*/

/** Skou: Unclear...
      if (InBound)
        std::cout << IString(" InBound: true") << std::endl;
      if (OutBound)
        std::cout << IString(" OutBound: true") << std::endl;
*/

      INTPRINT(Resources.Slots.NumberOfProcesses.current, NumberOfProcesses)
      INTPRINT(Resources.Slots.ProcessPerHost.current, ProcessPerHost)
      INTPRINT(Resources.Slots.ThreadsPerProcesses.current, ThreadsPerProcesses)
      //STRPRINT(Resources.Slots.SPMDVariation, SPMDVariation) // Skou: Unclear...

/** Skou: Currently not supported...
      if (!RunTimeEnvironment.empty()) {
        std::list<RunTimeEnvironmentType>::const_iterator iter;
        for (iter = RunTimeEnvironment.begin(); iter != RunTimeEnvironment.end(); iter++) {
          std::cout << IString(" RunTimeEnvironment.Name: %s", (*iter).Name) << std::endl;
          std::list<std::string>::const_iterator it;
          for (it = (*iter).Version.begin(); it != (*iter).Version.end(); it++)
            std::cout << IString("    RunTimeEnvironment.Version: %s", (*it)) << std::endl;
        }
      }
*/

      if (!DataStaging.File.empty()) {
        std::list<FileType>::const_iterator iter = DataStaging.File.begin();
        for (; iter != DataStaging.File.end(); iter++) {
          std::cout << IString(" File element:") << std::endl;
          std::cout << IString("     Name: %s", iter->Name) << std::endl;

          std::vector<DataSourceType>::const_iterator itSource = iter->Source.begin();
          for (; itSource != iter->Source.end(); itSource++) {
            std::cout << IString("     Source.URI: %s", itSource->URI.fullstr()) << std::endl;
            INTPRINT(itSource->Threads, Source.Threads)
          }

          std::list<DataTargetType>::const_iterator itTarget = iter->Target.begin();
          for (; itTarget != iter->Target.end(); itTarget++) {
            std::cout << IString("     Target.URI: %s", itTarget->URI.fullstr()) << std::endl;
            INTPRINT(itTarget->Threads, Target.Threads)
            if (itTarget->Mandatory)
              std::cout << IString("     Target.Mandatory: true") << std::endl;
            INTPRINT(itTarget->NeededReplica, NeededReplica)
          }
          if (iter->KeepData)
            std::cout << IString("     KeepData: true") << std::endl;
          if (iter->IsExecutable)
            std::cout << IString("     IsExecutable: true") << std::endl;
          if (!iter->DataIndexingService.empty()) {
            std::list<URL>::const_iterator itDIS = iter->DataIndexingService.begin();
            for (; itDIS != iter->DataIndexingService.end(); itDIS++)
              std::cout << IString("     DataIndexingService: %s", itDIS->fullstr()) << std::endl;
          }
          if (iter->DownloadToCache)
            std::cout << IString("     DownloadToCache: true") << std::endl;
        }
      }

      if (!DataStaging.Directory.empty()) {
        std::list<DirectoryType>::const_iterator iter = DataStaging.Directory.begin();
        for (; iter != DataStaging.Directory.end(); iter++) {
          std::cout << IString(" Directory element:") << std::endl;
          std::cout << IString("     Name: %s", iter->Name) << std::endl;

          std::vector<DataSourceType>::const_iterator itSource = iter->Source.begin();
          for (; itSource != iter->Source.end(); itSource++) {
            std::cout << IString("     Source.URI: %s", itSource->URI.fullstr()) << std::endl;
            INTPRINT(itSource->Threads, Source.Threads)
          }

          std::list<DataTargetType>::const_iterator itTarget = iter->Target.begin();
          for (; itTarget != iter->Target.end(); itTarget++) {
            std::cout << IString("     Target.URI: %s", itTarget->URI.fullstr()) << std::endl;
            INTPRINT(itTarget->Threads, Target.Threads)
            if (itTarget->Mandatory)
              std::cout << IString("     Target.Mandatory: true") << std::endl;
            INTPRINT(itTarget->NeededReplica, NeededReplica)
          }
          if (iter->KeepData)
            std::cout << IString("     KeepData: true") << std::endl;
          if (iter->IsExecutable)
            std::cout << IString("     IsExecutable: true") << std::endl;
          if (!iter->DataIndexingService.empty()) {
            std::list<URL>::const_iterator itDIS = iter->DataIndexingService.begin();
            for (; itDIS != iter->DataIndexingService.end(); itDIS++)
              std::cout << IString("     DataIndexingService: %s", itDIS->fullstr()) << std::endl;
          }
          if (iter->DownloadToCache)
            std::cout << IString("     DownloadToCache: true") << std::endl;
        }
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

  bool JobDescription::Parse(const XMLNode& xmlSource)
  {
    std::string source;
    xmlSource.GetXML(source);
    return Parse(source);
  }

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

    logger.msg(DEBUG, "Try to parse as JDL");
    JDLParser parser2;
    *this = parser2.Parse(source);
    if (*this) {
      sourceFormat = "jdl";
      return true;
    }

    logger.msg(DEBUG, "Try to parse as ARCJSDL");
    ARCJSDLParser parser3;
    *this = parser3.Parse(source);
    if (*this) {
      sourceFormat = "arcjsdl";
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
      return product;
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
    else if (lower(format) == "arcjsdl") {
      logger.msg(DEBUG, "Generate ARCJSDL output");
      ARCJSDLParser parser;
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
    for (std::list<FileType>::const_iterator it = DataStaging.File.begin();
         it != DataStaging.File.end(); it++)
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
} // namespace Arc
