// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobDescription.h"

#include "XRSLParser.h"
#include "JDLParser.h"
#include "ARCJSDLParser.h"


#define INTPRINT(OUT, X, Y) if ((X) > -1) \
  OUT << IString(#Y ": %d", X) << std::endl;
#define STRPRINT(OUT, X, Y) if (!(X).empty()) \
  OUT << IString(#Y ": %s", X) << std::endl;


namespace Arc {

  Logger JobDescription::logger(Logger::getRootLogger(), "JobDescription");

  JobDescription::JobDescription(const long int& ptraddr) { *this = *((JobDescription*)ptraddr); }

  void JobDescription::Print(bool longlist) const {
    logger.msg(WARNING, "The JobDescription::Print method is DEPRECATED, use the JobDescription::SaveToStream method instead.");
    SaveToStream(std::cout, (!longlist ? "user" : "userlong")); // ??? Prepend "user*" with a character to avoid clashes?
  }

  bool JobDescription::SaveToStream(std::ostream& out, const std::string& format) const {

    if (format != "user" && format != "userlong") {
      const std::string outjobdesc = UnParse((!format.empty() ? format : sourceFormat));
      out << outjobdesc;
      return !outjobdesc.empty();
    }

    STRPRINT(out, Application.Executable.Name, Executable);
    STRPRINT(out, Application.LogDir, Log Directory)
    STRPRINT(out, Identification.JobName, JobName)
    STRPRINT(out, Identification.Description, Description)
    STRPRINT(out, Identification.JobVOName, Virtual Organization)

    switch (Identification.JobType) {
    case SINGLE:
      out << IString("Job type: single") << std::endl;
      break;
    case COLLECTIONELEMENT:
      out << IString("Job type: collection") << std::endl;
      break;
    case PARALLELELEMENT:
      out << IString("Job type: parallel") << std::endl;
      break;
    case WORKFLOWNODE:
      out << IString("Job type: workflownode") << std::endl;
      break;
    }

    if (format == "userlong") {
      if (!Identification.UserTag.empty()) {
        std::list<std::string>::const_iterator iter = Identification.UserTag.begin();
        for (; iter != Identification.UserTag.end(); iter++)
          out << IString(" UserTag: %s", *iter) << std::endl;
      }

      if (!Identification.ActivityOldId.empty()) {
        std::list<std::string>::const_iterator iter = Identification.ActivityOldId.begin();
        for (; iter != Identification.ActivityOldId.end(); iter++)
          out << IString(" Activity Old Id: %s", *iter) << std::endl;
      }

      STRPRINT(out, JobMeta.Author, Author)
      if (JobMeta.DocumentExpiration.GetTime() > 0)
        out << IString(" DocumentExpiration: %s", JobMeta.DocumentExpiration.str()) << std::endl;

      if (!Application.Executable.Argument.empty()) {
        std::list<std::string>::const_iterator iter = Application.Executable.Argument.begin();
        for (; iter != Application.Executable.Argument.end(); iter++)
          out << IString(" Argument: %s", *iter) << std::endl;
      }

      STRPRINT(out, Application.Input, Input)
      STRPRINT(out, Application.Output, Output)
      STRPRINT(out, Application.Error, Error)

      if (!Application.RemoteLogging.empty()) {
        std::list<URL>::const_iterator iter = Application.RemoteLogging.begin();
        for (; iter != Application.RemoteLogging.end(); iter++) {
          out << IString(" RemoteLogging: %s", iter->fullstr()) << std::endl;
        }
      }

      if (!Application.Environment.empty()) {
        std::list< std::pair<std::string, std::string> >::const_iterator iter = Application.Environment.begin();
        for (; iter != Application.Environment.end(); iter++) {
          out << IString(" Environment.name: %s", iter->first) << std::endl;
          out << IString(" Environment: %s", iter->second) << std::endl;
        }
      }

      INTPRINT(out, Application.Rerun, Rerun)

      STRPRINT(out, Application.Prologue.Name, Prologue)
      if (!Application.Prologue.Argument.empty()) {
        std::list<std::string>::const_iterator iter = Application.Prologue.Argument.begin();
        for (; iter != Application.Prologue.Argument.end(); iter++)
          out << IString(" Prologue.Arguments: %s", *iter) << std::endl;
      }

      STRPRINT(out, Application.Epilogue.Name, Epilogue)
      if (!Application.Epilogue.Argument.empty()) {
        std::list<std::string>::const_iterator iter = Application.Epilogue.Argument.begin();
        for (; iter != Application.Epilogue.Argument.end(); iter++)
          out << IString(" Epilogue.Arguments: %s", *iter) << std::endl;
      }

      INTPRINT(out, Resources.SessionLifeTime.GetPeriod(), SessionLifeTime)

      if (bool(Application.AccessControl)) {
        std::string str;
        Application.AccessControl.GetXML(str, true);
        out << IString(" AccessControl: %s", str) << std::endl;
      }

      if (Application.ProcessingStartTime.GetTime() > 0)
        out << IString(" ProcessingStartTime: %s", Application.ProcessingStartTime.str()) << std::endl;

      if (Application.Notification.size() > 0) {
        out << IString(" Notify:") << std::endl;
        for (std::list<NotificationType>::const_iterator it = Application.Notification.begin();
             it != Application.Notification.end(); it++) {
          for (std::list<std::string>::const_iterator it2 = it->States.begin();
               it2 != it->States.end(); it2++) {
            out << " " << *it2;
          }
          out << ":   " << it->Email << std::endl;
        }
      }

      if (!Application.CredentialService.empty()) {
        std::list<URL>::const_iterator iter = Application.CredentialService.begin();
        for (; iter != Application.CredentialService.end(); iter++)
          out << IString(" CredentialService: %s", iter->str()) << std::endl;
      }

      if (Application.Join)
        out << " Join: true" << std::endl;

      INTPRINT(out, Resources.TotalCPUTime.range.max, TotalCPUTime)
      INTPRINT(out, Resources.IndividualCPUTime.range.max, IndividualCPUTime)
      INTPRINT(out, Resources.TotalWallTime.range.max, TotalWallTime)
      INTPRINT(out, Resources.IndividualWallTime.range.max, IndividualWallTime)

      STRPRINT(out, Resources.NetworkInfo, NetworkInfo)

      if (!Resources.OperatingSystem.empty()) {
        out << IString(" Operating system requirements:") << std::endl;
        std::list<Software>::const_iterator itOS = Resources.OperatingSystem.getSoftwareList().begin();
        std::list<Software::ComparisonOperator>::const_iterator itCO = Resources.OperatingSystem.getComparisonOperatorList().begin();
        for (; itOS != Resources.OperatingSystem.getSoftwareList().end(); itOS++, itCO++) {
          if (*itCO != &Software::operator==) out << Software::toString(*itCO) << " ";
          out << *itOS << std::endl;
        }
      }

      STRPRINT(out, Resources.Platform, Platform)
      INTPRINT(out, Resources.IndividualPhysicalMemory.max, IndividualPhysicalMemory)
      INTPRINT(out, Resources.IndividualVirtualMemory.max, IndividualVirtualMemory)
      INTPRINT(out, Resources.DiskSpaceRequirement.DiskSpace.max, DiskSpace)
      INTPRINT(out, Resources.DiskSpaceRequirement.CacheDiskSpace, CacheDiskSpace)
      INTPRINT(out, Resources.DiskSpaceRequirement.SessionDiskSpace, SessionDiskSpace)

      for (std::list<ResourceTargetType>::const_iterator it = Resources.CandidateTarget.begin();
           it != Resources.CandidateTarget.end(); it++) {
        if (it->EndPointURL)
          out << IString(" EndPointURL: %s", it->EndPointURL.str()) << std::endl;
        if (!it->QueueName.empty())
          out << IString(" QueueName: %s", it->QueueName) << std::endl;
      }

      if (!Resources.CEType.empty()) {
        out << IString(" Computing endpoint requirements:") << std::endl;
        std::list<Software>::const_iterator itCE = Resources.CEType.getSoftwareList().begin();
        std::list<Software::ComparisonOperator>::const_iterator itCO = Resources.CEType.getComparisonOperatorList().begin();
        for (; itCE != Resources.CEType.getSoftwareList().end(); itCE++, itCO++) {
          if (*itCO != &Software::operator==) out << Software::toString(*itCO) << " ";
          out << *itCE << std::endl;
        }
      }

      switch (Resources.NodeAccess) {
      case NAT_NONE:
        break;
      case NAT_INBOUND:
        out << IString(" NodeAccess: Inbound") << std::endl;
        break;
      case NAT_OUTBOUND:
        out << IString(" NodeAccess: Outbound") << std::endl;
        break;
      case NAT_INOUTBOUND:
        out << IString(" NodeAccess: Inbound and Outbound") << std::endl;
        break;
      }

      INTPRINT(out, Resources.SlotRequirement.NumberOfSlots.max, NumberOfSlots)
      INTPRINT(out, Resources.SlotRequirement.ProcessPerHost.max, ProcessPerHost)
      INTPRINT(out, Resources.SlotRequirement.ThreadsPerProcesses.max, ThreadsPerProcesses)
      STRPRINT(out, Resources.SlotRequirement.SPMDVariation, SPMDVariation)

      if (!Resources.RunTimeEnvironment.empty()) {
        out << IString(" Run time environment requirements:") << std::endl;
        std::list<Software>::const_iterator itSW = Resources.RunTimeEnvironment.getSoftwareList().begin();
        std::list<Software::ComparisonOperator>::const_iterator itCO = Resources.RunTimeEnvironment.getComparisonOperatorList().begin();
        for (; itSW != Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++, itCO++) {
          if (*itCO != &Software::operator==) out << Software::toString(*itCO) << " ";
          out << *itSW << std::endl;
        }
      }

      if (!DataStaging.File.empty()) {
        std::list<FileType>::const_iterator iter = DataStaging.File.begin();
        for (; iter != DataStaging.File.end(); iter++) {
          out << IString(" File element:") << std::endl;
          out << IString("     Name: %s", iter->Name) << std::endl;

          std::list<DataSourceType>::const_iterator itSource = iter->Source.begin();
          for (; itSource != iter->Source.end(); itSource++) {
            out << IString("     Source.URI: %s", itSource->URI.fullstr()) << std::endl;
            INTPRINT(out, itSource->Threads, Source.Threads)
          }

          std::list<DataTargetType>::const_iterator itTarget = iter->Target.begin();
          for (; itTarget != iter->Target.end(); itTarget++) {
            out << IString("     Target.URI: %s", itTarget->URI.fullstr()) << std::endl;
            INTPRINT(out, itTarget->Threads, Target.Threads)
            if (itTarget->Mandatory)
              out << IString("     Target.Mandatory: true") << std::endl;
            INTPRINT(out, itTarget->NeededReplica, NeededReplica)
          }
          if (iter->KeepData)
            out << IString("     KeepData: true") << std::endl;
          if (iter->IsExecutable)
            out << IString("     IsExecutable: true") << std::endl;
          if (!iter->DataIndexingService.empty()) {
            std::list<URL>::const_iterator itDIS = iter->DataIndexingService.begin();
            for (; itDIS != iter->DataIndexingService.end(); itDIS++)
              out << IString("     DataIndexingService: %s", itDIS->fullstr()) << std::endl;
          }
          if (iter->DownloadToCache)
            out << IString("     DownloadToCache: true") << std::endl;
        }
      }

      if (!DataStaging.Directory.empty()) {
        std::list<DirectoryType>::const_iterator iter = DataStaging.Directory.begin();
        for (; iter != DataStaging.Directory.end(); iter++) {
          out << IString(" Directory element:") << std::endl;
          out << IString("     Name: %s", iter->Name) << std::endl;

          std::list<DataSourceType>::const_iterator itSource = iter->Source.begin();
          for (; itSource != iter->Source.end(); itSource++) {
            out << IString("     Source.URI: %s", itSource->URI.fullstr()) << std::endl;
            INTPRINT(out, itSource->Threads, Source.Threads)
          }

          std::list<DataTargetType>::const_iterator itTarget = iter->Target.begin();
          for (; itTarget != iter->Target.end(); itTarget++) {
            out << IString("     Target.URI: %s", itTarget->URI.fullstr()) << std::endl;
            INTPRINT(out, itTarget->Threads, Target.Threads)
            if (itTarget->Mandatory)
              out << IString("     Target.Mandatory: true") << std::endl;
            INTPRINT(out, itTarget->NeededReplica, NeededReplica)
          }
          if (iter->KeepData)
            out << IString("     KeepData: true") << std::endl;
          if (iter->IsExecutable)
            out << IString("     IsExecutable: true") << std::endl;
          if (!iter->DataIndexingService.empty()) {
            std::list<URL>::const_iterator itDIS = iter->DataIndexingService.begin();
            for (; itDIS != iter->DataIndexingService.end(); itDIS++)
              out << IString("     DataIndexingService: %s", itDIS->fullstr()) << std::endl;
          }
          if (iter->DownloadToCache)
            out << IString("     DownloadToCache: true") << std::endl;
        }
      }
    }

    if (!XRSL_elements.empty()) {
      std::map<std::string, std::string>::const_iterator it;
      for (it = XRSL_elements.begin(); it != XRSL_elements.end(); it++)
        out << IString(" XRSL_elements: [%s], %s", it->first, it->second) << std::endl;
    }

    if (!JDL_elements.empty()) {
      std::map<std::string, std::string>::const_iterator it;
      for (it = JDL_elements.begin(); it != JDL_elements.end(); it++)
        out << IString(" JDL_elements: [%s], %s", it->first, it->second) << std::endl;
    }

    out << std::endl;

    return true;
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

    {
      logger.msg(VERBOSE, "Try to parse as XRSL");
      XRSLParser parser;
      parser.SetHints(hints);
      *this = parser.Parse(source);
      if (*this) {
        sourceFormat = "xrsl";
        return true;
      }
    }

    {
      logger.msg(VERBOSE, "Try to parse as JDL");
      JDLParser parser;
      parser.SetHints(hints);
      *this = parser.Parse(source);
      if (*this) {
        sourceFormat = "jdl";
        return true;
      }
    }

    {
      logger.msg(VERBOSE, "Try to parse as ARCJSDL");
      ARCJSDLParser parser;
      parser.SetHints(hints);
      *this = parser.Parse(source);
      if (*this) {
        sourceFormat = "arcjsdl";
        return true;
      }
    }

    return false;
  }

  // Generate the output in the requested format
  std::string JobDescription::UnParse(const std::string& format) const {
    std::string product;

    // Generate the output text with the right parser class
    if (!*this) {
      logger.msg(VERBOSE, "There is no successfully parsed source");
      return product;
    }

    if (lower(format) == "jdl") {
      logger.msg(VERBOSE, "Generate JDL output");
      JDLParser parser;
      parser.SetHints(hints);
      product = parser.UnParse(*this);
      if (product.empty())
        logger.msg(ERROR, "Generating %s output was unsuccessful", format);
    }
    else if (lower(format) == "xrsl") {
      logger.msg(VERBOSE, "Generate XRSL output");
      XRSLParser parser;
      parser.SetHints(hints);
      product = parser.UnParse(*this);
      if (product.empty())
        logger.msg(ERROR, "Generating %s output was unsuccessful", format);
    }
    else if (lower(format) == "arcjsdl") {
      logger.msg(VERBOSE, "Generate ARCJSDL output");
      ARCJSDLParser parser;
      parser.SetHints(hints);
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
      logger.msg(VERBOSE, "There is no input defined yet or it's format can be determinized.");
      return false;
    }
    else {
      _sourceFormat = sourceFormat;
      return true;
    }
  }

  void JobDescription::AddHint(const std::string& key,const std::string& value) {
    if(key.empty()) return;
    hints[key]=value;
  }

} // namespace Arc
