// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/client/JobDescriptionParser.h>

#include "JobDescription.h"


#define INTPRINT(OUT, X, Y) if ((X) > -1) \
  OUT << IString(#Y ": %d", X) << std::endl;
#define STRPRINT(OUT, X, Y) if (!(X).empty()) \
  OUT << IString(#Y ": %s", X) << std::endl;


namespace Arc {

  Logger JobDescription::logger(Logger::getRootLogger(), "JobDescription");

  // Maybe this mutex could go to JobDescriptionParserLoader. That would make
  // it transparent. On another hand JobDescriptionParserLoader must not know
  // how it is used.
  Glib::Mutex JobDescription::jdpl_lock;
  JobDescriptionParserLoader JobDescription::jdpl;

  JobDescription::JobDescription(const long int& ptraddr) { *this = *((JobDescription*)ptraddr); }

  JobDescription::operator bool() const {
    logger.msg(WARNING, "The JobDescription::operator bool() method is DEPRECATED, use validity checks when parsing sting or outputing contents of JobDescription object.");
    return (!Application.Executable.Name.empty());
  }

  JobDescription::JobDescription(const JobDescription& j, bool withAlternatives) {
    if (withAlternatives) {
      *this = j;
    }
    else {
      RemoveAlternatives();
      Set(j);
    }
  }

  void JobDescription::Set(const JobDescription& j) {
    Identification = j.Identification;
    Application = j.Application;
    Resources = j.Resources;
    DataStaging = j.DataStaging;
    JobMeta = j.JobMeta;

    OtherAttributes = j.OtherAttributes;

    sourceLanguage = j.sourceLanguage;
  }

  void JobDescription::RemoveAlternatives() {
    alternatives.clear();
    current = alternatives.begin();
  }

  JobDescription& JobDescription::operator=(const JobDescription& j) {
    RemoveAlternatives();
    Set(j);

    if (!j.alternatives.empty()) {
      alternatives = j.alternatives;
      current = alternatives.begin();
      for (std::list<JobDescription>::const_iterator it = j.alternatives.begin();
           it != j.current && it != j.alternatives.end(); it++) {
        current++; // Increase iterator so it points to same object as in j.
      }
    }

    return *this;
  }

  void JobDescription::Print(bool longlist) const {
    logger.msg(WARNING, "The JobDescription::Print method is DEPRECATED, use the JobDescription::SaveToStream method instead.");
    SaveToStream(std::cout, (!longlist ? "user" : "userlong")); // ??? Prepend "user*" with a character to avoid clashes?
  }

  bool JobDescription::SaveToStream(std::ostream& out, const std::string& format) const {

    if (format != "user" && format != "userlong") {
      std::string outjobdesc;
      if (!UnParse(outjobdesc, format)) {
        return false;
      }
      out << outjobdesc;
      return true;
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
        if (!it->QueueName.empty()) {
          if (it->UseQueue) {
            out << IString(" QueueName: %s", it->QueueName) << std::endl;
          }
          else {
            out << IString(" QueueName (ignored): %s", it->QueueName) << std::endl;
          }
        }
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

    if (!OtherAttributes.empty()) {
      std::map<std::string, std::string>::const_iterator it;
      for (it = OtherAttributes.begin(); it != OtherAttributes.end(); it++)
        out << IString(" Other attributes: [%s], %s", it->first, it->second) << std::endl;
    }

    out << std::endl;

    return true;
  } // end of Print

  void JobDescription::UseOriginal() {
    if (!alternatives.empty()) {
      std::list<JobDescription>::iterator it = alternatives.insert(current, *this); // Insert this before current position.
      it->RemoveAlternatives(); // No nested alternatives.
      Set(alternatives.front()); // Set this to first element.
      alternatives.pop_front(); // Remove this from list.
      current = alternatives.begin(); // Set current to first element.
    }
  }

  bool JobDescription::UseAlternative() {
    if (!alternatives.empty() && current != alternatives.end()) {
      std::list<JobDescription>::iterator it = alternatives.insert(current, *this); // Insert this before current position.
      it->RemoveAlternatives(); // No nested alternatives.
      Set(*current); // Set this to current.
      current = alternatives.erase(current); // Remove this from list.
      return true;
    }

    // There is no alternative JobDescription objects or end of list.
    return false;
  }

  void JobDescription::AddAlternative(const JobDescription& j) {
    alternatives.push_back(j);

    if (current == alternatives.end()) {
      current--; // If at end of list, set current to newly added jobdescription.
    }

    if (!j.alternatives.empty()) {
      alternatives.back().RemoveAlternatives(); // No nested alternatives.
      alternatives.insert(alternatives.end(), j.alternatives.begin(), j.alternatives.end());
    }
  }

  bool JobDescription::Parse(const XMLNode& xmlSource) {
    logger.msg(WARNING, "This method is DEPRECATED, please use the JobDescription::Parse(const std::string&, std::list<JobDescription>&, const std::string&, const std::string&) method instead.");
    std::string source;
    xmlSource.GetXML(source);
    return Parse(source);
  }

  bool JobDescription::Parse(const std::string& source, const std::string& language, const std::string& dialect) {
    logger.msg(WARNING, "This method is DEPRECATED, please use the JobDescription::Parse(const std::string&, std::list<JobDescription>&, const std::string&, const std::string&) method instead.");
    std::list<JobDescription> jobdescs;
    bool r = (Parse(source, jobdescs, language, dialect) && !jobdescs.empty());
    if (r) {
      *this = jobdescs.front();
    }
    return r;
  }

  bool JobDescription::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) {
    if (source.empty()) {
      logger.msg(ERROR, "Empty job description source string");
      return false;
    }

    jdpl_lock.lock();
    for (JobDescriptionParserLoader::iterator it = jdpl.GetIterator(); it; ++it) {
      // Releasing lock because we can't know how long parsing will take
      // But for current implementations of parsers it is not specified
      // if their Parse/Unparse methods can be called concurently.
      if ((language.empty() || it->IsLanguageSupported(language)) && it->Parse(source, jobdescs, language, dialect)) {
        jdpl_lock.unlock();
        return true;
     }
    }
    jdpl_lock.unlock();

    return false;
  }

  std::string JobDescription::UnParse(const std::string& language) const {
    logger.msg(WARNING, "This method is DEPRECATED, please use the JobDescription::UnParse(std::string&, std::string, const std::string&) method instead.");
    std::string product;
    if (UnParse(product, language)) {
      return product;
    }

    return "";
  }

  bool JobDescription::UnParse(std::string& product, std::string language, const std::string& dialect) const {
    if (language.empty()) {
      language = sourceLanguage;
      if (language.empty()) {
        logger.msg(ERROR, "Job description langauage not specified, unable to output description.");
        return false;
      }
    }

    jdpl_lock.lock();
    for (JobDescriptionParserLoader::iterator it = jdpl.GetIterator(); it; ++it) {
      if (it->IsLanguageSupported(language)) {
        logger.msg(VERBOSE, "Generating %s job description output", language);
        bool r = it->UnParse(*this, product, language, dialect);
        jdpl_lock.unlock();
        return r;
      }
    }
    jdpl_lock.unlock();

    logger.msg(ERROR, "Language (%s) not recognized by any job description parsers.", language);
    return false;
  }
} // namespace Arc
